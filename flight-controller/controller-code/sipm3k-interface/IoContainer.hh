#pragma once

#include <array>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>
#include <utility>
#include <cstdint>

/*
Note: we need function implementations inside of this header so that
the linker knows what's going on.

See: https://stackoverflow.com/questions/8752837/undefined-reference-to-template-class-constructor
*/

// C++17 required for static constexpr inside of a class
#if __cplusplus < 201703L
    #error "at least -std=c++17 is required to properly compile SipmUsb::IoContainer"
#endif

namespace SipmUsb
{
    static const uint8_t FPGA_WRITE_TYPE = 1;
    static const uint8_t FPGA_READ_TYPE = 2;
    static const uint8_t ARM_WRITE_TYPE = 3;
    static const uint8_t ARM_READ_TYPE = 4;
    // ripped from mca3k_device.py
    // numbers that tell the detector what command it will receive
    struct McaUsbFlags
    {
        uint8_t read_type;
        uint8_t write_type;
        uint8_t command_ident;
    };

    // registers can be differend widths (uint16_t, uint32_t, for example)
    // we want to returnn different data based on what kind of command we use (List, Histogram, etc.)
    template<typename RegT, size_t NumRegs>
    struct IoContainer {
        using Registers = std::array<RegT, NumRegs>;
        Registers registers;
    };

    template<typename RegT, size_t NumRegs, uint8_t command_ident>
    struct ArmIoContainer : public IoContainer<RegT, NumRegs> {
        static constexpr McaUsbFlags mca_flags {
            ARM_READ_TYPE,
            ARM_WRITE_TYPE,
            command_ident,
        };
    };

    template<typename RegT, size_t NumRegs, uint8_t command_ident>
    struct FpgaIoContainer : public IoContainer<RegT, NumRegs> {
        static constexpr McaUsbFlags mca_flags {
            FPGA_READ_TYPE,
            FPGA_WRITE_TYPE,
            command_ident,
        };
    };

    struct ArmVersion : public ArmIoContainer<uint8_t, 64, 0> {
        std::string decode_serial_number() const;
    };

    using ArmStatus = ArmIoContainer<float, 7, 1>;

    /* Bridgeport website says arm_ctrl has 12 registers.
     * their code says otherwise.
     * . . .
     * i'm gonna leave it as 12 registers and see if it breaks anything.
     * w setterberg 19 august 2021
     */
    /* need it as 16 i guess (21 august 2021) */
    // as of 2023 it is 64 registers. weird
    // LEAVE IT AS 16!!!!!
    using ArmCtrl = ArmIoContainer<float, 64, 2>;

    using ArmCal = ArmIoContainer<float, 64, 3>;

    using FpgaCtrl = FpgaIoContainer<uint16_t, 16, 0>;

    using FpgaStatistics = FpgaIoContainer<uint32_t, 16, 1>;

    struct FpgaResults : public FpgaIoContainer<uint16_t, 16, 2> {
        bool trace_done() const;
        uint16_t num_avail_time_slices() const;
    };

    using FpgaHistogram = FpgaIoContainer<uint32_t, 4096, 3>;

    using FpgaOscilloscopeTrace = FpgaIoContainer<int16_t, 1024, 4>;

    struct ListModeDataPoint {
        uint32_t rel_ts_clock_cycles;
        uint16_t energy_bin;
    };
    struct FpgaListMode : public FpgaIoContainer<uint16_t, 1024, 5> {
        static constexpr float CLOCK_SPEED = 40e6;
        std::vector<ListModeDataPoint> parse_list_buffer() const;
    };

    using FpgaWeights = FpgaIoContainer<uint16_t, 16, 6>;

    using FpgaAction = FpgaIoContainer<uint16_t, 4, 7>;
    static constexpr FpgaAction FPGA_ACTION_START_NEW_LIST_ACQUISITION = {
        // bits mean:
        0b1111, // clear everything
        0,      // unused
        0b0100, // enable list mode
        0       // unused
    };
    static constexpr FpgaAction FPGA_ACTION_START_NEW_HISTOGRAM_ACQUISITION = {
        // bits mean:
        0b1111, // clear everything
        0,      // unused
        0b0001, // enable histogram mode
        0       // unused
    };

    static constexpr FpgaAction FPGA_ACTION_START_NEW_TRACE_ACQUISITION = {
        // bits mean:
        0b0100, // clear trace
        0,      // unused
        0b0010, // enable trace mode
        0       // unused
    };

    struct DecodedTimeSlice {
        uint16_t buffer_number;              // no unit
        uint16_t num_evts;                   // no unit
        uint16_t num_triggers;               // no unit
        uint16_t dead_time;                  // 800ns ticks
        uint16_t anode_current;              // 25 nA ticks
        std::array<uint16_t, 123> histogram; // no unit
    };
    struct FpgaTimeSlice : public FpgaIoContainer<uint16_t, 128, 8> {
        DecodedTimeSlice decode() const;
    };

    using FpgaMap = FpgaIoContainer<uint16_t, 2048, 8>;
}
