#pragma once

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <type_traits>

#include <HafxControl.hh>

/*
 * Given a channel identifier, load in the correct UsbManager using its envar serial number.
 */
std::shared_ptr<SipmUsb::UsbManager> usb_from_channel_sn(std::string const& orig_channel) {
    // The channel has to be in upper case, to match
    // envar convention
    std::string channel{orig_channel};
    std::transform(
        channel.begin(), channel.end(), channel.begin(),
        [](auto c) { return std::toupper(c); }
    );

    auto serial_number = std::string{};
    try {
        serial_number = std::string{
            getenv(("HAFX_" + channel + "_SERIAL").c_str())
        };
    } catch (std::logic_error const&) {
        std::cerr
        << "Given channel not set as an envar: "
        << channel << '.' << std::endl
        << "There is no variable called HAFX_" << channel << "_SERIAL." << std::endl;
        std::cerr.flush();
        std::exit(EXIT_FAILURE);
    }

    auto all_devs = SipmUsb::BridgeportDeviceManager{};
    try {
        return all_devs.device_map.at(serial_number);
    } catch (std::out_of_range const&) {
        std::cerr
        << "Serial requested is not connected." << std::endl
        << "available serials: " << std::endl;
        for (auto [k, _] : all_devs.device_map) {
            std::cout << k << std::endl;
        }
        std::cerr.flush();
        std::exit(EXIT_FAILURE);
    }

    // Code should never reach this point
    std::abort();
}

/*
 * Bind a UDP socket to a given port for receiving data from a Control object.
 */
int bind_socket(unsigned short port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    bind(socket_fd, (const sockaddr*)&addr, sizeof(addr));
    return socket_fd;
}

/*
 * Receive a debug packet specified in the template argument
 */
template <typename T> T receive_hafx_debug(int socket_fd) {
    T ret{};
    // We need enough for (debug tag) + (bytes in buffer)
    std::vector<uint8_t> buffer(sizeof(ret.registers[0]) * ret.registers.size() + 1, 0);
    recv(
        socket_fd,
        buffer.data(),
        buffer.size(),
        0
    );
    // Cut out the debug tag from the buffer when copying into the histogram array
    std::memcpy(ret.registers.data(), buffer.data() + 1, buffer.size() - 1);
    return ret;
}
