cmake_minimum_required(VERSION 3.7...3.27)

install(FILES "10-bridgeport.rules" "11-dp5.rules" DESTINATION /etc/udev/rules.d/ COMPONENT udev)

add_custom_target(
	run_script ALL
    # COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/controller-code/install/add_envars.bash
    COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_CURRENT_SOURCE_DIR}/add_envars.bash ${CMAKE_CURRENT_SOURCE_DIR}
)
