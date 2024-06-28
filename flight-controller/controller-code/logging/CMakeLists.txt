add_library(logging STATIC)

target_sources(
    logging
    PRIVATE
        logging.cc
)

target_link_libraries(
    logging
    PRIVATE
        systemd
)

target_include_directories(
    logging
    PUBLIC
    .
)
