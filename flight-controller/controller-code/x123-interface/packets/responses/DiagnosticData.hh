#pragma once

#include <packets/BasePacket.hh>

namespace X123Driver { namespace Packets {

namespace Responses
{
class DiagnosticData : public BasePacket
{
    public:
        DiagnosticData() : BasePacket({0x82, 0x05}, 256ULL)
        { }
        ~DiagnosticData()
        { }
};
}
} }
