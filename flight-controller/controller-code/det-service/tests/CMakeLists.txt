find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(test_det_service
    test_det_service.cc
)

target_include_directories(
    test_det_service
    PRIVATE
    ${GTEST_INCLUDE_DIRS}
    "${DET_PROJ_PATH}/include"
)

target_link_libraries(
    test_det_service
    PRIVATE
        det-service
        gtest
)

gtest_discover_tests(test_det_service)
