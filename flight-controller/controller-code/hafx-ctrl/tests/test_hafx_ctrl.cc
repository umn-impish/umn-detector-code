#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <thread>
#include <cmath>
#include <span>
#include <sys/socket.h>

#include <HafxControl.hh>
#include <DetectorMessages.hh>

constexpr unsigned short science_port = 30000;
constexpr unsigned short debug_port = 31000;

std::unique_ptr<Detector::HafxControl> get_test_hafx_ctrl() {
    std::string TEST_SERIAL{"AB28CB7F4A344E51202020382E2B0BFF"};
    SipmUsb::BridgeportDeviceManager device_manager;

    return std::make_unique<Detector::HafxControl>(
        device_manager.device_map.at(TEST_SERIAL),
        Detector::DetectorPorts{science_port, debug_port}
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
    ctrl->restart_nrl_list_or_list_mode();
    ctrl->restart_time_slice_or_histogram();
    ctrl->restart_nrl_list_or_list_mode();

    // No exception = good
    SUCCEED();
}

TEST(HafxCtrl, HistogramSlices) {
    /*
     * Checks if we can get some time_slices from the HaFX controller.
     * Controller sends em out via a socket so we just listen on
     * the socket it expects to send data to.
     */

    // Make a socket to receive data on
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        throw std::runtime_error{"can't bind socket"};
    }
    auto opts = fcntl(sock_fd, F_GETFL);
    // Socket doesn't block
    opts |= O_NONBLOCK;
    fcntl(sock_fd, F_SETFL, opts);

    socklen_t length = sizeof(sockaddr_in);
    sockaddr_in addr;
    std::memset(&addr, 0, length);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(science_port));
    addr.sin_addr = in_addr{.s_addr = INADDR_ANY};

    int ret = bind(sock_fd, (sockaddr*)&addr, length);
    if (ret < 0) {
        throw std::runtime_error{"cannot bind socket"};
    }

    auto ctrl = get_test_hafx_ctrl();
    ctrl->data_time_anchor(time(nullptr));
    ctrl->restart_time_slice_or_histogram();

    for (int i = 0; i < 1; ++i) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(3s);

        ctrl->poll_save_time_slice();
    }

    constexpr size_t BUFFER_SIZE = 16384ULL;
    auto buffer = std::make_unique<uint8_t[]>(BUFFER_SIZE);

    DetectorMessages::HafxNominalSpectrumStatus nom;
    constexpr auto nominal_size = sizeof(DetectorMessages::HafxNominalSpectrumStatus);
    // Receive data until we can't
    errno = 0;
    while (errno != EWOULDBLOCK) {
        int received = recv(sock_fd, buffer.get(), BUFFER_SIZE, 0);
        if (received < 0) {
            std::cerr << "error receiving from socket: " << strerror(errno) << std::endl;
            break;
        }
        std::cout << received << std::endl;
        if (received % nominal_size == 0) {
            std::cerr << "size is good of data received" << std::endl;
        }

        // Copy the buffer into structs and print out their 
        // buffer numbers
        for (size_t i = 0; i < static_cast<size_t>(received); i += nominal_size) {
            std::memcpy(&nom, buffer.get() + i, nominal_size);

            std::cerr << "buffer number at index " << i << " is "
                      << nom.buffer_number << std::endl;
        }
    }


    close(sock_fd);
}

TEST(HafxCtrl, DebugCollections) {
    auto ctrl = get_test_hafx_ctrl();

    // All possible debugs
    ctrl->read_save_debug<SipmUsb::ArmStatus>();
    ctrl->read_save_debug<SipmUsb::FpgaListMode>();
    ctrl->read_save_debug<SipmUsb::FpgaHistogram>();
    ctrl->read_save_debug<SipmUsb::FpgaWeights>();
    ctrl->read_save_debug<SipmUsb::FpgaStatistics>();
    ctrl->read_save_debug<SipmUsb::FpgaOscilloscopeTrace>();
    ctrl->read_save_debug<SipmUsb::FpgaCtrl>();
    ctrl->read_save_debug<SipmUsb::ArmStatus>();
    ctrl->read_save_debug<SipmUsb::ArmCal>();
}

TEST(HafxCtrl, SwapBuffer) {
    auto ctrl = get_test_hafx_ctrl();

    //see if it can read out registers, swap one bit and write back
    ctrl->swap_to_buffer_0();
    ctrl->swap_to_buffer_1();
    ctrl->swap_to_buffer_0();
    ctrl->swap_to_buffer_1();
    ctrl->swap_to_buffer_1();
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
