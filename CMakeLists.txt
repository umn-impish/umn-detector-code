cmake_minimum_required(VERSION 3.7...3.27)

install(FILES "00-bridgeport.rules" DESTINATION /etc/udev/rules.d/ COMPONENT udev)
install(FILES "01-amptek.rules" DESTINATION /etc/udev/rules.d/ COMPONENT udev)
