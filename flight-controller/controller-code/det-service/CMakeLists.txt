find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

add_library(det-service STATIC)
target_sources(
    det-service
    PRIVATE
        DetectorService.cc
)

target_include_directories(
    det-service
    PUBLIC
        .
)

target_link_libraries(
    det-service
    PUBLIC
    	atomic
        hafx-ctrl
        x123-ctrl
        pthread
	PkgConfig::GPIOD
)
