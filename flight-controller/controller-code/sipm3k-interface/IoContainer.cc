#include "IoContainer.hh"
#include <cstring>

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
    
    bool FpgaResults::nrl_buffer_full(uint8_t buf_num) const {
        // Bit 1 (2) indicates if buffer 0 is full
        // Bit 3 (8) indicates if buffer 1 is full
        uint8_t mask = (buf_num == 0)? 2 : 8;
        bool is_full = static_cast<bool>(registers[2] & mask);
        return is_full;
    }

    std::vector<ListModeDataPoint> FpgaListMode::parse_list_buffer() const {
        std::vector<ListModeDataPoint> ret;
        uint16_t detail_reg = registers[0];
        // as per Bridgeport, first 12 bits hold # of events
        size_t num_events = detail_reg & 0xfff;

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

    std::vector<NrlListDataPoint> FpgaLmNrl1::decode() const {
        // size per event: 6 (x2 bytes)
        constexpr size_t EVT_SIZE = 6;
        uint16_t num_evts = registers[0] & 0xfff;

        NrlListDataPoint cur_data;
        std::vector<NrlListDataPoint> ret;
        // Skip the first event because it's special
        for (size_t i = EVT_SIZE; i < (EVT_SIZE * num_evts); i += EVT_SIZE) {
            std::memset(&cur_data, 0, sizeof(cur_data));
            // Memcpy the buffer into our bit field struct
            std::memcpy(
                &cur_data,
                registers.data() + i,
                sizeof(cur_data)
            );
            // We memset the cur_data at each iteration
            // so moving it is fine to do
            ret.push_back(std::move(cur_data));
        }

        return ret;
    }
}
