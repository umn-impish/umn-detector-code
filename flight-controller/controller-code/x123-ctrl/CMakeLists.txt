add_library(x123-ctrl STATIC)

target_sources(
    x123-ctrl
    PRIVATE
        X123Control.cc
        X123DriverWrap.cc
)

target_link_libraries(
    x123-ctrl
    PUBLIC
        det-support
        det-messages
        logging
        x123-interface
)

target_include_directories(
    x123-ctrl
    PUBLIC
        ${LIBUSB_INCLUDE_DIR}
        .
)
