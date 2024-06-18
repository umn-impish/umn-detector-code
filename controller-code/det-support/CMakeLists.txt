add_library(det-support STATIC)

target_sources(
    det-support
    PRIVATE
        DetectorSupport.cc
)

target_include_directories(
    det-support
    PUBLIC
        .
)

target_link_libraries(
    det-support
    PUBLIC
        logging
        det-messages
)

add_subdirectory(tests)
