# main executable
add_executable(detect-edge edge_detector.c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

target_link_libraries(
    detect-edge
    PUBLIC
        PkgConfig::GPIOD
)
