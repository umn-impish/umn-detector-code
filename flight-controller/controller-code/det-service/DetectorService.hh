#pragma once

#include <optional>
#include <variant>
#include <vector>

#include <logging.hh>
#include <thread_safe_queue.hh>

#include <DetectorMessages.hh>

#include <HafxControl.hh>
#include <X123Control.hh>

// for monitoring PPS
#include "raii_gpio.h"

class DetectorService {
public:
    using Message = std::variant<
        DetectorMessages::Initialize,
        DetectorMessages::Shutdown, 

        DetectorMessages::HafxSettings,
        DetectorMessages::X123Settings,

        DetectorMessages::HafxDebug,
        DetectorMessages::X123Debug,
        DetectorMessages::QueryListMode,
        DetectorMessages::QueryTraceAcquisition,
        DetectorMessages::QueryLegacyHistogram,
        DetectorMessages::QueryX123DebugHistogram,

        DetectorMessages::StartPeriodicHealth,
        DetectorMessages::StopPeriodicHealth,
        DetectorMessages::CollectNominal,
        DetectorMessages::StopNominal,
        DetectorMessages::PromiseWrap
    >;

    void push_message(Message c);
    bool taking_nominal_data();

    DetectorService(int socket_fd);
    ~DetectorService();

    void run();
    void evt_loop_step();

    bool alive() const;
    
    // make the constructor easier on the eyes
    void put_hafx_ports(
        std::unordered_map<DetectorMessages::HafxChannel, Detector::DetectorPorts>);
    void put_x123_ports(Detector::DetectorPorts);
    void put_hafx_serial_nums(
        std::unordered_map<DetectorMessages::HafxChannel, std::string>);

private:
    int socket_fd;

    bool _alive;

    Detector::DetectorPorts x123_ports;
    std::unordered_map<
        DetectorMessages::HafxChannel, Detector::DetectorPorts> hafx_ports;

    ThreadSafeQueue<Message> queue; 
    std::unique_ptr<Detector::X123Control> x123_ctrl;

    std::unordered_map<
        DetectorMessages::HafxChannel,
        std::unique_ptr<Detector::HafxControl> > hafx_ctrl;
    std::unordered_map<
        DetectorMessages::HafxChannel,
        std::string> hafx_serial_nums;

    // timers
    std::unique_ptr<TimerLifetime> nominal_timer;
    std::unique_ptr<TimerLifetime> health_timer;
    std::unique_ptr<TimerLifetime> hafx_debug_trace_timer;
    std::unique_ptr<TimerLifetime> hafx_debug_hist_timer;
    std::unique_ptr<TimerLifetime> hafx_debug_list_timer;
    std::unique_ptr<TimerLifetime> x123_debug_hist_timer;
    std::unique_prt<TimerLifetime> hafx_nrl_list_timer;

    // command handlers
    void handle_command(DetectorMessages::Initialize cmd);
    void handle_command(DetectorMessages::Shutdown cmd);
    void handle_command(DetectorMessages::HafxSettings cmd);
    void handle_command(DetectorMessages::X123Settings cmd);
    void handle_command(DetectorMessages::HafxDebug cmd);
    void handle_command(DetectorMessages::X123Debug cmd);
    void handle_command(DetectorMessages::QueryTraceAcquisition cmd);
    void handle_command(DetectorMessages::QueryLegacyHistogram cmd);
    void handle_command(DetectorMessages::QueryListMode cmd);
    void handle_command(DetectorMessages::QueryX123DebugHistogram cmd);
    void handle_command(DetectorMessages::StopNominal cmd);
    void handle_command(DetectorMessages::StartPeriodicHealth cmd);
    void handle_command(DetectorMessages::StopPeriodicHealth cmd);
    void handle_command(DetectorMessages::CollectNominal cmd);
    void handle_command(DetectorMessages::StartNrlList cmd);
    void handle_command(DetectorMessages::StopNrlList cmd);
    void handle_command(DetectorMessages::PromiseWrap msg);

    // implementations & helpers for `handle_command`s
    DetectorMessages::HealthPacket generate_health();
    void send_health(
        const sockaddr_in& dest,
        const DetectorMessages::HealthPacket& hp
    );

    // helpers
    void await_pps_edge();
    void initialize();
    void start_nominal();
    void read_all_time_slices();
    void reconnect_detectors();
    void x123_debug(DetectorMessages::X123Debug);
};
