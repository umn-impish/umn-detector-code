#include <packets/responses/Ack.hh>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace X123Driver { namespace Packets {

namespace Responses
{
Ack::Ack() : BasePacket({0xFF, 0x00}, 0ULL)
{ }

Ack::Ack(const AckError& e) :
	BasePacket(e.pid(), e.transferBuffer().size() - HEADER_SZ - CHECKSUM_SZ)
{
	std::memcpy(
		transfer.data(),
		e.transferBuffer().data(),
		e.transferBuffer().size()
	);
}

Ack::~Ack()
{ }

const char* Ack::issue()
{ return DECODE_PID2.at(pid.second); }

} } }
