add_library(libusb-cpp STATIC
        LibUsbCpp.cc
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)

target_link_libraries(
    libusb-cpp
    PUBLIC
        logging
        PkgConfig::libusb
)

target_include_directories(
    libusb-cpp
    PUBLIC
        .
)
