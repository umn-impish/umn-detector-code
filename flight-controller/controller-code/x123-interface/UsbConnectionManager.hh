#pragma once

#include <LibUsbCpp.hh>
#include <packets/BasePacket.hh>

namespace X123Driver
{

class UsbConnectionManager
{
	public:
		UsbConnectionManager();

		void sendAndReceive(Packets::BasePacket& out, Packets::BasePacket& in);

		static const int AMPTEK_DETECTOR_INTERFACE = 0;
	private:

		// these should always be used as a pair. so they're private
		void send(Packets::BasePacket& p);
		void receive(Packets::BasePacket& p);

		void connect();

		std::shared_ptr<LibUsbCpp::DeviceHandle> device_handle;

		// long enough for diagnostic packet
		static const int USB_TIMEOUT_MS = 5000;
};

}
