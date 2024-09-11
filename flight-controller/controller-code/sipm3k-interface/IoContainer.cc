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

    uint16_t FpgaResults::trace_done() const {
        // trace_done in bit 2 of results register
        return (registers[2] >> 2) & 0x1;
    }

    uint16_t FpgaResults::num_avail_time_slices() const {
        // from Mike's code (sorry about the magic numbers)
        return (registers[2] >> 9) & 0x7f;
    }
    
    uint16_t FpgaResults::full_0() const {
        // from BPI code, full_0 is in results register ( registers[2] ) bit 1 (2^1 = 2)
        std::cerr << registers[2] << std::endl;
        return (registers[2] & 2);
    }
    
    uint16_t FpgaResults::full_1() const {
        // from BPI code, full_1 is in results register ( registers[2] ) bit 3 (2^3 = 8)
        std::cerr << registers[2] << std::endl;
        return (registers[2] & 8);
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

    DecodedListBuffer FpgaLmNrl1::decode() const {
        // this is not complete, need to figure out how to implment that magic python method

        uint32_t number_events = registers[0] & 0xFFF;
        uint32_t E0 = number_events * 6; // idk why mike calls it this, but it is the length length of the registers array in uint16_t's

        std::array<uint16_t, 2048> psd_cont; // containers for ret values
        std::array<uint16_t, 2048> energy_cont;
        std::array<uint16_t, 2048> wc0_cont;
        std::array<uint16_t, 2048> wc1_cont;
        std::array<uint16_t, 2048> wc2_cont;
        std::array<uint16_t, 2048> wc3af_cont;

        for (size_t i = 6; i < E0; i += 6) {
            psd_cont[i / 6] = registers[i];
        }
        for (size_t i = 7; i < E0; i += 6) {
            energy_cont[i / 6] = registers[i]; // '/' always floor divides
        }
        for (size_t i = 8; i < E0; i += 6) {
            wc0_cont[i / 6] = registers[i];
        }
        for (size_t i = 9; i < E0; i += 6) {
            wc1_cont[i / 6] = registers[i];
        }
        for (size_t i = 10; i < E0; i += 6) {
            wc2_cont[i / 6] = registers[i];
        }
        for (size_t i = 11; i < E0; i += 6) {
            wc3af_cont[i / 6] = registers[i]; // the last 8 bits will be 0s no matter what (padding!) bits 0-2 are wc3 and 3-7 are flags
        }

        DecodedListBuffer ret{
            psd_cont,
            energy_cont,
            wc0_cont,
            wc1_cont,
            wc2_cont,
            wc3af_cont
        };

        return ret;
    }
}
