find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(test_raii_gpio
    test_raii_gpio.cpp
)

target_link_libraries(test_raii_gpio
    GTest::gtest_main
    raii-gpio
)

gtest_discover_tests(test_raii_gpio)
