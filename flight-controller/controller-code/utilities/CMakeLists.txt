add_executable(hafx-spectrum hafx_spectrum.cc)

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
