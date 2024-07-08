cmake_minimum_required(VERSION 3.0)
#Incomplete, need to update names when getting product ID for DP5 and sipm3k boards
install(FILES "usb.rules" DESTINATION /etc/udev/rules.d/ COMPONENT udev)