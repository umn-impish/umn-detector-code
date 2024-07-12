#pragma once
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>

#include <DetectorService.hh>
#include <logging.hh>

DetectorMessages::HafxSettings
parse_hafx_settings(std::stringstream& ss);
DetectorMessages::X123Settings
parse_x123_settings(std::stringstream& ss);

class Listener {
private:
    int socket_fd;
    sockaddr_in from;
    std::stringstream cmd_stream;
    std::unique_ptr<DetectorService> ser;
    std::unique_ptr<std::thread> th;

    void reply(const std::string& msg);
    void error_reply(const std::string& msg);

    void listen_step();
    std::optional<DetectorMessages::DetectorCommand> receive_decode_msg();
    DetectorMessages::DetectorCommand settings_update();
    DetectorMessages::DetectorCommand debug();
    DetectorMessages::DetectorCommand hafx_debug();
    DetectorMessages::DetectorCommand x123_debug();
    DetectorMessages::DetectorCommand start_periodic_health();

public:
    static constexpr uint32_t MAX_WAIT_TIME_SEC = 3600ULL;

    Listener() =delete;

    Listener(int sock_fd, std::unique_ptr<DetectorService> service);
    ~Listener();

    void listen_loop();
};
