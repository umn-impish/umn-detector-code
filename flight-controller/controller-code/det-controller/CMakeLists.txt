# main executable
add_executable(det-controller main.cc)

# need built .a to be available
target_link_directories(
    det-controller PUBLIC "${LIB_OUT_DIR}")

target_link_libraries(
    det-controller
    PUBLIC
        det-service
        logging
)

# Boost
find_package(Boost ${BOOSTVERSION} REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIR})
target_link_directories(det-controller PUBLIC ${Boost_LIBRARY_DIRS})

target_sources(
    det-controller
    PRIVATE
        Listener.cc
)
target_include_directories(
    det-controller
    PUBLIC
        .
)
