#pragma once

#include <IoContainer.hh>
#include <UsbManager.hh>

#include <DetectorMessages.hh>

#include <DetectorSupport.hh>
#include <typeindex>
#include <unordered_map>

namespace Detector
{

class HafxControl {
public:
    HafxControl(std::shared_ptr<SipmUsb::UsbManager> driver_, DetectorPorts ports);

    DetectorMessages::HafxHealth
    generate_health();

    void restart_time_slice_or_histogram();
    void restart_nrl_list_or_list_mode();
    void restart_trace();
    uint16_t check_trace_done();
    void poll_save_nrl_list();
    void swap_to_buffer_0();
    void swap_to_buffer_1();
    void poll_save_time_slice();

    // debug, settings
    void update_settings(const DetectorMessages::HafxSettings& new_settings);
    DetectorMessages::HafxSettings
    fetch_settings();

    template <class ConT>
    void read_save_debug();

    std::optional<time_t> data_time_anchor();
    void data_time_anchor(std::optional<time_t> new_anchor);
private:
    std::shared_ptr<SipmUsb::UsbManager> driver;

    SettingsSaver settings_saver;

    std::optional<time_t> science_time_anchor;

    using science_t = DetectorMessages::HafxNominalSpectrumStatus;
    std::unique_ptr<QueuedDataSaver<science_t> > science_saver;
    std::unique_ptr<DataSaver> nrl_data_saver;
    std::unique_ptr<DataSaver> debug_saver;

    DetectorMessages::HafxNominalSpectrumStatus
    read_time_slice();

    DetectorMessages::HafxNrlListStatus
    read_buffer();

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
