#include <iostream>
#include <stdexcept>

#include <UsbConnectionManager.hh>
#include <packets/BasePacket.hh>

namespace X123Driver {

UsbConnectionManager::UsbConnectionManager() {
	connect();
}

void UsbConnectionManager::connect()
{
	auto ctx = std::make_shared<LibUsbCpp::Context>();

	static const int AMPTEK_PRODUCT_ID = 0x842a;
	static const int AMPTEK_VENDOR_ID = 0x10c4;
	auto handle = libusb_open_device_with_vid_pid(ctx->handle, AMPTEK_VENDOR_ID, AMPTEK_PRODUCT_ID);

	device_handle = std::make_shared<LibUsbCpp::DeviceHandle>(handle, ctx);
	device_handle->claim_interface(AMPTEK_DETECTOR_INTERFACE);
}

void UsbConnectionManager::send(Packets::BasePacket& p)
{
	// prepare to send
	p.transferFromParsed();
	static const uint8_t BULK_OUT_ENDPOINT = 0x02;
	int rc = libusb_bulk_transfer(
		device_handle->handle,
		BULK_OUT_ENDPOINT,
		p.transferPointer(),
		p.transferSize(),
		nullptr,
		USB_TIMEOUT_MS
	);

	if (rc != 0) {
		std::string err = libusb_strerror(rc);
		throw LibUsbCpp::UsbException{"error writing to x-123: " + err};
	}
}

void UsbConnectionManager::receive(Packets::BasePacket& p)
{
	static const uint8_t BULK_IN_ENDPOINT = 0x81;
	constexpr size_t MAX_SIZE = 32800;
	std::vector<uint8_t> transfer(MAX_SIZE);
	int transferred{0};
	int rc = libusb_bulk_transfer(
		device_handle->handle,
		BULK_IN_ENDPOINT,
		transfer.data(),
		transfer.size(),
		&transferred,
		USB_TIMEOUT_MS
	);
	if (rc != 0) {
		std::string err = libusb_strerror(rc);
		throw LibUsbCpp::UsbException{"error reading from x-123: " + err};
	}

	transfer.resize(transferred);
	p.updateTransfer(transfer);

	// decode after recv
	p.parsedFromTransfer();
}

void UsbConnectionManager::sendAndReceive(Packets::BasePacket& out, Packets::BasePacket& in)
{
	send(out);
	receive(in);
}

}
