#!/bin/bash

# Collect a bunch of debug data using the flight code and dump it to files.
# Tries to do all available types.
# Might need to update in the future if we add other possible "debug" reads

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [detector identifier]"
    exit 1
fi

bash check_running.bash || exit 1

det="$1"

immediate_types="arm_ctrl
arm_cal
arm_status
fpga_ctrl
fpga_statistics
fpga_weights"

num=5
echo "Immediate reads (5 each)"
for cmd in $immediate_types; do
    echo "start $cmd"
    for (( i=0; i<$num; i++ )); do
        echo "$cmd --> $det"
        ./hafx_registers_read.bash "$cmd" "$det"
    done
done


delay=4
echo "Delay reads ($delay s each)"
./hafx_histogram.bash $delay $det
./hafx_list_mode.bash $delay $det

