#pragma once

#include <packets/BasePacket.hh>
#include <iostream>

namespace X123Driver { namespace Packets {
namespace Responses {

static constexpr size_t MAX_READBACK_SIZE = 32767;
class TextConfigurationReadback : public BasePacket {
public:
    TextConfigurationReadback() :
        BasePacket({0x82, 0x07}, MAX_READBACK_SIZE)
    { }

    void parsedFromTransfer() override {
    // shrink this size
        const auto sz = dataSizeFromTransfer();
        parsed.resize(sz);
        transfer.resize(sz + additionalTransferBytes());

        BasePacket::parsedFromTransfer();

        // resize back to default buffers after
        parsed.resize(MAX_READBACK_SIZE);
        transfer.resize(MAX_READBACK_SIZE);
    }
};

} } }
