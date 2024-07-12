cmake_minimum_required(VERSION 3.7...3.27)

install(FILES "10-bridgeport.rules" "11-dp5.rules" DESTINATION /etc/udev/rules.d/ COMPONENT udev)
install(FILES "det-controller.service" "det-parent.target" "det-udpcapture-hafx-debug-c1.service" 
"det-udpcapture-hafx-debug-m1.service" "det-udpcapture-hafx-debug-m5.service" "det-udpcapture-hafx-debug-x1.service" 
"det-udpcapture-hafx-sci-c1.service" "det-udpcapture-hafx-sci-m1.service" "det-udpcapture-hafx-sci-m5.service" 
"det-udpcapture-hafx-sci-x1.service" "det-udpcapture-health.service" "det-udpcapture-x123-debug.service" 
"det-udpcapture-x123-sci.service" "detvars.env" DESTINATION /etc/systemd/system/ )

add_custom_target(
	run_script ALL
	COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/controller-code/install/add_envars.bash
)
