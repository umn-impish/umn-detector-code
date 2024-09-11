#!/bin/bash

# Start the correct detector programs.

bash verify.bash || exit 1

bash check_running.bash && echo "already running!" && exit 0

det-controller || exit 1 &
bash nrl_launch_udp_caps.bash || exit 1 &
echo "launched B)"
