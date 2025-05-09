#pragma once

#include <IoContainer.hh>
#include <UsbManager.hh>

#include <DetectorMessages.hh>

#include <DetectorSupport.hh>
#include <functional>
#include <typeindex>
#include <unordered_map>

namespace Detector
{

// Reduce the NRL native 11-byte frame to a
// smaller one
std::vector<DetectorMessages::StrippedNrlDataPoint>
strip_nrl_data(std::vector<SipmUsb::NrlListDataPoint>&& vec);

class HafxControl {
public:
    HafxControl(std::shared_ptr<SipmUsb::UsbManager> driver_, DetectorPorts ports);

    DetectorMessages::HafxHealth
    generate_health();

    void restart_time_slice_or_histogram();
    void restart_list_mode();
    void restart_trace();
    bool check_trace_done();
    void swap_nrl_buffer(uint8_t buf_num);
    void use_full_size(bool full_size);
    void poll_save_nrl_list();
    void poll_save_time_slice();

    // debug, settings
    void update_settings(const DetectorMessages::HafxSettings& new_settings);
    DetectorMessages::HafxSettings
    fetch_settings();

    template <class ConT>
    void read_save_debug();

    std::optional<time_t> data_time_anchor() const;
    void data_time_anchor(std::optional<time_t> new_anchor);
private:
    std::shared_ptr<SipmUsb::UsbManager> driver;

    SettingsSaver settings_saver;

    std::optional<time_t> science_time_anchor;

    using science_t = DetectorMessages::HafxNominalSpectrumStatus;
    std::unique_ptr<QueuedDataSaver<science_t> > science_saver;
    std::unique_ptr<DataSaver> nrl_data_saver;
    std::unique_ptr<DataSaver> debug_saver;

    // Do we want to save full-size NRL data?
    bool save_full_size;

    DetectorMessages::HafxNominalSpectrumStatus
    read_time_slice();

    std::vector<SipmUsb::NrlListDataPoint>
    read_nrl_buffer();

    void save_settings(const DetectorMessages::HafxSettings& settings);
    void send_off_settings();
    DetectorMessages::HafxSettings
    construct_default_settings();

    template<class ConT, class SourceT>
    void update_registers(SourceT const& source_regs);
};

// --
// Template implementations

template <class ConT>
void HafxControl::read_save_debug() {
    using dbt = DetectorMessages::HafxDebug;

    // Map from a real type to an enum type
    static const std::unordered_map<
        std::type_index,
        dbt::Type>
    TAG_MAP = {
        {typeid(SipmUsb::FpgaListMode), dbt::Type::ListMode},
        {typeid(SipmUsb::FpgaHistogram), dbt::Type::Histogram},
        {typeid(SipmUsb::FpgaWeights), dbt::Type::FpgaWeights},
        {typeid(SipmUsb::FpgaStatistics), dbt::Type::FpgaStatistics},
        {typeid(SipmUsb::FpgaOscilloscopeTrace), dbt::Type::FpgaOscilloscopeTrace},
        {typeid(SipmUsb::FpgaCtrl), dbt::Type::FpgaCtrl},
        {typeid(SipmUsb::ArmStatus), dbt::Type::ArmStatus},
        {typeid(SipmUsb::ArmCal), dbt::Type::ArmCal},
        {typeid(SipmUsb::ArmCtrl), dbt::Type::ArmCtrl}
    };

    ConT dbgc;
    auto tag = TAG_MAP.at(typeid(ConT));

    driver->read(dbgc, SipmUsb::MemoryType::ram);
    const auto& buf = dbgc.registers;

    std::stringstream to_save;
    to_save.write(reinterpret_cast<char const*>(&tag), sizeof(tag));
    to_save.write(reinterpret_cast<char const*>(buf.data()), buf.size() * sizeof(buf[0]));

    debug_saver->add(to_save.str());
}

} // namespace Detector
