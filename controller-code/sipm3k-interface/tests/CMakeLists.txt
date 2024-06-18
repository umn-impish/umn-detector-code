find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(test_sipm3k
    test_sipm3k_interface.cc
)

target_include_directories(
    test_sipm3k
    PRIVATE
    ${GTEST_INCLUDE_DIRS}
    "${DET_PROJ_PATH}/include"
)

target_link_libraries(
    test_sipm3k
    PRIVATE
    sipm3k-interface
    pthread
    atomic
    gtest
    ${CMAKE_THREAD_LIBS_INIT}
)

gtest_discover_tests(test_sipm3k)
