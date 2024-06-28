set(DET_PROJ_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)
cmake_policy(SET CMP0076 NEW)

enable_testing()
find_package(GTest)

add_subdirectory(raii-gpio)

# Order matters (dependencies)
add_subdirectory(logging)
add_subdirectory(det-messages)
add_subdirectory(det-support)

add_subdirectory(libusb-cpp)
add_subdirectory(sipm3k-interface)
add_subdirectory(x123-interface)

add_subdirectory(hafx-ctrl)
add_subdirectory(x123-ctrl)

add_subdirectory(det-service)
add_subdirectory(det-controller)

# Install target
install(
    # baseline programs required to function
    TARGETS det-controller
    COMPONENT binaries
)

# Create the settings folder we need
file(
    MAKE_DIRECTORY "$ENV{HOME}/detector-config" OPTIONAL)

# All write/read
file(
    CHMOD "$ENV{HOME}/detector-config"
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_WRITE GROUP_EXECUTE
        WORLD_READ WORLD_WRITE WORLD_EXECUTE
)
