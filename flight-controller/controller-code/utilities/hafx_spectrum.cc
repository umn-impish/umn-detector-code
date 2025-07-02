/*
 * Collect a spectrum from a selected SiPM-3k detector, and output it in a nice format
*/
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <algorithm>
#include <HafxControl.hh>
#include <thread>
#include <chrono>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout
        << "Usage: " << argv[0] << " [channel] [collection time in seconds]"
        << std::endl
        << "Take a histogram from the given channel. Reads serial number from envars."
        << std::endl;
        return 1;
    }

    // Put the channel in upper case
    std::string channel{argv[1]};
    std::transform(
        channel.begin(), channel.end(), channel.begin(),
        [](auto c) { return std::toupper(c); }
    );

    // Get the device we want
    std::shared_ptr<SipmUsb::UsbManager> usb_man = nullptr;
    {
        std::string serial_number;
        try {
            serial_number = std::string{
                getenv(("HAFX_" + channel + "_SERIAL").c_str())
            };
        } catch (std::logic_error const&) {
            std::cerr
            << "Given channel not set as an envar: "
            << channel << std::endl;
            return 1;
        }
        auto all_devs = SipmUsb::BridgeportDeviceManager{};
        std::cout << "available serials: " << std::endl;
        try {
            for (auto [k, _] : all_devs.device_map) {
                std::cout << k << std::endl;
            }
            usb_man = all_devs.device_map.at(serial_number);
        } catch (std::out_of_range const&) {
            std::cerr << "Serial requested is not connected." << std::endl;
            return 1;
        }
    }
 
    // Set up the hafx control
    Detector::DetectorPorts dp {
        .science = 12000,
        .debug = 12001
    };
    auto hc = std::make_shared<Detector::HafxControl>(
        usb_man,
        dp
    );

    // We need to receive the data via a UDP socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dp.debug);
    addr.sin_addr.s_addr = INADDR_ANY;
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    bind(socket_fd, (const sockaddr*)&addr, sizeof(addr));

    hc->restart_time_slice_or_histogram();
    std::this_thread::sleep_for(
        std::chrono::seconds(std::atoi(argv[2]))
    );
    hc->read_save_debug<SipmUsb::FpgaHistogram>();

    constexpr size_t HISTOGRAM_SIZE = 4096;
    std::array<uint16_t, HISTOGRAM_SIZE> buffer;
    recv(socket_fd, buffer.data(), HISTOGRAM_SIZE * 2, 0);

    for (auto count : buffer) {
        std::cout << count << ' ';
    }
    std::cout << std::endl;

    return 0;
}