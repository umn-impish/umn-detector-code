#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <thread>
#include <UsbManager.hh>
#include <IoContainer.hh>

std::shared_ptr<SipmUsb::UsbManager> get_usb_manager() {
    std::string TEST_SERIAL{"55FD9A8F4A344E5120202041131E05FF"};
    SipmUsb::BridgeportDeviceManager device_manager;
    return device_manager.device_map.at(TEST_SERIAL);
}

template<class con_t>
void read_write_verify() {
    auto driv = get_usb_manager();

    con_t c;
    driv->read(c, SipmUsb::MemoryType::ram);
    auto read_regs = c.registers;

    driv->write(c, SipmUsb::MemoryType::ram);

    con_t new_one;
    driv->read(new_one, SipmUsb::MemoryType::ram);

    auto new_regs = new_one.registers;
    for (size_t i = 0; i < new_regs.size(); ++i) {
        EXPECT_EQ(new_regs[i], read_regs[i]);
    }
}

template<class con_t>
void read_only() {
    auto driv = get_usb_manager();

    con_t c;
    driv->read(c, SipmUsb::MemoryType::ram);
}

TEST(sipm3k, ReadWriteAllValid) {
    read_write_verify<SipmUsb::FpgaCtrl>();
    read_write_verify<SipmUsb::FpgaWeights>();
    read_write_verify<SipmUsb::ArmCtrl>();
    read_write_verify<SipmUsb::ArmCal>();

    // no issues: good
    SUCCEED();
}

TEST(sipm3k, ReadAllValid) {
    using namespace SipmUsb;
    read_only<ArmStatus>();
    read_only<ArmVersion>();
    read_only<FpgaTimeSlice>();
    read_only<FpgaResults>();
    read_only<FpgaStatistics>();
    read_only<FpgaHistogram>();
    read_only<FpgaListMode>();
    read_only<FpgaOscilloscopeTrace>();

    // no issues: good
    SUCCEED();
}

TEST(sipm3k, TimeSliceRead) {
    using namespace SipmUsb;
    auto driv = get_usb_manager();

    auto restart_regs = FPGA_ACTION_START_NEW_HISTOGRAM_ACQUISITION;
    driv->write(restart_regs, MemoryType::ram);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    FpgaResults res{};
    driv->read(res, MemoryType::ram);
    auto num_slices = res.num_avail_time_slices();

    EXPECT_GE(num_slices, 0) << "did not get any available slices after 1s delay";

    // verify that read works and decode it
    // read a few to get >0 buffer number
    FpgaTimeSlice tsc{};
    driv->read(tsc, MemoryType::ram);
    driv->read(tsc, MemoryType::ram);
    driv->read(tsc, MemoryType::ram);
    auto buff_num = tsc.decode().buffer_number;
    EXPECT_GE(buff_num, 0) << "buffer number was zero";
    EXPECT_LE(buff_num, 256) << "buffer number was crap data";
}

TEST(sipm3k, TraceDone) {
    using namespace SipmUsb;
    auto driv = get_usb_manager();
    
    auto restart_regs = FPGA_ACTION_START_NEW_TRACE_ACQUISITION;
    driv->write(restart_regs, MemoryType::ram);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    FpgaResults res{};

    driv->read(res, MemoryType::ram);
    auto trace_done = res.trace_done();

    EXPECT_EQ(1, trace_done) << "Trace did not complete in 1 second";
}

// TODO add more tests for each container
// maybe template?

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
