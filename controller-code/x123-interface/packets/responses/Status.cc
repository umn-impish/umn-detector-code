#include <packets/responses/Status.hh>

namespace X123Driver { namespace Packets {

namespace Responses
{
Status::Status() :
	BasePacket({0x80, 0x01}, SIZE)
{ }

Status::~Status() { }
}
} }
