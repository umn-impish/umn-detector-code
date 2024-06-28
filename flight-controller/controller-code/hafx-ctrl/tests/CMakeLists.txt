find_package(GTest REQUIRED)
add_executable(test_hafx_ctrl
    test_hafx_ctrl.cc
)

message("path is: ${COMMON_LIBRARY_DIRECTORY_PATH}")

target_include_directories(
    test_hafx_ctrl
    PRIVATE
    ${GTEST_INCLUDE_DIRS}
    "${COMMON_LIBRARY_DIRECTORY_PATH}/src"
    "${DET_PROJ_PATH}/include"
)

target_link_libraries(
    test_hafx_ctrl
    PRIVATE
    hafx-ctrl
    det-messages
    gtest
    pthread
)

gtest_discover_tests(test_hafx_ctrl)
