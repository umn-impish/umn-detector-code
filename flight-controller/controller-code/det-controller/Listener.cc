#include <Listener.hh>

/* Constants only used here */
static const std::unordered_map<std::string, DetectorMessages::HafxChannel> CH_MAP{
    {"c1", DetectorMessages::HafxChannel::C1},
    {"m1", DetectorMessages::HafxChannel::M1},
    {"m5", DetectorMessages::HafxChannel::M5},
    {"x1", DetectorMessages::HafxChannel::X1},
};

Listener::Listener(int sock_fd, std::unique_ptr<DetectorService> service) :
    socket_fd{sock_fd},
    from{},
    cmd_stream{},
    ser{std::move(service)},
    th{std::make_unique<std::thread>([this]() { ser->run(); })}
{
	log_info("constructing Listener");
}

Listener::~Listener()
{
	log_info("destructing Listener");
}

void Listener::listen_loop() {
    while (true) {
        try {
            listen_step();
        }
        catch (const DetectorException& e) {
            log_debug(std::string{"uh oh: "} + e.what());
            error_reply(e.what());
        }
    }
}

void Listener::listen_step() {
    auto reply_promise = std::promise<std::string>{};
    auto fut = reply_promise.get_future();

    auto opt_det_command = receive_decode_msg();
    if (!opt_det_command) {
        reply("given command is a no-op; no change.");
        return;
    }

    auto det_command = *opt_det_command;

    ser->push_message(DetectorMessages::PromiseWrap{
        std::move(reply_promise),
        std::move(det_command),
    });

    using namespace std::chrono_literals;
    auto status = fut.wait_for(30s);

    if (status != std::future_status::ready) {
        throw DetectorException{"command execution timed out"};
    }

    try {
        fut.wait();
        reply("ack-ok\n" + fut.get());
    }
    catch (const std::out_of_range& e) {
        error_reply("out of range; invalid hafx choice? (" + std::string{e.what()} + ")");
    }
    catch (const std::exception& e) {
        log_debug("got an error: " + std::string{e.what()} + "\nof type " + typeid(e).name());
        error_reply(std::string{"(in promise, listener) "} + e.what());
    }
}

std::optional<DetectorMessages::DetectorCommand>
Listener::receive_decode_msg() {
    size_t BUF_SZ = 65535;
    auto buf = std::make_unique<char[]>(BUF_SZ);
    std::memset(&from, 0, sizeof(from));

    // can't be const, so make a copy here
    auto fsz = static_cast<socklen_t>(sizeof(sockaddr_in));
    int bytes_received =
        recvfrom(
            socket_fd, buf.get(), BUF_SZ, 0, (sockaddr*)&from, &fsz);

    if (bytes_received < 0) {
        throw DetectorException{
            std::string{"Error reading socket: "} + strerror(errno)};
    }

    // set the cmd_stream for this iteration
    this->cmd_stream = std::stringstream{
        std::string{buf.get(), buf.get() + bytes_received}};
    // null out the buffer --> "delete" it
    buf = nullptr;

    std::string cmd_name;
    cmd_stream >> cmd_name;

    // these are the two allowed uninitialized cmds
    if (cmd_name == "terminate") {
        reply("ack-ok\nterminated");
        exit(EXIT_SUCCESS);
    }

    else if (cmd_name == "wake") {
        return DetectorMessages::Initialize{};
    }

    // die if uninitialized
    else if (!th || !ser || !ser->alive()) {
        // Special case: "stop" command given to a sleep state
        // shouldn't throw an error because it is effectively a no-op
        if (cmd_name.starts_with("stop-") || cmd_name.starts_with("sleep")) {
            return { };
        }
        std::string err_str{"Bad command given to sleeping detector: "};
        throw DetectorException{err_str + cmd_name};
    }

    else if (cmd_name == "sleep") {
        return DetectorMessages::Shutdown {};
    }

    else if (cmd_name == "start-nominal") {
        return DetectorMessages::CollectNominal {.started = false};
    }

    else if (cmd_name == "stop-nominal") {
        return DetectorMessages::StopNominal {};
    }

    else if (cmd_name == "start-nrl-list") {
        return DetectorMessages::StartNrlList {
            .started = false, .full_size = false
        };
    }

    else if (cmd_name == "start-nrl-full-size-list") {
        return DetectorMessages::StartNrlList {
            .started = false, .full_size = true
        };
    }

    else if (cmd_name == "stop-nrl-list") {
        return DetectorMessages::StopNrlList {};
    }

    else if (cmd_name == "settings-update") {
        return settings_update();
    }

    else if (cmd_name == "debug") {
        return debug();
    }

    else if (cmd_name == "start-periodic-health") {
        return start_periodic_health();
    }

    else if (cmd_name == "stop-periodic-health") {
        return DetectorMessages::StopPeriodicHealth{};
    }

    else {
        std::string err_str{"Cannot process given command: "};
        throw DetectorException{err_str + cmd_name};
    }
}

void Listener::reply(const std::string& msg) {
    bool no_sender = (from.sin_family == 0 && from.sin_port == 0);
    if (no_sender) {
        std::cerr << "NO SENDER PRESENT?" << std::endl << msg << std::endl;
        return;
    }

    int ret = sendto(
        socket_fd, msg.c_str(), msg.size(), 0,
        (const sockaddr*)&from, sizeof(sockaddr_in)
    );
    if (ret < 0) {
        throw DetectorException{"Failed to send message '" + msg + "': " + strerror(errno)};
    }
}

void Listener::error_reply(const std::string& msg) {
    reply("error\n" + msg);
}

DetectorMessages::DetectorCommand Listener::debug() {
    std::string detector;
    cmd_stream >> detector;
    if (detector == "x123") {
        return x123_debug();
    }
    else if (detector == "hafx") {
        return hafx_debug();
    }
    else {
        throw DetectorException{"Detector choice '" + detector + "' not valid for debug"};
    }
}

DetectorMessages::DetectorCommand Listener::x123_debug() {
    std::string type, set_str;
    uint32_t hg_wait{0};

    if (cmd_stream >> type) {
        if (type == "ascii_settings") {
            cmd_stream >> set_str;
            if (set_str.empty()) {
                throw DetectorException{"no x123 ascii settings given"};
            }
        }
        else if (type == "histogram") {
            cmd_stream >> hg_wait;
            if (!hg_wait) {
                throw DetectorException{"no x123 histogram wait time given (required)"};
            }
        }
    }

    if (type == "histogram" && (hg_wait == 0 || hg_wait > MAX_WAIT_TIME_SEC)) {
        throw DetectorException{
            "Debug histogram must be given in-bounds collection duration"};
    }

    using dbg_t = DetectorMessages::X123Debug;
    dbg_t::Type t;
         if (type == "histogram"        ) t = dbg_t::Type::Histogram;
    else if (type == "diagnostic"       ) t = dbg_t::Type::Diagnostic;
    else if (type == "ascii_settings"   ) t = dbg_t::Type::AsciiSettings;
    else throw DetectorException{"Invalid x123 debug type '" + type + "'"};

    dbg_t d;
    d.type = t;
    d.ascii_settings_query = set_str;
    d.histogram_wait = hg_wait;

    return d;
}

DetectorMessages::DetectorCommand Listener::hafx_debug() {
    std::string type, channel;
    uint32_t wait{0};

    cmd_stream >> channel;
    DetectorMessages::HafxDebug dbg;

    try {
        dbg.ch = CH_MAP.at(channel);
        log_debug(std::string{"debug channel we got: "} + channel);
    }
    catch (const std::out_of_range&) {
        throw DetectorException{"Ill-formed detector choice for debug '" + channel + "' given"};
    }

    if (cmd_stream >> type) {
        // continue if there is something more in the stream
        cmd_stream >> wait;
    }

    bool do_wait = (
        type == "histogram" || type == "list_mode" ||
        type == "nrl_list_mode" || type == "time_slice"
    );
    if (do_wait && (wait == 0 || wait > MAX_WAIT_TIME_SEC)) {
        log_debug("maybe waiting for " + std::to_string(wait));
        throw DetectorException{
            "Debug histogram/list_mode must be given in-bounds collection duration"};
    }

    using dbg_t = DetectorMessages::HafxDebug;
    dbg_t::Type t;
         if (type == "arm_ctrl"               ) t = dbg_t::Type::ArmCtrl;
    else if (type == "arm_cal"                ) t = dbg_t::Type::ArmCal;
    else if (type == "arm_status"             ) t = dbg_t::Type::ArmStatus;
    else if (type == "fpga_ctrl"              ) t = dbg_t::Type::FpgaCtrl;
    else if (type == "fpga_oscilloscope_trace") t = dbg_t::Type::FpgaOscilloscopeTrace;
    else if (type == "fpga_statistics"        ) t = dbg_t::Type::FpgaStatistics;
    else if (type == "fpga_weights"           ) t = dbg_t::Type::FpgaWeights;
    else if (type == "histogram"              ) t = dbg_t::Type::Histogram;
    else if (type == "list_mode"              ) t = dbg_t::Type::ListMode;
    else throw DetectorException{"Ill-formed debug request type '" + type + "'"};

    dbg.type = t;
    dbg.wait_between = wait;

    return dbg;
}

DetectorMessages::DetectorCommand Listener::settings_update() {
    std::string detector;
    cmd_stream >> detector;
    if (detector == "x123") {
        log_debug("parse x123");
        return parse_x123_settings(cmd_stream);
    }
    else if (detector == "hafx") {
        log_debug("update hafx");
        return parse_hafx_settings(cmd_stream);
    }
    throw DetectorException{"Malformed settings detector identifier: '" + detector + "'"};
}

DetectorMessages::X123Settings
parse_x123_settings(std::stringstream& ss) {
    DetectorMessages::X123Settings ret{};

    std::string token;
    ss >> token;

    if (token == "adc_rebin_edges") {
        std::vector<uint16_t> edges;
        uint16_t e;
        while (ss >> e) edges.push_back(e);
        log_debug("got " + std::to_string(edges.size()) + " x123 edges");
        ret.adc_rebin_edges_length = std::min(edges.size(), ret.adc_rebin_edges.size());
        std::copy_n(edges.begin(), ret.adc_rebin_edges_length, ret.adc_rebin_edges.begin());
    }
    else if (token == "ack_err_retries") {
        uint32_t retries;
        ss >> retries;
        log_debug("retry for " + std::to_string(retries));
        ret.ack_err_retries = retries;
        ret.ack_err_retries_present = true;
    }
    else if (token == "ascii_settings") {
        ss >> token;
        log_debug("got " + std::to_string(token.size()) + "-character x123 ascii settings string");
        ret.ascii_settings_length = std::min(token.size(), ret.ascii_settings.size());
        std::copy_n(token.begin(), ret.ascii_settings_length, ret.ascii_settings.begin());
    }
    else {
        throw DetectorException{"Invalid x123 settings modifier '" + token + "'"};
    }

    return ret;
}

DetectorMessages::HafxSettings
parse_hafx_settings(std::stringstream& ss) {
    DetectorMessages::HafxSettings ret {};

    std::string token;
    ss >> token;
    try { ret.ch = CH_MAP.at(token); }
    catch (const std::out_of_range&) {
        throw DetectorException("Ill-formed detector for settings update '" + token + "' given");
    }
    ss >> token;

    size_t i = 0;
    if (token == "fpga_ctrl") {
        SipmUsb::FpgaCtrl::Registers r;
        SipmUsb::FpgaCtrl::Registers::value_type v;
        while (ss >> v && i < r.size())
            r[i++] = v;
        ret.fpga_ctrl_regs_present = true;
        ret.fpga_ctrl_regs = r;
    }
    else if (token == "arm_cal") {
        SipmUsb::ArmCal::Registers r;
        SipmUsb::ArmCal::Registers::value_type v;
        while (ss >> v && i < r.size())
            r[i++] = v;
        ret.arm_cal_regs_present = true;
        ret.arm_cal_regs = r;
    }
    else if (token == "arm_ctrl") {
        SipmUsb::ArmCtrl::Registers r;
        SipmUsb::ArmCtrl::Registers::value_type v;
        while (ss >> v && i < r.size())
            r[i++] = v;
        ret.arm_ctrl_regs_present = true;
        ret.arm_ctrl_regs = r;
    }
    else if (token == "fpga_weights") {
        SipmUsb::FpgaWeights::Registers r;
        SipmUsb::FpgaWeights::Registers::value_type v;
        while (ss >> v && i < r.size())
            r[i++] = v;
        ret.fpga_weights_regs_present = true;
        ret.fpga_weights_regs = r;
    }
    else if (token == "adc_rebin_edges") {
        std::vector<uint16_t> bin_map;
        uint16_t b;
        while (ss >> b) bin_map.push_back(b);
	log_debug("number of elements: " + std::to_string(bin_map.size()));
        ret.adc_rebin_edges_length = std::min(bin_map.size(), ret.adc_rebin_edges.size());
        std::copy_n(bin_map.begin(), ret.adc_rebin_edges_length, ret.adc_rebin_edges.begin());
    }
    else {
        throw DetectorException{"Invalid settings modifier '" + token + "'"};
    }

    return ret;
}

DetectorMessages::DetectorCommand Listener::start_periodic_health() {
    auto extract_ip_port = [](const std::string& addy) {
        auto port_div_pos = addy.find(':');
        if (port_div_pos == std::string::npos) {
            throw DetectorException{"can't find port from ip string"};
        }

        auto ip = addy.substr(0, port_div_pos);
        if (ip == "localhost") {
            ip = "127.0.0.1";
        }

        auto port = std::stoi(addy.substr(port_div_pos+1, std::string::npos));

        return sockaddr_in {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = {
                .s_addr = inet_addr(ip.c_str())
            },
            .sin_zero = {0},
        };
    };

    uint32_t sec_between = 0;
    cmd_stream >> sec_between;
    if (sec_between == 0) {
        throw DetectorException{
            "Need to provide valid wait time between health"
            " packet acquisitions. (>1 s)"
        };
    }
    std::string s;
    std::vector<sockaddr_in> fwds;
    while (cmd_stream >> s) {
        fwds.emplace_back(extract_ip_port(s));
    }

    if (fwds.empty()) {
        throw DetectorException{
            "Need at least one address to send health data to."
        };
    }

    return DetectorMessages::StartPeriodicHealth{
        .seconds_between = sec_between,
        .fwd = std::move(fwds)
    };
}
