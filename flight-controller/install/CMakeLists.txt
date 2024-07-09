cmake_minimum_required(VERSION 3.7...3.27)

install(FILES "10-bridgeport.rules" DESTINATION /etc/udev/rules.d/ COMPONENT udev)
install(FILES "11-dp5.rules" DESTINATION /etc/udev/rules.d/ COMPONENT udev)
