add_library(sipm3k-interface STATIC)

target_sources(
    sipm3k-interface
    PRIVATE
        IoContainer.cc
        UsbManager.cc
)

target_link_libraries(
    sipm3k-interface
    PUBLIC
        logging
        libusb-cpp
)

target_include_directories(
    sipm3k-interface
    PUBLIC
        .
)

add_subdirectory(tests)
