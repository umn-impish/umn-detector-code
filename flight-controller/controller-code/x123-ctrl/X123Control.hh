#pragma once
#include <memory>

#include "packets/BasePacket.hh"
#include "packets/requests/TextConfiguration.hh"
#include "packets/requests/ZeroLengthPackets.hh"
#include "packets/requests/SequentialBuffering.hh"

#include "packets/responses/Ack.hh"
#include "packets/responses/DiagnosticData.hh"
#include "packets/responses/Spectrum.hh"
#include "packets/responses/Status.hh"
#include "packets/responses/TextConfigurationReadback.hh"
#include "packets/requests/TextConfiguration.hh"
#include "packets/requests/ZeroLengthPackets.hh"
#include "packets/requests/SequentialBuffering.hh"

#include "DetectorSupport.hh"
#include "DetectorMessages.hh"
#include "X123DriverWrap.hh"

#include <logging.hh>

namespace Detector {

class X123Control {
public:
    X123Control(DetectorPorts ports);
    ~X123Control();

    DetectorMessages::X123Health
    generate_health();

    void read_save_sequential_buffer();
    void restart_hardware_controlled_sequential_buffering();
    void stop_sequential_buffering();

    void update_settings(DetectorMessages::X123Settings const& new_settings);
    DetectorMessages::X123Settings fetch_settings();

    void poll_save_debug(const DetectorMessages::X123Debug& debug);

    void data_time_anchor(time_t new_anchor);
    time_t data_time_anchor();

    void read_save_debug_diagnostic();
    void init_debug_histogram();
    void read_save_debug_histogram();
    void read_save_debug_ascii(const std::string& ascii_query);

    bool driver_valid() const;

private:
    std::unique_ptr<X123DriverWrap> driver;
    uint16_t local_next_buffer_num;
    time_t time_anchor;
    uint16_t num_histogram_bins;
    std::unique_ptr<DataSaver> science_saver;
    std::unique_ptr<DataSaver> debug_saver;

    std::unique_ptr<SettingsSaver> settings_saver;
    DetectorMessages::X123Settings settings;

    void increment_reset_buffering(std::vector<uint8_t> const& status_bytes);

    std::vector<uint32_t>
    assemble_spectrum(std::span<const uint8_t> buf);

    std::vector<uint32_t>
    rebin_spectrum(std::vector<uint32_t> const& orig_spectrum);

    void upload_ascii_settings(const std::string& ascii);
    void save_debug(
        DetectorMessages::X123Debug::Type const& t,
        X123Driver::Packets::BasePacket const& pack
    );

    std::unique_ptr<X123Driver::Packets::Responses::BaseSpectrum>
    elicit_spectrum_packet();

    void num_histogram_bins_from_ram();
};

} // namespace Detector

