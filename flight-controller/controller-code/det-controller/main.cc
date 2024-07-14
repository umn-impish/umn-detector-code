#include <main.hh>

// Get a port (unsigned short) from a string environment variable
auto port_env = [](auto envar_name) {
    return static_cast<unsigned short>(
        std::atoi(std::getenv(envar_name))
    );
};

int main(int argc, char* argv[]) {
    if (argc != 1) {
        usage(argv[0]);
        return -1;
    }

    // Get a file descriptor to the socket
    // which will listen to UDP messages
    int sock_fd = make_listen_socket();

    // Set up the DetectorService object with the proper
    // environment variables
    auto service = setup_service(sock_fd);

    // Give the listen socket and the service (logic) to the listener
    auto listen = std::make_unique<Listener>(sock_fd, std::move(service));
    listen->listen_loop();

    return 0;
}

int make_listen_socket() {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd < 0) {
	    throw std::runtime_error{"cant create socket:" + std::string{strerror(errno)}};
    }

    sockaddr_in info;
    std::memset(&info, 0, sizeof(sockaddr_in));
    info.sin_family = AF_INET;
    info.sin_port = htons(port_env("DET_SERVICE_PORT"));
    info.sin_addr = {.s_addr = INADDR_ANY};

    int ret = bind(sock_fd, (sockaddr*)&info, sizeof(info));
    if (ret < 0) {
	    throw std::runtime_error{"cant bind socket:" + std::string{strerror(errno)}};
    }

    return sock_fd;
}

std::unique_ptr<DetectorService>
setup_service(int sock_fd) {
    using hc = DetectorMessages::HafxChannel;

    // Serial numbers for HaFX detectors
    using SerialMap = std::unordered_map<DetectorMessages::HafxChannel, std::string>;
    const SerialMap sns {
        {hc::C1, std::getenv("HAFX_C1_SERIAL")},
        {hc::M1, std::getenv("HAFX_M1_SERIAL")},
        {hc::M5, std::getenv("HAFX_M5_SERIAL")},
        {hc::X1, std::getenv("HAFX_X1_SERIAL")},
    };

    // Ports used to log data via UDP packets
    using detp = Detector::DetectorPorts;
    using PortMap = std::unordered_map<hc, detp>;
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

    return service;
}

void usage(const char* prog) {
    std::cerr << "Usage: run" << prog << "with no arguments.\n"
              << "Expects environment variables to be defined as in main().\n";
}
