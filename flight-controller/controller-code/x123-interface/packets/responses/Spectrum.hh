#pragma once

#include <packets/BasePacket.hh>
#include <packets/responses/Status.hh>
#include <CharCast.hh>

namespace X123Driver { namespace Packets {

namespace Responses
{

class BaseSpectrum : public BasePacket
{
    public:
        static const size_t BYTES_PER_BIN = 3;
        size_t num_bins() const { return (parsedBuffer().size() - Status::SIZE) / BYTES_PER_BIN; }

        BaseSpectrum() =delete;
        BaseSpectrum(size_t sz, char pid1, char pid2) :
            BasePacket({pid1, pid2}, BYTES_PER_BIN*sz + Status::SIZE)
        { }
        virtual ~BaseSpectrum()
        { }
};

struct Spectrum256  final : public BaseSpectrum { Spectrum256()  : BaseSpectrum(256,  0x81_ch, 0x02_ch) {} };
struct Spectrum512  final : public BaseSpectrum { Spectrum512()  : BaseSpectrum(512,  0x81_ch, 0x04_ch) {} };
struct Spectrum1024 final : public BaseSpectrum { Spectrum1024() : BaseSpectrum(1024, 0x81_ch, 0x06_ch) {} };
struct Spectrum2048 final : public BaseSpectrum { Spectrum2048() : BaseSpectrum(2048, 0x81_ch, 0x08_ch) {} };
struct Spectrum4096 final : public BaseSpectrum { Spectrum4096() : BaseSpectrum(4096, 0x81_ch, 0x0a_ch) {} };
struct Spectrum8192 final : public BaseSpectrum { Spectrum8192() : BaseSpectrum(8192, 0x81_ch, 0x0c_ch) {} };

}

} }
