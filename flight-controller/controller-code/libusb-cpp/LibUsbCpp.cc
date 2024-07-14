#include <sstream>

#include "LibUsbCpp.hh"

namespace LibUsbCpp {

Context::Context() {
    log_info("Constructing LibUsb::Context");
    int ret = libusb_init(&handle);
    if (ret < 0) {
        std::stringstream error_message;
        error_message << "Failed to initialize USB context: " << libusb_strerror(ret);
        throw UsbException(error_message.str());
    }

    // Maybe this shouldn't go here, but it's fine unless we need to make bigger changes to this.
    ret = libusb_set_option(handle, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
    if (ret < 0) {
        std::stringstream ss;
        ss << "Error setting log level: " << libusb_strerror(ret) << std::endl;
        throw UsbException(ss.str());
    }
}
Context::~Context() {
    log_info("Destructing LibUsb::Context");
    if (handle) {
        libusb_exit(handle);
    }
}

DeviceList::DeviceList(std::shared_ptr<Context> ctx_)
    : ctx(ctx_) {
    log_info("Constructing LibUsb::DeviceList");
    num_devices = libusb_get_device_list(ctx->handle, &devices);
    if (num_devices < 0) {
        std::stringstream ss;
        ss << "Error getting device list: " << libusb_strerror(num_devices) << std::endl;
        throw UsbException(ss.str());
    }
}
DeviceList::~DeviceList() {
    log_info("Destructing LibUsb::DeviceList");
    if (devices) {
        libusb_free_device_list(devices, 1);
    }
}

DeviceHandle::DeviceHandle(libusb_device *device, std::shared_ptr<Context> ctx_)
  : ctx(ctx_) {
    log_info("Constructing LibUsb::DeviceHandle");
    int ret = libusb_open(device, &handle);
    if (ret < 0) {
        std::stringstream ss;
        ss << "Failed to open device: " << libusb_strerror(ret);
        throw UsbException(ss.str());
    }

    // detach/retach kernel driver from interfaces we claim
    ret = libusb_set_auto_detach_kernel_driver(handle, 1);
    if (ret < 0) {
        std::stringstream ss;
        ss << "couldn't auto-detach kernel driver: " << libusb_strerror(ret);
        throw UsbException(ss.str());
    }
}

DeviceHandle::DeviceHandle(libusb_device_handle *handle_, std::shared_ptr<Context> ctx_)
  : handle(handle_)
  , ctx(ctx_) {
    log_info("Constructing LibUsb::DeviceHandle from existing handle");
    if (handle == nullptr) {
        throw UsbException{"Existing handle is null (not connected?)"};
    }
    // detach/retach kernel driver from interfaces we claim
    auto ret = libusb_set_auto_detach_kernel_driver(handle, 1);
    if (ret < 0) {
        std::stringstream ss;
        ss << "couldn't auto-detach kernel driver: " << libusb_strerror(ret);
        throw UsbException(ss.str());
    }
}

DeviceHandle::~DeviceHandle() {
    log_info("Destructing LibUsb::DeviceHandle");
    if (handle) {
        for (auto interface : claimed_interfaces) {
            libusb_release_interface(handle, interface);
        }
        libusb_close(handle);
    }
}

bool DeviceHandle::claim_interface(int interface) {
    int ret = libusb_claim_interface(handle, interface);
    if (ret == LIBUSB_ERROR_BUSY) {
        log_debug("device busy; it's claimed elsewhere");
        return false;
    }
    else if (ret < 0) {
        std::stringstream ss;
        ss << "couldn't claim interface: " << libusb_strerror(ret);
        throw UsbException(ss.str());
    }
    claimed_interfaces.push_back(interface);
    return true;
}

} // namespace LibUsb
