add_library(det-messages STATIC)

target_include_directories(
    det-messages
    PUBLIC
        .)

target_sources(
    det-messages
    PRIVATE
        DetectorMessages.cc
)

target_link_libraries(
    det-messages
    PUBLIC
        sipm3k-interface
)
