#include <memory>

#include <gtest/gtest.h>
#include <DetectorSupport.hh>
#include <DetectorMessages.hh>

TEST(DetSupport, ReadWriteX123Struct) {
    Detector::SettingsSaver saver{"test-x123.bin"};

    DetectorMessages::X123Settings sett;
    sett.ack_err_retries_present = true;
    sett.ack_err_retries = 1234;

    std::string ascii_sett{"MCAC=1024;PEE=THREE;"};
    size_t length = std::min(
        sett.ascii_settings.size(),
        ascii_sett.size());
    sett.ascii_settings_length = length;
    std::memcpy(
        sett.ascii_settings.data(),
        ascii_sett.data(),
        length);

    sett.adc_rebin_edges_length = 5;
    sett.adc_rebin_edges = {1, 2, 3, 4, 5};
    
    saver.write_struct<DetectorMessages::X123Settings>(sett);
    auto read_sett = saver.read_struct<DetectorMessages::X123Settings>();
    auto start = read_sett.ascii_settings.data();
    auto reread_ascii = std::string(start, start + read_sett.ascii_settings_length);
    EXPECT_EQ(reread_ascii, ascii_sett);

    // TODO add tests for other struct fields
    // should be trivially correct?

    SUCCEED();
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
