#include <main.hh>

int main(int argc, char* argv[]) {
    if (argc != 1) {
        usage(argv[0]);
        return -1;
    }

    auto sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    using hc = DetectorMessages::HafxChannel;

    // Serial numbers for HaFX detectors
    using SerialMap = std::unordered_map<DetectorMessages::HafxChannel, std::string>;
    const SerialMap sns {
        {hc::C1, std::getenv("HAFX_C1_SERIAL")},
        {hc::M1, std::getenv("HAFX_M1_SERIAL")},
        {hc::M5, std::getenv("HAFX_M5_SERIAL")},
        {hc::X1, std::getenv("HAFX_X1_SERIAL")},
    };

    // UDP ports for HaFX detectors
    using PortMap = std::unordered_map<DetectorMessages::HafxChannel, Detector::DetectorPorts>;

    auto port_env = [](auto ident) {
        return static_cast<unsigned short>(std::atoi(std::getenv(ident)));
    };

    // ugly
    using detp = Detector::DetectorPorts;
    const PortMap hafx_ports {
        {hc::C1, detp{port_env("HAFX_C1_SCI_PORT"), port_env("HAFX_C1_DBG_PORT")}},
        {hc::M1, detp{port_env("HAFX_M1_SCI_PORT"), port_env("HAFX_M1_DBG_PORT")}},
        {hc::M5, detp{port_env("HAFX_M5_SCI_PORT"), port_env("HAFX_M5_DBG_PORT")}},
        {hc::X1, detp{port_env("HAFX_X1_SCI_PORT"), port_env("HAFX_X1_DBG_PORT")}},
    };

    // Construct service and then give it the right ports and serial numbers
    auto service = std::make_unique<DetectorService>(sock_fd);
    service->put_hafx_serial_nums(sns);
    service->put_hafx_ports(hafx_ports);
    service->put_x123_ports(
        detp{port_env("X123_SCI_PORT"), port_env("X123_DBG_PORT")}
    );

    auto listen = std::make_unique<Listener>(sock_fd, std::move(service));
    listen->listen_loop();

    return 0;
}

void usage(const char* prog) {
    std::cerr << "Usage: run" << prog << "with no arguments.\n"
              << "Expects environment variables to be defined as in main().\n";
}
