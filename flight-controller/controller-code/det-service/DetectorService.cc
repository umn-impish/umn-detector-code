#include <ctime>
#include <chrono>
#include <iostream>
#include <string>

#include "DetectorService.hh"

namespace {
namespace dm = DetectorMessages;
using hafx_chan = dm::HafxChannel;

using namespace std::chrono_literals;
}

DetectorService::DetectorService(int socket_fd) :
    socket_fd(socket_fd),
    _alive{false},
    x123_ports{},
    hafx_ports{},
    queue{},
    x123_ctrl{nullptr},
    hafx_ctrl{},
    hafx_serial_nums{}
{ }

void DetectorService::put_hafx_ports(
    std::unordered_map<dm::HafxChannel, Detector::DetectorPorts> p) {
    hafx_ports = p;
}

void DetectorService::put_x123_ports(Detector::DetectorPorts p) {
    x123_ports = p;
}

void DetectorService::put_hafx_serial_nums(
    std::unordered_map<dm::HafxChannel, std::string> nums) {
    hafx_serial_nums = nums;
}

DetectorService::~DetectorService() { }

void DetectorService::run() {
    while (true) {
        evt_loop_step();
    }
}

void DetectorService::evt_loop_step() {
    static const auto cmd_visitor = [this](auto cmd) {
        try {
            try {
                handle_command(std::move(cmd));
            }
            catch (const ReconnectDetectors& e) {
                // this will get caught if we have a timeout or other USB error
                // TODO add flag that we had a communication issue in health packet
                log_error("Reconnecting detectors: " + std::string{e.what()});
                reconnect_detectors();
                if (taking_nominal_data()) {
                    push_message(dm::StopNominal{});
                    push_message(dm::CollectNominal{.started = false});
                }
            }
        }
        catch (const std::exception& e) {
            std::stringstream ss;
            ss << "uncaught error: " << e.what() << std::endl
                      << "type: " << typeid(e).name();
            log_error(ss.str());
            handle_command(dm::Shutdown{});
        }
    };

    auto command = queue.pop();
    std::visit(cmd_visitor, std::move(command));
}

void DetectorService::handle_command(dm::PromiseWrap msg) {
    auto p = std::move(msg.prom);
    try {
        std::visit([this](auto m) {
            handle_command(std::move(m));
        }, msg.wrap_msg);
        p.set_value("promise-fulfilled");
    }
    catch (const std::exception& e) {
        std::stringstream ss;
        ss << "error in promise-wrapped: " << e.what() << std::endl
           << "type is: " << typeid(e).name();
        log_debug(ss.str());
        p.set_exception(std::current_exception());
    }
}

void DetectorService::handle_command(dm::Initialize) {
    initialize();
}

void DetectorService::reconnect_detectors() {
    // HaFX detectors (scintillators)
    hafx_ctrl.clear();

    auto bridgeport_device_manager = std::make_shared<SipmUsb::BridgeportDeviceManager>();

    for (const auto& [chan, sn] : hafx_serial_nums) {
        if (!bridgeport_device_manager->device_map.contains(sn)) {
            continue;
        }
        try {
            hafx_ctrl[chan] = std::make_unique<Detector::HafxControl>(
                bridgeport_device_manager->device_map[sn],
                hafx_ports[chan]
            );
        } catch (const std::runtime_error& e) {
            push_message(dm::Shutdown{});
            throw DetectorException{std::string{"making hafx control: "} + e.what()};
        }
    }

    // X-123
    // release the resource before re-making it
    x123_ctrl.reset();
    x123_ctrl = std::make_unique<Detector::X123Control>(x123_ports);
}

void DetectorService::initialize() {
    // shut off everything before re-init
    // (go into a clean state)
    handle_command(dm::Shutdown{});

    // do this before anything else
    reconnect_detectors();

    // send out settings each time (from disk to detector RAM)
    for (auto& [ch, ctrl] : hafx_ctrl) {
        try {
            ctrl->update_settings(ctrl->fetch_settings());
        } catch (const std::runtime_error& e) {
            log_warning("settings load: " + std::string{e.what()});
        }
    }

    try {
        auto s = x123_ctrl->fetch_settings();
        x123_ctrl->update_settings(s);
    } catch (const std::runtime_error& e) {
        log_warning(e.what());
    }

    _alive = true;
}

void DetectorService::handle_command(dm::Shutdown) {
    nominal_timer = nullptr;
    health_timer = nullptr;
    hafx_debug_hist_timer = nullptr;
    hafx_debug_list_timer = nullptr;
    x123_debug_hist_timer = nullptr;

    x123_ctrl = nullptr;
    hafx_ctrl.clear();

    _alive = false;
    log_debug("detector shutdown");
}

void DetectorService::send_health(
    const sockaddr_in& dest,
    const DetectorMessages::HealthPacket& hp
) {
    constexpr auto HEALTH_SZ = sizeof(DetectorMessages::HealthPacket);
    std::string send(HEALTH_SZ, '\0');

    std::memcpy(send.data(), (char*)&hp, HEALTH_SZ);
    int ret = sendto(
        socket_fd,
        send.data(),
        send.size(),
        0,
        (const sockaddr*)&dest,
        sizeof(sockaddr_in)
    );

    if (ret < 0) {
        std::string err{strerror(errno)};
        throw DetectorException{"Problem sending health packet: " + err};
    }
}

void DetectorService::handle_command(dm::ManualHealthPacket cmd) {
    send_health(cmd.destination, generate_health());
}

void DetectorService::handle_command(dm::StartPeriodicHealth cmd) {
    auto hp = generate_health();
    for (const auto& dest : cmd.fwd) {
        send_health(dest, hp);
    }

    using namespace std::chrono_literals;
    health_timer = TimerLifetime::create(
        queue.push_delay(cmd, cmd.seconds_between * 1s)
    );
}

void DetectorService::handle_command(dm::StopPeriodicHealth) {
    health_timer = nullptr;
}

dm::HealthPacket DetectorService::generate_health() {
    dm::HealthPacket health_pack;
    std::memset(&health_pack, 0, sizeof(dm::HealthPacket));

    health_pack.timestamp = time(NULL);

    // only add health from connected detectors
    if (hafx_ctrl.contains(dm::HafxChannel::C1))
        health_pack.c1 = hafx_ctrl[dm::HafxChannel::C1]->generate_health();
    if (hafx_ctrl.contains(dm::HafxChannel::M1))
        health_pack.m1 = hafx_ctrl[dm::HafxChannel::M1]->generate_health();
    if (hafx_ctrl.contains(dm::HafxChannel::M5))
        health_pack.m5 = hafx_ctrl[dm::HafxChannel::M5]->generate_health();
    if (hafx_ctrl.contains(dm::HafxChannel::X1))
        health_pack.x1 = hafx_ctrl[dm::HafxChannel::X1]->generate_health();

    try {
        health_pack.x123 = x123_ctrl->generate_health();
    } catch (const DetectorException& e) {
        log_debug(e.what());
    }

    return health_pack;
}

void DetectorService::handle_command(dm::HafxSettings cmd) {
    auto channel = cmd.ch;
    try {
        hafx_ctrl.at(channel)->update_settings(cmd);
    } catch (const std::out_of_range&) {
        throw DetectorException{"Channel not valid for settings modification (detector not connected)"};
    }
}

void DetectorService::handle_command(dm::X123Settings cmd) {
    try {
        x123_ctrl->update_settings(cmd);
    } catch (const std::runtime_error& e) {
        throw DetectorException{"X123 issue: " + std::string{e.what()}};
    }
}

void DetectorService::handle_command(dm::HafxDebug cmd) {
    if (taking_nominal_data()) {
        throw DetectorException{"Cannot take debug data during nominal data collection"};
    }

    using dbr_t = dm::HafxDebug;
    const auto acq_type = cmd.type;
    auto& ctrl = hafx_ctrl.at(cmd.ch);

    auto delay = std::chrono::seconds(cmd.wait_between);
    // basic reads
    if (acq_type == dbr_t::Type::ArmCtrl)
        ctrl->read_save_debug<SipmUsb::ArmCtrl>();
    else if (acq_type == dbr_t::Type::ArmCal)
        ctrl->read_save_debug<SipmUsb::ArmCal>();
    else if (acq_type == dbr_t::Type::ArmStatus)
        ctrl->read_save_debug<SipmUsb::ArmStatus>();
    else if (acq_type == dbr_t::Type::FpgaCtrl)
        ctrl->read_save_debug<SipmUsb::FpgaCtrl>();
    else if (acq_type == dbr_t::Type::FpgaStatistics)
        ctrl->read_save_debug<SipmUsb::FpgaStatistics>();
    else if (acq_type == dbr_t::Type::FpgaWeights)
        ctrl->read_save_debug<SipmUsb::FpgaWeights>();

    // reads after a delay
    else if (acq_type == dbr_t::Type::Histogram) {
        ctrl->restart_time_slice_or_histogram();
        hafx_debug_hist_timer = TimerLifetime::create(
            queue.push_delay(dm::QueryLegacyHistogram{cmd.ch}, delay)
        );
    }
    else if (acq_type == dbr_t::Type::ListMode) {
        ctrl->restart_list();
        hafx_debug_list_timer = TimerLifetime::create(
            queue.push_delay(dm::QueryListMode{cmd.ch}, delay)
        );
    }
}

void DetectorService::handle_command(dm::X123Debug cmd) {
    if (taking_nominal_data()) {
        throw DetectorException{"Cannot take debug data during nominal data collection"};
    }

    if (!x123_ctrl || !x123_ctrl->driver_valid()) {
        throw DetectorException{"X123 not connected"};
    }

    try {
        x123_debug(std::move(cmd));
    } catch (const std::runtime_error& e) {
        log_error(e.what());
    }
}

void DetectorService::x123_debug(dm::X123Debug cmd) {
    auto what = cmd.type;
    using dbg_t = dm::X123Debug;
    if (what == dbg_t::Type::Diagnostic) {
        x123_ctrl->read_save_debug_diagnostic();
    }

    else if (what == dbg_t::Type::Histogram) {
        x123_ctrl->init_debug_histogram();
        auto delay = std::chrono::seconds(cmd.histogram_wait);
        x123_debug_hist_timer = TimerLifetime::create(
            queue.push_delay(dm::QueryX123DebugHistogram{}, delay)
        );
    }

    else if (what == dbg_t::Type::AsciiSettings) {
        log_debug("ascii is: " + cmd.ascii_settings_query);
        x123_ctrl->read_save_debug_ascii(cmd.ascii_settings_query);
    }
}

void DetectorService::handle_command(dm::QueryListMode cmd) {
    using list_t = SipmUsb::FpgaListMode;
    hafx_ctrl.at(cmd.ch)->read_save_debug<list_t>();
}

void DetectorService::handle_command(dm::QueryLegacyHistogram cmd) {
    using hg_t = SipmUsb::FpgaHistogram;
    hafx_ctrl.at(cmd.ch)->read_save_debug<hg_t>();
}

void DetectorService::handle_command(dm::QueryX123DebugHistogram) {
    x123_ctrl->read_save_debug_histogram();
}

void DetectorService::await_pps_edge() {
    static const uint8_t PPS_DETECT_PIN = 23;
    RaiiGpioPin p{PPS_DETECT_PIN, RaiiGpioPin::Operation::rising_edge};
    // wait 2s for PPS
    struct timespec timeout {.tv_sec = 2, .tv_nsec = 0};
    auto detected = p.edge_event(timeout);
    if (!detected) {
        log_warning(
            "Cannot obtain PPS detect after " + 
            std::to_string(timeout.tv_sec) +
            " seconds"
        );
    }
}

void DetectorService::read_all_time_slices() {
    for (const auto& [ch, ctrl] : hafx_ctrl) {
        try {
            ctrl->poll_save_time_slice();
        }
        catch (std::runtime_error const& e) {
            throw ReconnectDetectors{"hafx issue: " + std::string{e.what()}};
        }
    }
}

void DetectorService::handle_command(dm::CollectNominal cmd) {
    auto finish = [this](auto cmd) {
        constexpr auto TIME_SLICE_DELAY = 2s;
        nominal_timer = TimerLifetime::create(queue.push_delay(cmd, TIME_SLICE_DELAY));
    };

    if (not cmd.started) {
        start_nominal();
        cmd.started = true;
        finish(cmd);
        return;
    }
    try {
        x123_ctrl->read_save_sequential_buffer();
    } catch (const DetectorException& e) {
        log_debug("x123 disconnected: " + std::string{e.what()});
    }

    read_all_time_slices();
    finish(cmd);
}

void DetectorService::start_nominal() {
    // wait until the PPS comes in to start these operations
    // for a good initial time sync
    // assumption: all of the rest of the init process can take place in < 1s
    await_pps_edge();

    // set to "nothing" value for initial synchronizing read
    for (const auto& [ch, ctrl] : hafx_ctrl) {
        ctrl->data_time_anchor({});
    }

    auto after_pps = std::chrono::system_clock::now();
    // add 1: next PPS will be the one that initiates measurements
    auto restore_anchor = std::chrono::system_clock::to_time_t(after_pps) + 1;

    try {
        x123_ctrl->data_time_anchor(restore_anchor);
        x123_ctrl->restart_hardware_controlled_sequential_buffering();
    } catch (const DetectorException& e) {
        log_warning("X123 issue: " + std::string{e.what()});
    }

    for (auto& [ch, ctrl] : hafx_ctrl) {
        ctrl->restart_time_slice_or_histogram();
    }

    // wait until at least 1 buffer is available in all Bridgeport 
    // detectors
    using namespace std::chrono_literals;
    while ((std::chrono::system_clock::now() - after_pps) <= 256ms);

    // read initial buffers (they are garbage)
    read_all_time_slices();

    // set the actual anchor time for the
    // now-synchronized slice collection
    for (auto& [ch, ctrl] : hafx_ctrl) {
        ctrl->data_time_anchor(restore_anchor);
    }
}

void DetectorService::handle_command(dm::StopNominal) {
    try {
        x123_ctrl->stop_sequential_buffering();
    } catch (const DetectorException& e) {
        log_warning("X123 issue: " + std::string{e.what()});
    }
    nominal_timer = nullptr;
}

// TODO: NRL LIST MODE

void DetectorService::push_message(DetectorService::Message m) {
    queue.push(std::move(m));

}
bool DetectorService::taking_nominal_data() {
    return (nominal_timer != nullptr);
}

bool DetectorService::alive() const {
    return _alive;
}
