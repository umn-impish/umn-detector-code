add_executable(hafx-spectrum hafx_spectrum.cc common.hh)

target_link_libraries(
    hafx-spectrum
    PUBLIC
        hafx-ctrl
)

target_include_directories(
    hafx-spectrum
    PUBLIC
        .
)


add_executable(hafx-trace hafx_scope_trace.cc common.hh)

target_link_libraries(
    hafx-trace
    PUBLIC
        hafx-ctrl
)

target_include_directories(
    hafx-trace
    PUBLIC
        .
)
