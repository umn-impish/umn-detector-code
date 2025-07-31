#include <DetectorMessages.hh>

namespace DetectorMessages {

// Mark "indicator" entries as not present by default
X123Settings::X123Settings() :
    ack_err_retries_present{false},
    ascii_settings_length{0},
    adc_rebin_edges_length{0}
{ }

};
