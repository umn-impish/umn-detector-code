#include <sstream>

#include <X123Control.hh>

namespace req = X123Driver::Packets::Requests;
namespace res = X123Driver::Packets::Responses;

namespace Detector {

X123Control::X123Control(DetectorPorts ports) :
    driver{std::make_unique<X123DriverWrap>()},
    local_next_buffer_num{0},
    science_saver{std::make_unique<DataSaver>(ports.science)},
    debug_saver{std::make_unique<DataSaver>(ports.debug)},
    settings_saver{std::make_unique<SettingsSaver>("x123-settings.bin")},
    settings{fetch_settings()}
{
    // If X-123 is disconnected, set default # bins to 1024
    try {
        num_histogram_bins_from_ram();
    } catch (DetectorException const&) {
        log_warning("X-123 disconnected; using 1024 bins as default");
        num_histogram_bins = 1024;
    }
}

X123Control::~X123Control() { }

DetectorMessages::X123Health
X123Control::generate_health() {
    DetectorMessages::X123Health ret;

    res::Status stat;
    driver->send_recv(req::Status{}, stat);
    const auto& buf = stat.parsedBuffer();

    // See Amptek programmer's guide for indices, units, etc.
    ret.board_temp = buf[34];

    ret.det_temp =
        (static_cast<uint16_t>(buf[32] & 0xf) << 8) |
         static_cast<uint16_t>(buf[33]);

    ret.det_high_voltage = static_cast<int16_t>(
        (static_cast<int16_t>(buf[30]) << 8) |
         static_cast<int16_t>(buf[31])
    );

    #define bytes_to_u32(a, b, c, d) \
         static_cast<uint32_t>(buf[a])        | \
        (static_cast<uint32_t>(buf[b]) << 8)  | \
        (static_cast<uint32_t>(buf[c]) << 16) | \
        (static_cast<uint32_t>(buf[d]) << 24)

    ret.fast_counts = bytes_to_u32(0, 1, 2, 3);
    ret.slow_counts = bytes_to_u32(4, 5, 6, 7);

    // the top 3B are in 100ms
    uint32_t acc_big = (bytes_to_u32(12, 13, 14, 15)) >> 8;
    // this is in 1ms/tick
    uint32_t acc_small = buf[12];
    // convert both to miliseconds
    auto total_acc_time = acc_small + (acc_big * 100);
    ret.accumulation_time = total_acc_time;

    ret.real_time = bytes_to_u32(20, 21, 22, 23);
    #undef bytes_to_u32
    return ret;
}

void X123Control::read_save_sequential_buffer() {
    if (local_next_buffer_num == 0) {
        // wait until buffer #0 is done to read out.
        ++local_next_buffer_num;
        return;
    }

    uint32_t pre_read_time = static_cast<uint32_t>(time_anchor);
    auto pack = elicit_spectrum_packet();
    driver->send_recv(
        req::RequestBuffer{static_cast<uint16_t>(this->local_next_buffer_num-1)},
        *pack
    );
    auto buffer_span = std::span(pack->parsedBuffer());
    auto spectrum_span = buffer_span.subspan(0, buffer_span.size() - res::Status::SIZE);
    auto status_span = buffer_span.subspan(buffer_span.size() - res::Status::SIZE, res::Status::SIZE);

    auto rebinned_spectrum = rebin_spectrum(assemble_spectrum(spectrum_span));

    // Need "normal" status, not the one associated with the 
    // spectrum+status of the HCSBO buffer
    res::Status status{};
    driver->send_recv(req::Status{}, status);
    increment_reset_buffering(status.parsedBuffer());

    // Write format:
    // uint32_t: time just before read
    // 64B: status data
    // uint16_t: size of rebinned spectrum (N)
    // uint32_t[N] rebinned X-123 spectrum
    uint16_t spec_sz = static_cast<uint16_t>(rebinned_spectrum.size());
    std::stringstream nominal_spectrum_status;
    nominal_spectrum_status.write(reinterpret_cast<char const*>(&pre_read_time), sizeof(pre_read_time));
    nominal_spectrum_status.write(reinterpret_cast<char const*>(status_span.data()), status_span.size());
    nominal_spectrum_status.write(reinterpret_cast<char const*>(&spec_sz), sizeof(spec_sz));
    nominal_spectrum_status.write(
            reinterpret_cast<char const*>(rebinned_spectrum.data()),
            spec_sz * sizeof(rebinned_spectrum[0]));
    science_saver->add(nominal_spectrum_status.str());
}

void X123Control::restart_hardware_controlled_sequential_buffering() {
    local_next_buffer_num = 0;
    static constexpr const char* BUF_SETTINGS =
        "GPED=RISING;" // counter triggers on rising edge
        "GPGA=OFF;"    // counter does not use "gate" (?)
        "GPIN=AUX2;"   // counter input is AUX2
        "GPMC=OFF;"    // counter not cleared with MCA counters
        "GPME=OFF";    // counter does care about McaEnable

    // write these to RAM to "preserve" the flash mem
    driver->send_recv(req::TextConfigurationToRam{BUF_SETTINGS}, res::Ack{});
    driver->send_recv(req::ClearGeneralPurposeCounter{}, res::Ack{});
    driver->send_recv(req::RestartSequentialBuffering{}, res::Ack{});
}

void X123Control::stop_sequential_buffering() {
    driver->send_recv(req::CancelSequentialBuffering{}, res::Ack{});
}

std::vector<uint32_t>
X123Control::assemble_spectrum(std::span<const uint8_t> buf) {
    std::vector<uint32_t> assembled;

    for (size_t i = 0; i < buf.size(); i += 3) {
        uint32_t a = buf[i], b = buf[i+1], c = buf[i+2];
        assembled.emplace_back(a | (b << 8) | (c << 16));
    }

    return assembled;
}

std::vector<uint32_t>
X123Control::rebin_spectrum(std::vector<uint32_t> const& orig_spectrum) {
    // if no edges given, do not rebin
    if (!settings.adc_rebin_edges_length) {
        return orig_spectrum;
    }

    const auto& rebin_edges = settings.adc_rebin_edges;

    for (const auto& e : orig_spectrum) {
        if (e > orig_spectrum.size()) {
            throw DetectorException{"X123 rebin edges out of bounds"};
        }
    }

    std::vector<uint32_t> ret;
    // minus 1 because edges come in pairs
    size_t limit = settings.adc_rebin_edges_length - 1;
    for (size_t edge_idx = 0; edge_idx < limit; ++edge_idx) {
        const auto start_bin_edge = rebin_edges[edge_idx];
        const auto stop_bin_edge = rebin_edges[edge_idx+1];

        uint32_t cur = 0;
        for (size_t j = start_bin_edge; j < stop_bin_edge; ++j) {
            cur += orig_spectrum[j];
        }
        ret.emplace_back(cur);
    }
    return ret;
}

void X123Control::increment_reset_buffering(const std::vector<uint8_t>& status_bytes) {
    // programmer's guide page 73
    uint16_t remote_next_buffer_num = 
        (static_cast<uint16_t>(status_bytes[46] & 0x1) << 8ULL) |
        (static_cast<uint16_t>(status_bytes[47] & 0xff));
    
    // we are ahead so do nothing
    if (remote_next_buffer_num < local_next_buffer_num) {
        return;
    }

    // increment the timestep and buffer num if we are ready to do so
    ++local_next_buffer_num;
    ++time_anchor;

    // we are behind
    if (remote_next_buffer_num > local_next_buffer_num) {
        read_save_sequential_buffer();
        return;
    }

    bool buffering_stopped = !(status_bytes[46] & 0x2);
    if (buffering_stopped) {
        restart_hardware_controlled_sequential_buffering();
    }
}

void X123Control::update_settings(
        DetectorMessages::X123Settings const& new_settings) {
    if (new_settings.adc_rebin_edges_length) {
        log_debug("new settings rebin edges (service): ");
        std::stringstream ss;
        for (size_t i = 0; i < new_settings.adc_rebin_edges_length; ++i) {
            ss << new_settings.adc_rebin_edges[i] << ' ';
        }
        log_debug(ss.str());
        settings.adc_rebin_edges_length = new_settings.adc_rebin_edges_length;
        settings.adc_rebin_edges = new_settings.adc_rebin_edges;
    }

    if (new_settings.ack_err_retries_present) {
        settings.ack_err_retries_present = new_settings.ack_err_retries_present;
        settings.ack_err_retries = new_settings.ack_err_retries;
        log_debug("new ack err retries (service): " + std::to_string(settings.ack_err_retries));
        driver->num_retries(settings.ack_err_retries);
    }

    if (new_settings.ascii_settings_length) {
        auto start = new_settings.ascii_settings.data();
        auto s = std::string(start, start + new_settings.ascii_settings_length);
        log_debug("new ascii settings (service): " + s);
        settings.ascii_settings_length = new_settings.ascii_settings_length;
        settings.ascii_settings = new_settings.ascii_settings;
    }

    settings_saver->write_struct<DetectorMessages::X123Settings>(settings);

    // Write the settings to the X-123 after everything else
    // has been verified/configured
    upload_ascii_settings(
        std::string(
            settings.ascii_settings.data(),
            settings.ascii_settings_length
        )
    );
    num_histogram_bins_from_ram();
}

DetectorMessages::X123Settings
X123Control::fetch_settings() {
    try {
        return settings_saver->read_struct<DetectorMessages::X123Settings>();
    } catch (const DetectorException&) {
        return DetectorMessages::X123Settings{};
    }
}

void X123Control::upload_ascii_settings(const std::string& ascii) {
    req::TextConfigurationToNvram tconf;
    res::Ack ack;

    try {
        tconf.updateSettingsString(ascii);
        driver->send_recv(tconf, ack);
        log_debug("settings uploaded successfully");
    }
    catch (X123Driver::GenericException const& e) {
        throw DetectorException(std::string{"X123 exception: "} + e.what());
    }
    catch (X123Driver::Packets::AckError const& e) {
        res::Ack errored_ack{e};
        std::string emsg{errored_ack.issue()};
        throw DetectorException{std::string{"error sending x123 msg: "} + emsg};
    }
}

void X123Control::data_time_anchor(time_t new_anchor) {
    time_anchor = new_anchor;
}

time_t X123Control::data_time_anchor() {
    return time_anchor;
}

std::unique_ptr<res::BaseSpectrum>
X123Control::elicit_spectrum_packet() {
        if (num_histogram_bins == 256)
            return std::make_unique<res::Spectrum256>();
        if (num_histogram_bins == 512)
            return std::make_unique<res::Spectrum512>();
        if (num_histogram_bins == 1024)
            return std::make_unique<res::Spectrum1024>();
        if (num_histogram_bins == 2048)
            return std::make_unique<res::Spectrum2048>();
        if (num_histogram_bins == 4096)
            return std::make_unique<res::Spectrum4096>();
        else
            throw DetectorException{
                "Cannot read number of bins requested (X123 nominal read)"};
}

void X123Control::read_save_debug_diagnostic() {
    res::DiagnosticData diag_res;
    driver->send_recv(req::DiagnosticData{}, diag_res);
    save_debug(
        DetectorMessages::X123Debug::Type::Diagnostic,
        diag_res);
}

void X123Control::read_save_debug_histogram() {
    auto spec = elicit_spectrum_packet();
    driver->send_recv(req::SpectrumPlusStatus{}, *spec);
    save_debug(
        DetectorMessages::X123Debug::Type::Histogram,
        *spec);
}

void X123Control::init_debug_histogram() {
    driver->send_recv(req::CancelSequentialBuffering{}, res::Ack{});
    driver->send_recv(req::MCADisable{}, res::Ack{});
    driver->send_recv(req::ClearSpectrum{}, res::Ack{});
    driver->send_recv(req::MCAEnable{}, res::Ack{});
}

void X123Control::read_save_debug_ascii(const std::string& ascii_query) {
    log_debug("ascii query in ctrl is: " + ascii_query);
    req::TextConfigurationReadback rb_req{ascii_query};
    res::TextConfigurationReadback rb_res;
    driver->send_recv(rb_req, rb_res);

    const auto& buf = rb_res.parsedBuffer();
    std::string repl{buf.begin(), buf.end()};
    log_debug("reply is: " + repl);
    save_debug(DetectorMessages::X123Debug::Type::AsciiSettings, rb_res);
}

void X123Control::save_debug(
    DetectorMessages::X123Debug::Type const& type,
    X123Driver::Packets::BasePacket const& pack
) {
    std::stringstream save;
    save.write(reinterpret_cast<char const*>(&type), sizeof(type));

    // Some replies (like ASCII requests) may be 
    // variable-length, so we need to save the size in addition to the data.
    auto const& buf = pack.parsedBuffer();
    uint32_t const buf_sz = buf.size();
    save.write(reinterpret_cast<char const*>(&buf_sz), sizeof(buf_sz));
    save.write(reinterpret_cast<char const*>(buf.data()), buf.size());

    debug_saver->add(save.str());
}

bool X123Control::driver_valid() const {
    return driver->valid();
}

void X123Control::num_histogram_bins_from_ram() {
    auto extract_bins = [](std::string const& s) {
        auto tok_loc = s.find("MCAC=");
        auto start = s.find("=", tok_loc);
        auto end = s.find(";", start);

        // go past the '=' sign
        start += 1;
        auto str_bins = s.substr(start, end - start);
        return static_cast<uint16_t>(stoi(str_bins));
    };
    X123Driver::Packets::Requests::TextConfigurationReadback tconf{"MCAC=;"};
    X123Driver::Packets::Responses::TextConfigurationReadback tconf_rb{};

    driver->send_recv(tconf, tconf_rb);
    auto transfer_buf = tconf_rb.parsedBuffer();
    std::string response_str{transfer_buf.begin(), transfer_buf.end()};
    num_histogram_bins = extract_bins(response_str);
}

} // namespace Detector
