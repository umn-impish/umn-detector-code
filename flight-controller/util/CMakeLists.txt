# UDP_CAPTURE EXECUTABLE
add_executable(udp_capture 
    udp_capture.cpp
)
install(
    TARGETS udp_capture
    COMPONENT binaries
)
