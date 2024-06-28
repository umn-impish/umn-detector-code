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
        raii-gpio
)
