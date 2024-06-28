#pragma once
#include <packets/BasePacket.hh>

namespace X123Driver { namespace Packets {

namespace Responses {

class Status : public BasePacket
{
	public:
		static const size_t SIZE = 64ULL;
		Status();
		~Status();
};

}

} }
