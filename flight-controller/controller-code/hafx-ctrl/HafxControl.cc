  #include <HafxControl.hh>
#include <logging.hh>
#include <sstream>

namespace Detector
{

constexpr size_t SLICES_PER_SECOND = 32;
HafxControl::HafxControl(std::shared_ptr<SipmUsb::UsbManager> driver_, DetectorPorts ports) :
    driver{driver_},
    settings_saver{driver->get_arm_serial() + ".bin"},
    science_time_anchor{},
    // christ that's a long line
    science_saver{std::make_unique<
                  QueuedDataSaver<
                  DetectorMessages::HafxNominalSpectrumStatus> >(
                  ports.science, SLICES_PER_SECOND)},
    nrl_data_saver{std::make_unique<DataSaver>(ports.science)}, // will need different udp capture flags than time slice nominal
    debug_saver{std::make_unique<DataSaver>(ports.debug)}
{ }

DetectorMessages::HafxHealth HafxControl::generate_health() {
    using namespace SipmUsb;

    ArmStatus asc;
    FpgaStatistics fsc;
    driver->read(fsc, MemoryType::ram);
    driver->read(asc, MemoryType::ram);

    auto const& fsc_regs = fsc.registers;
    auto const& asc_regs = asc.registers;

    auto float_to_uint16 = [](const float x) {
        return static_cast<uint16_t>(x * 100);
    };
    static const float CELSIUS_TO_KELVIN = 273.15;

    return DetectorMessages::HafxHealth{
        .arm_temp = float_to_uint16(asc_regs[3] + CELSIUS_TO_KELVIN),
        .sipm_temp = float_to_uint16(asc_regs[4] + CELSIUS_TO_KELVIN),
        .sipm_operating_voltage = float_to_uint16(asc_regs[0]),
        .sipm_target_voltage = float_to_uint16(asc_regs[1]),
        .counts = fsc_regs[1],
        .dead_time = fsc_regs[3],
        .real_time = fsc_regs[0],
    };
}


void HafxControl::restart_time_slice_or_histogram() {
    using namespace SipmUsb;
    driver->write(FPGA_ACTION_START_NEW_HISTOGRAM_ACQUISITION, MemoryType::ram);
}

void HafxControl::restart_list_mode() {
    using namespace SipmUsb;
    driver->write(FPGA_ACTION_START_NEW_LIST_ACQUISITION, MemoryType::ram);
}

void HafxControl::restart_trace() {
    using namespace SipmUsb;
    driver->write(FPGA_ACTION_START_NEW_TRACE_ACQUISITION, MemoryType::ram);
}

bool HafxControl::check_trace_done() {
    using namespace SipmUsb;
    FpgaResults res{};
    driver->read(res, SipmUsb::MemoryType::ram);
    return res.trace_done();
}

std::optional<time_t> HafxControl::data_time_anchor() const {
    return science_time_anchor;
}

void HafxControl::data_time_anchor(std::optional<time_t> new_anchor) {
    science_time_anchor = new_anchor;
}

void HafxControl::poll_save_time_slice() {
    using namespace SipmUsb;

    FpgaResults fpga_res_con;
    driver->read(fpga_res_con, MemoryType::ram);
    auto avail = fpga_res_con.num_avail_time_slices();

    for (uint16_t i = 0; i < avail; ++i) {
        // don't save the initial bad reads when the time anchor is invalid
        if (!science_time_anchor) {
            log_warning("anchor invalid for " + driver->get_arm_serial());
            continue;
        }
        auto nominal = read_time_slice();
        science_saver->add(nominal);
    }
}

DetectorMessages::HafxNominalSpectrumStatus
HafxControl::read_time_slice() {
    using namespace SipmUsb;
    // decode data from this to save it
    FpgaTimeSlice time_slice_con;
    driver->read(time_slice_con, MemoryType::ram);

    const auto decoded = time_slice_con.decode();
    DetectorMessages::HafxNominalSpectrumStatus ret{};
    std::memset(&ret, 0, sizeof(ret));

    // start of chunk of 32 slices -- save the timestamp
    // else leave it zero
    if ((decoded.buffer_number % 32) == 0) {
        ret.time_anchor = *science_time_anchor;
        ++(*science_time_anchor);
    }

    // if buffer number rolls over to 32 onward,
    // it means that we missed a PPS. so we shoud note that.
    ret.missed_pps = (decoded.buffer_number > 31);

    ret.buffer_number = decoded.buffer_number;
    ret.num_evts = decoded.num_evts;
    ret.num_triggers = decoded.num_triggers;
    ret.dead_time = decoded.dead_time;
    ret.anode_current = decoded.anode_current;

    for (size_t i = 0; i < decoded.histogram.size(); ++i) {
        ret.histogram[i] = decoded.histogram[i];
    }

    return ret;
}

void HafxControl::swap_nrl_buffer(uint8_t buf_num) {
    using namespace SipmUsb;

    FpgaCtrl cont;
    // read out current Fpga Ctrl registers
    driver->read(cont, MemoryType::nvram);
    // update buffer number
    auto reg = cont.registers[15];
    // set bit 2 to 0 for bank 0,
    //           to 1 for bank 1
    reg = (buf_num == 0)? (reg & ~4) : (reg | 4);
    cont.registers[15] = reg;

    driver->write(cont, MemoryType::nvram);
}

std::vector<SipmUsb::NrlListDataPoint>
HafxControl::read_nrl_buffer() {
    using namespace SipmUsb;

    FpgaLmNrl1 nrl_buff;
    driver->read(nrl_buff, MemoryType::ram);
    return nrl_buff.decode();
}

void HafxControl::poll_save_nrl_list() {
    /*
     * Check if NRL list buffers 0 and 1 are full,
     * and read them out and save the contents
     * if they are.
     */
    using namespace SipmUsb;

    FpgaResults fpga_res_con;
    driver->read(fpga_res_con, MemoryType::ram);
    uint32_t time_after_read = time(NULL);

    auto save = [&](auto buf_num) {
        auto full = fpga_res_con.nrl_buffer_full(buf_num);
        if (!full) return;
        log_debug(std::to_string(buf_num) + " is full");
#if 1
        this->swap_nrl_buffer(buf_num);
        auto data = this->read_nrl_buffer();
        // If there is no PPS in the data,
        // we can't use it. So, discard it.
        bool has_pps = false; 
        for (const auto& d : data) {
            has_pps = has_pps || d.was_pps;
        }
        if (!has_pps) {
            log_info("there was no PPS");
            return;
        }
#endif
        auto stripped = strip_nrl_data(std::move(data));
        auto evts_save = std::string{
            reinterpret_cast<const char*>(stripped.data()),
            stripped.size() * sizeof(decltype(stripped)::value_type)
        };
        // Put some metadata into strings to save
        // Never gonna be more than 2048 events;
        // could be fewer but unlikely
        uint16_t len = static_cast<uint16_t>(stripped.size());
        auto size_save = std::string{
            reinterpret_cast<const char*>(&len),
            sizeof(len)};
        auto timestamp_save = std::string{
            reinterpret_cast<const char*>(&time_after_read),
            sizeof(time_after_read)};

        // Save order:
        // - # of events recorded
        // - data
        // - timestamp immediately after readout
        // save all at once so we don't get misaligned files
        this->nrl_data_saver->add(size_save + evts_save + timestamp_save);
    };

    // Save buffers 0, 1
    save(0);
    save(1);
}

void HafxControl::update_settings(const DetectorMessages::HafxSettings& new_settings) {
    // save settings to file, read em back, send em to detector
    save_settings(new_settings);
    send_off_settings();
}

DetectorMessages::HafxSettings HafxControl::fetch_settings() {
    DetectorMessages::HafxSettings ret;
    try {
        return settings_saver.read_struct<DetectorMessages::HafxSettings>();
    } catch (const DetectorException& e) {
        ret = construct_default_settings();
        log_warning(e.what() + std::string{"; using NVRAM ones"});
    }
    return ret;
}

void HafxControl::save_settings(const DetectorMessages::HafxSettings& new_settings) {
    auto to_save = fetch_settings();

    if (new_settings.adc_rebin_edges_length) {
        to_save.adc_rebin_edges_length = new_settings.adc_rebin_edges_length;
        to_save.adc_rebin_edges = new_settings.adc_rebin_edges;
    }
    if (new_settings.fpga_ctrl_regs_present) {
        to_save.fpga_ctrl_regs_present = new_settings.fpga_ctrl_regs_present;
        to_save.fpga_ctrl_regs = new_settings.fpga_ctrl_regs;
    }
    if (new_settings.arm_ctrl_regs_present) {
        to_save.arm_ctrl_regs_present = new_settings.arm_ctrl_regs_present;
        to_save.arm_ctrl_regs = new_settings.arm_ctrl_regs;
    }
    if (new_settings.arm_cal_regs_present) {
        to_save.arm_cal_regs_present = new_settings.arm_cal_regs_present;
        to_save.arm_cal_regs = new_settings.arm_cal_regs;
    }
    if (new_settings.fpga_weights_regs_present) {
        to_save.fpga_weights_regs_present = new_settings.fpga_weights_regs_present;
        to_save.fpga_weights_regs = new_settings.fpga_weights_regs;
    }

    settings_saver.write_struct<DetectorMessages::HafxSettings>(to_save);
}

DetectorMessages::HafxSettings
HafxControl::construct_default_settings() {
    DetectorMessages::HafxSettings ret;

    auto assign_nvram_regs = [this](auto container, auto &mut_regs) {
        driver->read(container, SipmUsb::MemoryType::nvram);
        mut_regs = container.registers;
    };

    assign_nvram_regs(SipmUsb::FpgaMap{}, ret.adc_rebin_edges);
    assign_nvram_regs(SipmUsb::FpgaCtrl{}, ret.fpga_ctrl_regs);
    assign_nvram_regs(SipmUsb::ArmCtrl{}, ret.arm_ctrl_regs);
    assign_nvram_regs(SipmUsb::ArmCal{}, ret.arm_cal_regs);
    assign_nvram_regs(SipmUsb::FpgaWeights{}, ret.fpga_weights_regs);

    return ret;
}

void HafxControl::send_off_settings() {
    auto settings = settings_saver.read_struct<DetectorMessages::HafxSettings>();

    // check if any have data in them and send em off
    if (settings.adc_rebin_edges_length)
        update_registers<SipmUsb::FpgaMap>(settings.adc_rebin_edges);
    if (settings.fpga_ctrl_regs_present)
        update_registers<SipmUsb::FpgaCtrl>(settings.fpga_ctrl_regs);
    if (settings.arm_ctrl_regs_present)
        update_registers<SipmUsb::ArmCtrl>(settings.arm_ctrl_regs);
    if (settings.arm_cal_regs_present)
        update_registers<SipmUsb::ArmCal>(settings.arm_cal_regs);
    if (settings.fpga_weights_regs_present)
        update_registers<SipmUsb::FpgaWeights>(settings.fpga_weights_regs);
}

template<class ConT, class SourceT>
void HafxControl::update_registers(SourceT const& source_regs) {
    ConT con;
    if (source_regs.size() != con.registers.size()) {
        std::stringstream ss;
        ss << "cannot set registers for " 
           << typeid(ConT).name() << "; wrong size ("
           << source_regs.size() << " from pb vs. "
           << con.registers.size() << " from driver)";
        throw DetectorException{ss.str()};
        return;
    }

    for (size_t i = 0; i < con.registers.size(); ++i) {
        con.registers[i] = source_regs[i];
    }
    driver->write(con, SipmUsb::MemoryType::nvram);
}

std::vector<DetectorMessages::StrippedNrlDataPoint>
strip_nrl_data(std::vector<SipmUsb::NrlListDataPoint>&& vec) {
    /*
     * The original data format coming out of the NRL list mode
     * is to store the wall clock as a 51-bit number, in multiples
     * of 25 nanoseconds. That could reach almost two year's worth
     * of data (650 days).
     *
     * Instead, in this case, we scale down the precision of the clock,
     * and assume that the data will only span times on the order
     * of ~5-10s. Then we don't need as many bits for the wall clock time.
     * We can also drop some other useless information--
     * we don't care about pulse shape discrimination.
     * */

    using stripped_t = DetectorMessages::StrippedNrlDataPoint;
    std::vector<stripped_t> ret;
    for (const auto& orig : vec) {
        ret.emplace_back(stripped_t::from(orig));
    }
    return ret;
}

} // namespace Detector
