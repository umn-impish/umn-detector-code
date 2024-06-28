add_subdirectory(requests)
add_subdirectory(responses)

target_sources(
    x123-interface
    PRIVATE
        BasePacket.cc
)
