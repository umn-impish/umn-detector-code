#pragma once

#include <memory>
#include <vector>

#include <libusb-1.0/libusb.h>

#include "logging.hh"

namespace LibUsbCpp {

class UsbException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// RAII wrapper classes around libusb pointers

struct Context {
    libusb_context *handle = nullptr;

    Context();
    ~Context();
};

struct DeviceList {
    ssize_t num_devices = 0;
    libusb_device** devices = nullptr;

    DeviceList(std::shared_ptr<Context> ctx);
    ~DeviceList();
private:
    std::shared_ptr<Context> ctx;
};

struct DeviceHandle {
    libusb_device_handle *handle = nullptr;

    DeviceHandle(libusb_device *device, std::shared_ptr<Context> ctx);
    DeviceHandle(libusb_device_handle *handle, std::shared_ptr<Context> ctx);
    ~DeviceHandle();

    bool claim_interface(int interface);

private:
    std::vector<int> claimed_interfaces;
    std::shared_ptr<Context> ctx;
};

} // namespace LibUsb
