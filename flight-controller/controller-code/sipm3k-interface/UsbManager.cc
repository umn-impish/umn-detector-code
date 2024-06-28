#include <iostream>
#include <sstream>

#include "IoContainer.hh"
#include "logging.hh"
#include "UsbManager.hh"

namespace SipmUsb
{

UsbManager::UsbManager(std::shared_ptr<LibUsbCpp::DeviceHandle> device_handle_) :
    device_handle(device_handle_),
    arm_serial("Unknown serial number")
{
    ArmVersion armvc;
    read(armvc, MemoryType::ram);
    arm_serial = armvc.decode_serial_number();
}

std::string UsbManager::get_arm_serial() const {
    return arm_serial;
}

int UsbManager::xfer_in_chunks(int endpoint, void* raw_buffer, int num_bytes, int timeout)
{
    // wow
    unsigned char* buffer = (unsigned char*) raw_buffer; 
    libusb_device_handle* han = device_handle->handle;

    // ARM processor inside detector only has 256-byte buffer
    static const int CHUNK_SZ = 256;
    int nchunks = num_bytes / CHUNK_SZ;
    int leftover = num_bytes % CHUNK_SZ;
    int ret = 0;
    int transferred = 0;

    // transfer the 256 byte chunks
    for (int i = 0; i < nchunks; ++i) {
        ret = libusb_bulk_transfer(han, endpoint, buffer + i*CHUNK_SZ, CHUNK_SZ, &transferred, timeout);
        if (transferred != CHUNK_SZ) {
            std::stringstream ss;
            ss << "didn't read/write appropriate chunk size: " << transferred << " vs " << CHUNK_SZ << ". " << libusb_strerror(ret);
            log_error(ss.str());
        }
        if (ret < 0) return ret;
    }

    transferred = 0;
    // transfer the rest
    if (leftover != 0) {
        ret = libusb_bulk_transfer(han, endpoint, buffer + nchunks*CHUNK_SZ, leftover, &transferred, timeout);
        if (transferred != leftover) {
            std::stringstream ss;
            ss << "didn't read/write appropriate leftover: " << transferred << " vs " << leftover << ". " << libusb_strerror(ret);
            log_error(ss.str());
        }
        if (ret < 0) return ret;
    }
    return 0;
}


BridgeportDeviceManager::BridgeportDeviceManager() {
    auto usb_ctx = std::make_shared<LibUsbCpp::Context>();
    auto device_list = LibUsbCpp::DeviceList(usb_ctx);

    for (ssize_t i = 0; i < device_list.num_devices; ++i) {
        libusb_device* dev = device_list.devices[i];
        libusb_device_descriptor desc;

        int ret = libusb_get_device_descriptor(dev, &desc);
        if (ret < 0) {
            std::stringstream ss;
            ss << "Error getting device descriptor: " << libusb_strerror(ret);
            throw LibUsbCpp::UsbException(ss.str());
        }

        if (desc.idVendor == BRIDGEPORT_VID) {
            auto device_handle = std::make_shared<LibUsbCpp::DeviceHandle>(dev, usb_ctx);
            if (!device_handle->claim_interface(DETECTOR_INTERFACE)) {
                return;
            }

            auto usb_manager = std::make_shared<UsbManager>(device_handle);
            device_map[usb_manager->get_arm_serial()] = usb_manager;
        }
    }
}

}
