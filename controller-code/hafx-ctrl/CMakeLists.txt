add_library(hafx-ctrl STATIC)

target_sources(
    hafx-ctrl
    PRIVATE
        HafxControl.cc
)

target_link_libraries(
    hafx-ctrl
    PUBLIC
        det-support
        det-messages
        logging
        sipm3k-interface
)

target_include_directories(
    hafx-ctrl
    PUBLIC
        .
)

add_subdirectory(tests)
