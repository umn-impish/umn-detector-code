#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>

namespace X123Driver { namespace Packets {

class BasePacket {
	public:
		using buff_t = std::vector<uint8_t>;
		using pid_t = std::pair<uint8_t, uint8_t>;
		static constexpr size_t HEADER_SZ = 6;
		static constexpr size_t CHECKSUM_SZ = 2;

        static constexpr size_t additionalTransferBytes()
        { return HEADER_SZ + CHECKSUM_SZ; }


		BasePacket() =delete;
		BasePacket(const pid_t& pid, size_t sz);
		~BasePacket()
		{ }

		uint8_t* transferPointer()
		{ return transfer.data(); }
		size_t transferSize() const
		{ return transfer.size(); }
		const buff_t& parsedBuffer() const
		{ return parsed; }
		const buff_t& transferBuffer() const
		{ return transfer; }

		virtual void parsedFromTransfer();
		virtual void transferFromParsed();

		// Amptek stream synchronization bytes
		static const uint8_t SYNC_1 = 0xF5;
		static const uint8_t SYNC_2 = 0xFA;

		// need this to handle bad "ack"
		void updateTransfer(const buff_t& new_buf) {
			transfer = new_buf;
		}
	protected:
		buff_t parsed;
		buff_t transfer;
		pid_t pid;

		void verify() const;
		void verifyPid() const;
		void verifySync() const;
		void verifyDataSize() const;
		void verifyChecksum() const;

        size_t dataSizeFromTransfer() const
        { return (static_cast<size_t>(transfer[4]) << 8) | static_cast<size_t>(transfer[5]); }
};

class AckError : public std::exception
{
	public:
		AckError() =delete;
		AckError(
			const BasePacket::pid_t& pid,
			const BasePacket::buff_t& transferBuffer) :
				std::exception(),
				pid_(pid),
				transferBuffer_(transferBuffer)
		{ }
	
	const BasePacket::pid_t&
	pid() const { return pid_; }
	const BasePacket::buff_t&
	transferBuffer() const { return transferBuffer_; }

	private:
		BasePacket::pid_t pid_;
		BasePacket::buff_t transferBuffer_;
};

} }

