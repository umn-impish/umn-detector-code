/*
 * Collect an oscilloscope trace from a selected SiPM-3k detector, and output it in a nice format
*/
#include <sys/ioctl.h>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <algorithm>
#include <HafxControl.hh>
#include <thread>
#include <chrono>

#include "common.hh"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout
        << "Usage: " << argv[0] << " [channel]"
        << std::endl
        << "Take an oscilloscope trace from the given channel. Reads serial number from envars."
        << std::endl;
        return 1;
    }

    auto usb_man = usb_from_channel_sn(argv[1]);
 
    // The HafxControl sends out data via UDP sockets,
    // so we specify some ports here for that purpose.
    Detector::DetectorPorts dp {.science = 12000, .debug = 12001};
    auto hc = std::make_shared<Detector::HafxControl>(usb_man, dp);

    int socket_fd = bind_socket(dp.debug);

    hc->restart_trace();
    hc->read_save_debug<SipmUsb::FpgaOscilloscopeTrace>();
    auto trace = receive_hafx_debug<SipmUsb::FpgaOscilloscopeTrace>(socket_fd);

    // Output to stdout so we can send to a file or other places if we want
    for (auto count : trace.registers) {
        std::cout << count << ' ';
    }
    std::cout << std::endl;

    return 0;
}
