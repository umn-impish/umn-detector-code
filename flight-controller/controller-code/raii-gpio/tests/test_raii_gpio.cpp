#include <gtest/gtest.h>
#include <raii_gpio.h>

TEST (RaiiGpio, RisingEdgeDetect) {
    // NB this is a Broadcom pin.
    // you need to make sure the physical Pi pin is the same.
    // on the CM3+ the mapping is 1 to 1 but on other Pi's
    // sometimes it isn't.
    constexpr uint8_t GPIO_PIN = 31;
    RaiiGpioPin p{GPIO_PIN, RaiiGpioPin::Operation::rising_edge};

    struct timespec ts = { .tv_sec = 5, .tv_nsec = 0 };

    EXPECT_TRUE(p.edge_event(ts)) << "Rising edge not detected within 5s.";
    SUCCEED() << "Rising edge detected!";
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
