add_library(x123-interface STATIC)

target_sources(
    x123-interface
    PRIVATE
        UsbConnectionManager.cc
)

target_link_libraries(
    x123-interface
    PUBLIC
        logging
        libusb-cpp
)

add_subdirectory(packets)

target_include_directories(
    x123-interface
    PUBLIC
        .
)
