#include <gtest/gtest.h>
#include <memory>
#include <DetectorService.hh>
#include <IoContainer.hh>

TEST(detservice, PpsDetect) {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    auto ser = std::make_unique<DetectorService>(socket_fd);
    bool had_pps = ser->await_pps_edge();
    EXPECT_TRUE(had_pps) << "PPS could not be detected";
    close(socket_fd);
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
