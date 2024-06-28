find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(test_det_support
    test_det_support.cc
)

target_include_directories(
    test_det_support
    PRIVATE
    ${GTEST_INCLUDE_DIRS}
    "${DET_PROJ_PATH}/include"
)

target_link_libraries(
    test_det_support
    PRIVATE
    det-support
    det-messages
    logging
    pthread
    atomic
    gtest
    ${CMAKE_THREAD_LIBS_INIT}
)

gtest_discover_tests(test_det_support)
