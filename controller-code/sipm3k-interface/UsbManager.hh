#pragma once

#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "LibUsbCpp.hh"

namespace SipmUsb {

enum class MemoryType {
    ram = 0,
    nvram = 1,
};

class UsbManager {
public:
    explicit UsbManager(std::shared_ptr<LibUsbCpp::DeviceHandle> device_handle);

    std::string get_arm_serial() const;

    // writes settings or data from the IoContainer
    template<typename ContainerT>
    void write(ContainerT const& con, MemoryType memory_type) {
        // Find out if short write is possible
        static_assert(std::is_trivial_v<ContainerT>);
        static_assert(std::is_trivial_v<CommandBuffer_t>);
        constexpr bool short_write_possible = 4 + (sizeof(ContainerT)) <= sizeof(CommandBuffer_t);

        // Construct command buffer header
        uint32_t nbytes = short_write_possible ? sizeof(CommandBuffer_t) : sizeof(ContainerT);
        uint32_t transfer_flags = ContainerT::mca_flags.write_type;
        if constexpr (short_write_possible) {
            constexpr uint32_t SHORT_WRITE_FLAG = 0x800;
            transfer_flags += SHORT_WRITE_FLAG;
        }
        auto header = command_buffer_header(
            nbytes,
            static_cast<uint32_t>(memory_type),
            ContainerT::mca_flags.command_ident,
            transfer_flags
        );

        // Construct command buffer
        CommandBuffer_t command_buffer;
        memcpy(command_buffer.data(), &header, sizeof(header));
        if constexpr (short_write_possible) {
            memcpy(command_buffer.data() + sizeof(header), &con, sizeof(con));
        }

        // Send command buffer
        int xfer_ret = xfer_in_chunks(
            CMD_OUT_EP, command_buffer.data(),
            sizeof(command_buffer), TIMEOUT_MS);
        if (xfer_ret < 0) {
            std::stringstream ss;
            ss << "Error writing cmd to handle specified by SN "
                << arm_serial << ": "
                << libusb_strerror(xfer_ret);
            throw LibUsbCpp::UsbException(ss.str().c_str());
        }

        // Send additional data, if short write was not possible
        if (!short_write_possible) {
            xfer_ret = xfer_in_chunks(
                // Casting away const for libusb_bulk_transfer!  Must not actually write to &con.
                DATA_OUT_EP, (void*)&con,
                sizeof(con), TIMEOUT_MS);
            if (xfer_ret < 0) {
                std::stringstream ss;
                ss <<
                    "Error writing additional data to handle specified by SN "
                    << arm_serial << ": "
                    << libusb_strerror(xfer_ret);
                throw LibUsbCpp::UsbException(ss.str().c_str());
            }
        }
    }

    // reads data into the IoContainer
    template<typename ContainerT>
    void read(ContainerT &con, MemoryType memory_type) {
        // Construct command buffer
        static_assert(std::is_trivial_v<ContainerT>);
        static_assert(std::is_trivial_v<CommandBuffer_t>);
        auto header = command_buffer_header(
            sizeof(con),
            static_cast<uint32_t>(memory_type),
            ContainerT::mca_flags.command_ident,
            ContainerT::mca_flags.read_type
        );
        CommandBuffer_t command_buffer;
        memcpy(&command_buffer, &header, sizeof(header));

        // send command saying, "hi, i want data"
        int xfer_ret = xfer_in_chunks(
            CMD_OUT_EP, command_buffer.data(),
            sizeof(command_buffer), TIMEOUT_MS);

        if (xfer_ret < 0) {
            std::stringstream ss;
            ss << "Error writing cmd to handle specified by SN "
                << arm_serial << ": "
                << libusb_strerror(xfer_ret);
            throw LibUsbCpp::UsbException(ss.str().c_str());
        }

        // read the actual data now
        xfer_ret = xfer_in_chunks(
            DATA_IN_EP, &con,
            sizeof(con), TIMEOUT_MS);

        if (xfer_ret < 0) {
            std::stringstream ss;
            ss << "Error reading data from handle specified by SN "
                << arm_serial << ": "
                << libusb_strerror(xfer_ret);
            throw LibUsbCpp::UsbException(ss.str().c_str());
        }
    }

private:
    using CommandBuffer_t = std::array<std::byte, 64>;

    uint32_t command_buffer_header(
        uint32_t nbytes,
        uint32_t memory_type,
        uint32_t command_ident,
        uint32_t transfer_flags
    ) {
        return
            (nbytes << 16) +
            (memory_type << 12) +
            (command_ident << 4) +
            transfer_flags;
    }

    std::shared_ptr<LibUsbCpp::DeviceHandle> device_handle;
    std::string arm_serial;

    int xfer_in_chunks(int endpoint, void* buffer, int num_bytes, int timeout);

    static constexpr int TIMEOUT_MS = 1000;

    // from Bridgeport code
    static constexpr int CMD_OUT_EP = 0x01;
    static constexpr int CMD_IN_EP = 0x81;
    static constexpr int DATA_OUT_EP = 0x02;
    static constexpr int DATA_IN_EP = 0x82;
};

struct BridgeportDeviceManager {
    std::map<std::string, std::shared_ptr<UsbManager>> device_map;

    BridgeportDeviceManager();

    static constexpr int DETECTOR_INTERFACE = 1;
    static constexpr uint16_t BRIDGEPORT_VID = 0x1fa4;
};
}
