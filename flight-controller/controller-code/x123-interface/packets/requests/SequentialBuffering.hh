#pragma once

#include <packets/BasePacket.hh>
#include <CharCast.hh>

namespace X123Driver { namespace Packets {

namespace Requests
{

template <char pid1, char pid2>
class SequentialBufferBase : public BasePacket
{
    public:
        SequentialBufferBase(uint16_t n) :
            BasePacket({pid1, pid2}, 2ULL),
            sequentialBufferNumber{n}
        { }
        SequentialBufferBase() : 
            SequentialBufferBase(0)
        { }
        ~SequentialBufferBase()
        { }
        void updateSequentialBufferNumber(uint16_t newNumber)
        { sequentialBufferNumber = newNumber; }
        virtual void transferFromParsed() override
        {
            parsed[1] = sequentialBufferNumber & 0xff;
            parsed[0] = (sequentialBufferNumber >> 8) & 0xff;

            BasePacket::transferFromParsed();
        }

    private:
        uint16_t sequentialBufferNumber;
};

using RequestBuffer = SequentialBufferBase<2_ch, 7_ch>;
using BufferSpectrum = SequentialBufferBase<2_ch, 5_ch>;
using BufferAndClearSpectrum = SequentialBufferBase<2_ch, 6_ch>;

}

}}
