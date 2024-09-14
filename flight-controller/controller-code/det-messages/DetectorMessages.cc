#include <DetectorMessages.hh>

namespace DetectorMessages {

// Mark "indicator" entries as not present by default
X123Settings::X123Settings() :
    ack_err_retries_present{false},
    ascii_settings_length{0},
    adc_rebin_edges_length{0}
{ }

StrippedNrlDataPoint
StrippedNrlDataPoint::from(const SipmUsb::NrlListDataPoint& orig) {
    /*
     * Strip down the NrlListDataPoint into a much smaller type.
     * */

    // Scale the LSB of the data by the DIVISOR
    // Make them constexpr so they are genuine constants,
    // but easier to change if we want to down the road
    static constexpr uint64_t ORIG_DIV_NS = 25;
    static constexpr uint64_t GOAL_DIV_NS = 200;
    static_assert(GOAL_DIV_NS % ORIG_DIV_NS == 0,
                  "Goal NRL time division must evenly divide by original time steps");
    static constexpr uint64_t DIVISOR = GOAL_DIV_NS / ORIG_DIV_NS;

    return {
        // Convert to the time step specified above
        .wall_clock = static_cast<uint32_t>((orig.wall_clock_time / DIVISOR)) & 0x1ffffff,
        // Drop the bottom 12 bits of precision because EXACT
        // doesn't really care about energy
        .energy = static_cast<uint32_t>((orig.energy >> 12ULL)) & 0xf,
        .was_pps = static_cast<uint8_t>(orig.was_pps),
        .piled_up = static_cast<uint8_t>(orig.piled_up),
        .out_of_range = static_cast<uint8_t>(orig.out_of_range)
    };
}
};
