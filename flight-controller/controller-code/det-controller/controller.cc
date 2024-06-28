#include <controller.hh>

int main(int argc, char* argv[]) {
    if (argc != 8) {
        usage(argv[0]);
        return -1;
    }

    auto sock = std::make_shared<Socket>(atoi(argv[1]));

    using hc = DetectorMessages::HafxChannel;
    // serial numbers of the detectors!
    const SerialMap sns{
        {hc::C1, argv[2]}, {hc::M1, argv[3]},
        {hc::M5, argv[4]}, {hc::X1, argv[5]}
    };

    const Detector::BasePorts p {
        .x123 = static_cast<unsigned short>(atoi(argv[6])),
        .hafx = static_cast<unsigned short>(atoi(argv[7]))
    };

    auto listen = std::make_unique<Listener>(sock, sns, p);
    listen->listen_loop();

    return 0;
}

void usage(const char* prog) {
    std::cerr << "Usage:\n"
        << "    " << prog << " det_port c1_sn m1_sn m5_sn x1_sn x123_port hafx_port\n"
        << "\tdet_port: port for detector controllerto listen on\n"
        << "\tc1_sn: serial number of C1-optimized detector\n"
        << "\tm1_sn: serial number of M1-optimized detector\n"
        << "\tm5_sn: serial number of M5-optimized detector\n"
        << "\tx1_sn: serial number of X1-optimized detector\n"
        << "\tx123_port: base port for x123 data dumping\n"
        << "\thafx_port: base port for HaFX data dumping\n";
}
