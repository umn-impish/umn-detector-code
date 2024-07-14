#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <thread>
#include <cmath>
#include <span>
#include <sys/socket.h>

#include <HafxControl.hh>
#include <DetectorMessages.hh>

constexpr int base_port = 61900;

std::unique_ptr<Detector::HafxControl> get_test_hafx_ctrl() {
    std::string TEST_SERIAL{"55FD9A8F4A344E5120202041131E05FF"};
    SipmUsb::BridgeportDeviceManager device_manager;

    return std::make_unique<Detector::HafxControl>(
        device_manager.device_map.at(TEST_SERIAL),
        Detector::DetectorPorts{base_port, base_port + 1}
    );
}


TEST(HafxCtrl, CreationDeletion) {
    auto ctrl = get_test_hafx_ctrl();
}

TEST(HafxCtrl, HealthPacket) {
    using namespace Detector;

    auto ctrl = get_test_hafx_ctrl();

    auto h = ctrl->generate_health();
    // Make sure we get logical values
    EXPECT_TRUE((25000 < h.arm_temp) && (h.arm_temp < 35000));
    EXPECT_TRUE((25000 < h.sipm_temp) && (h.sipm_temp < 35000));
    EXPECT_TRUE((1000 < h.sipm_operating_voltage) && (h.sipm_operating_voltage < 4000));
    EXPECT_TRUE((1000 < h.sipm_target_voltage) && (h.sipm_target_voltage < 4000));
    EXPECT_TRUE(h.counts <= std::numeric_limits<uint32_t>::max());
    EXPECT_TRUE(h.dead_time <= std::numeric_limits<uint32_t>::max());
    EXPECT_TRUE(h.real_time <= std::numeric_limits<uint32_t>::max());
}

TEST(HafxCtrl, SettingsFetchSave) {
    /*
     * Test that getting/setting the settings works.
     * Only checks one register type.
     * Probably should check them all!
     * --
     * Reads out settings, then writes new ones.
     * After write, checks that the updated settings were updated.
     */
    auto ctrl = get_test_hafx_ctrl();

    auto sett = ctrl->fetch_settings();
    // auto regz = sett.arm_ctrl_regs;

    ctrl->update_settings(sett);

    auto reread_sett = ctrl->fetch_settings();
    auto reread_regs = reread_sett.arm_ctrl_regs;
    auto old_regs = sett.arm_ctrl_regs;
    float epsilon = 0.0001;
    for (size_t i = 0; i < old_regs.size(); ++i) {
        EXPECT_TRUE(
            std::abs(reread_regs[i] - old_regs[i]) < epsilon
        );
    }
}

TEST(HafxCtrl, OperationRestarts) {
    /*
     * Just tests of operation of list mode and time_slice mode can be restarted.
     * Do it a couple times for good measure.
     */
    auto ctrl = get_test_hafx_ctrl();
    ctrl->restart_time_slice_or_histogram();
    ctrl->restart_list();
    ctrl->restart_time_slice_or_histogram();
    ctrl->restart_list();

    // No exception = good
    SUCCEED();
}

TEST(HafxCtrl, HistogramSlices) {
    /*
     * Checks if we can get some time_slices from the HaFX controller.
     * Controller sends em out via a socket so we just listen on
     * the socket it expects to send data to.
     */
    auto ctrl = get_test_hafx_ctrl();
    ctrl->restart_time_slice_or_histogram();

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    // just test saving operation; ignore data output to the socket
    ctrl->poll_save_time_slice();
}

TEST(HafxCtrl, DebugCollections) {
    auto ctrl = get_test_hafx_ctrl();

    // All possible debugs
    ctrl->read_save_debug<SipmUsb::ArmStatus>();
    ctrl->read_save_debug<SipmUsb::FpgaListMode>();
    ctrl->read_save_debug<SipmUsb::FpgaHistogram>();
    ctrl->read_save_debug<SipmUsb::FpgaWeights>();
    ctrl->read_save_debug<SipmUsb::FpgaStatistics>();
    ctrl->read_save_debug<SipmUsb::FpgaCtrl>();
    ctrl->read_save_debug<SipmUsb::ArmStatus>();
    ctrl->read_save_debug<SipmUsb::ArmCal>();
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
