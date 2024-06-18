find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

# RAII GPIO is its own library
add_library(raii-gpio raii_gpio.cpp)
target_include_directories(raii-gpio
    PUBLIC
    .
)
target_link_libraries(raii-gpio
    PkgConfig::GPIOD
)
