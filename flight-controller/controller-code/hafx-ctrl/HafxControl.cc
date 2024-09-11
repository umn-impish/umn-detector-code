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

void HafxControl::restart_nrl_list_or_list_mode() {
    using namespace SipmUsb;
    driver->write(FPGA_ACTION_START_NEW_LIST_ACQUISITION, MemoryType::ram);
}

void HafxControl::restart_trace() {
    using namespace SipmUsb;
    driver->write(FPGA_ACTION_START_NEW_TRACE_ACQUISITION, MemoryType::ram);
}

uint16_t HafxControl::check_trace_done() {
    using namespace SipmUsb;
    FpgaResults res{};
    driver->read(res, SipmUsb::MemoryType::ram);
    
    // From IoContainer.cc
    return res.trace_done();
}

std::optional<time_t> HafxControl::data_time_anchor() {
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

void HafxControl::swap_to_buffer_0() {
    using namespace SipmUsb;

    // container
    FpgaCtrl cont;
    // read out current Fpga Ctrl registers
    driver->read(cont, MemoryType::nvram);
    
    // check if in buffer 1
    if (cont.registers[15] & 0x4) {
        cont.registers[15] = cont.registers[15] & ~(0x4);
    } else {
        log_debug("Tried to swap to buffer 0 but was already in buffer 0");
        return;
    }
    //std::cerr << cont.registers[15] << std::endl;
    // send new fpga ctrl (tells it to swap to buffer 0)
    driver->write(cont, MemoryType::nvram);
}

void HafxControl::swap_to_buffer_1() {
    using namespace SipmUsb;
    // container
    FpgaCtrl cont;
    // read out current Fpga Ctrl registers
    driver->read(cont, MemoryType::nvram);
    
    // check if in buffer 0 (registers[15] bit 2)
    if ((cont.registers[15] & 0x4) == 0) {
        cont.registers[15] = cont.registers[15] | 0x4;
    } else {
        log_debug("Tried to swap to buffer 1 but was already in buffer 1");
        return;
    }
    //std::cerr << cont.registers[15] << std::endl;
    // send new fpga ctrl (tells it to swap to buffer 1)
    driver->write(cont, MemoryType::nvram);
}

SipmUsb::DecodedListBuffer
HafxControl::read_buffer() {
    using namespace SipmUsb;

    FpgaLmNrl1 NrlBuff;
    driver->read(NrlBuff, MemoryType::ram);
    const auto decodedNrl = NrlBuff.decode();
    // DetectorMessages::HafxNrlListStatus ret{};
    // ret.psd = decodedNrl.psd;
    // ret.energy = decodedNrl.energy;
    // ret.wc0 = decodedNrl.wc0;
    // ret.wc1 = decodedNrl.wc1;
    // ret.wc2 = decodedNrl.wc2;
    // ret.wc3af = decodedNrl.wc3af;

    return decodedNrl;
}

void HafxControl::poll_save_nrl_list() {
    using namespace SipmUsb;

    FpgaResults fpga_res_con;
    driver->read(fpga_res_con, MemoryType::ram);
    // to check if buffers are full
    bool full_0 = fpga_res_con.full_0();
    bool full_1 = fpga_res_con.full_1();
    
    // buffer 0 full
    if (full_0) {

        swap_to_buffer_0();

        SipmUsb::DecodedListBuffer read_out = read_buffer();
        std::stringstream to_save;
        for (size_t i = 0; i < 2048; ++i) {
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.psd[i]; // write each uint16_t in hex format
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.energy[i]; // need a better way to do this
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.wc0[i]; // could shrink file size by half if I can represent 
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.wc1[i]; // uint16_t's as 2 bytes in a .bin / .txt file
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.wc2[i]; // :P
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.wc3af[i];
        }
        std::cerr << "emptying 0" << std::endl;
        std::cerr << to_save.str().length() << std::endl;
        nrl_data_saver->add(to_save.str());
    }
    // buffer 1 full
    if (full_1) {
        
        swap_to_buffer_1();
        
        SipmUsb::DecodedListBuffer read_out = read_buffer();
        std::stringstream to_save;
        for (size_t i = 0; i < 2048; ++i) {
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.psd[i]; // RN each buffer is 50 kB
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.energy[i]; // could be 25 kB if I can do above
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.wc0[i];
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.wc1[i];
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.wc2[i];
            to_save << std::hex << std::setw(4) << std::setfill('0') << read_out.wc3af[i];
        }
        std::cerr << "emptying 1" << std::endl;
        std::cerr << to_save.str().length() << std::endl;
        nrl_data_saver->add(to_save.str());
    }
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

} // namespace Detector
