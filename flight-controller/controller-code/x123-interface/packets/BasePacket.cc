#include <algorithm>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <packets/BasePacket.hh>

namespace X123Driver { namespace Packets {

BasePacket::BasePacket(const pid_t& p, size_t sz) :
	parsed(sz),
	transfer(sz + BasePacket::additionalTransferBytes()),
	pid(p)
{ }

void BasePacket::transferFromParsed()
{
	transfer.assign(transfer.size(), 0);
	auto dataLength = parsed.size();

	// Amptek packet header
	const uint8_t head[] = {
		SYNC_1,
		SYNC_2,
		pid.first,
		pid.second,
		uint8_t((dataLength & 0xff00) >> 8),
		uint8_t(dataLength & 0xff),
	};
	std::memcpy(transfer.data(), head, HEADER_SZ);

	std::memcpy(transfer.data() + HEADER_SZ, parsed.data(), dataLength);
	int32_t checksum = std::accumulate(transfer.begin(), transfer.end() - CHECKSUM_SZ, uint32_t(0));

	checksum = (0xffff ^ checksum) + 1;
	transfer[dataLength + HEADER_SZ] = static_cast<uint8_t>((checksum >> 8) & 0xff);
	transfer[dataLength + HEADER_SZ + 1] = static_cast<uint8_t>(checksum & 0xff);
}

void BasePacket::parsedFromTransfer()
{
	verify();
	parsed.assign(parsed.size(), 0);
	std::memcpy(parsed.data(), transfer.data() + HEADER_SZ, parsed.size());
}

void BasePacket::verify() const
{
	// Verify the X-123 packet is not malformed.

	// do pid first in case ACK error (?)
	verifyPid();
	verifySync();
	verifyChecksum();
	verifyDataSize();
}

void BasePacket::verifySync() const
{
	if (transfer[0] != SYNC_1 || transfer[1] != SYNC_2)
		throw std::runtime_error("sync error from x-123 raw packet");
}

void BasePacket::verifyDataSize() const
{
	const size_t dataSize = dataSizeFromTransfer();
	if (dataSize != parsed.size()) {
		throw std::runtime_error("x123 packet size incorrect");
    }
}

void BasePacket::verifyPid() const
{
	pid_t test = {transfer[2], transfer[3]};
	if (test == pid) return;

	static const uint8_t ACK_FIRST_PID = 0xff;
	if (test.first == ACK_FIRST_PID) {
		throw AckError(test, transfer);
	}

	std::stringstream ss;
	const auto [p1, p2] = test;
	ss << std::hex
		<< "x123 received incorrect packet" << std::endl
		<< "recv pid1 = " << +p1 << std::endl
		<< "recv pid2 = " << +p2 << std::endl
		<< "expected pid1 = " << +pid.first << std::endl
		<< "expected pid2 = " << +pid.second << std::endl; 
	throw std::runtime_error(ss.str());
}

void BasePacket::verifyChecksum() const
{
	int32_t checksum = std::accumulate(
		transfer.begin(), transfer.end() - CHECKSUM_SZ, uint32_t(0));
    auto sz = transfer.size();
    checksum += transfer[sz - 1] + 256 * transfer[sz - 2];

	if ((checksum & 0xffff) != 0)
		throw std::runtime_error("checksum error in x-123 raw packet");
}

}}

