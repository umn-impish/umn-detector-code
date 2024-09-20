#include "IoContainer.hh"

namespace SipmUsb
{
    std::string ArmVersion::decode_serial_number() const {
        std::stringstream ss;
        // location of serial number
        for (size_t i = 8; i < 24; ++i) {
            ss << std::hex << std::setw(2)
               << std::setfill('0') << std::uppercase
               << (0xff & registers[i]);
        }

        return ss.str();
    }

    bool FpgaResults::trace_done() const {
        auto res = registers[2];
        return static_cast<bool>(res & 4);
    }

    uint16_t FpgaResults::num_avail_time_slices() const {
        // from Mike's code (sorry about the magic numbers)
        return (registers[2] >> 9) & 0x7f;
    }

    std::vector<ListModeDataPoint> FpgaListMode::parse_list_buffer() const {
        std::vector<ListModeDataPoint> ret;
        uint16_t detail_reg = registers[0];
        // as per Bridgeport, first 12 bits hold # of events
        size_t num_events = detail_reg & 0xfff;
        // mode is the topmost bit
        uint8_t lm_mode = detail_reg >> 15;
        if (lm_mode == 1) { throw std::runtime_error("lm_mode must be set to zero for the higher time precision"); }

        // each list mode event is 3 uint16_t long. they start at index 4.
        size_t max_idx = 4 + 3*num_events;
        uint32_t ts_clock_cycles;
        uint16_t energy;

        for (size_t i = 4; i < max_idx; i += 3) {
            ts_clock_cycles = registers[i+1] | (registers[i+2] << 16);
            // throw away extra energy precision . . . for now
            energy = registers[i] / 16;
            ret.push_back({ts_clock_cycles, energy});
        }

        return ret;
    }

    DecodedTimeSlice FpgaTimeSlice::decode() const {
        DecodedTimeSlice ret{
            .buffer_number = registers[0],
            .num_evts = registers[1],
            .num_triggers = registers[2],
            .dead_time = registers[3],
            .anode_current = registers[4],
            .histogram = {0}
        };

        for (size_t i = 0; i < ret.histogram.size(); ++i) {
            ret.histogram[i] = registers[i+5];
        }

        return ret;
    }
}
