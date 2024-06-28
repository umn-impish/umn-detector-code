#pragma once

#include <array>
#include <packets/BasePacket.hh>

namespace X123Driver { namespace Packets {

namespace Responses
{
class Ack : public BasePacket
{
    public:
        Ack();
        ~Ack();
        Ack(const AckError& e);

        const char* issue();

        // index of element = pid2 value
        static constexpr std::array<const char*, 18>
        DECODE_PID2 = {
            "OK",
            "Sync error",
            "PID error",
            "LEN error",
            "Checksum error",
            "Bad parameter",
            "Bad hex record (structure/chksum)",
            "Unrecognized command",
            "FPGA error (not initialized)",
            "CP2201 not found",
            "Scope data not available (not triggered)",
            "PC5 not present",
            "OK + Interface sharing request",
            "Busy - another interface is in use",
            "I2C error",
            "DO NOT USE OK + FPGA upload address",
            "Feature not supported by this FPGA version",
            "Calibration data not present",
        };
};
}

} }
