#!/bin/bash

# Start the correct detector programs.

bash verify.bash || exit 1

bash check_running.bash && echo "already running!" && exit 0

bash launch_detector.bash || exit 1 &
bash launch_udp_caps.bash || exit 1 &
echo "launched B)"
