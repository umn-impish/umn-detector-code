cmake_minimum_required(VERSION 3.7...3.27)

project(impish-detector-controller)

find_package(GTest REQUIRED)
enable_testing()

add_subdirectory(util)
add_subdirectory(controller-code)
