#!/bin/bash

# Take a "debug" list mode acquisition for a Bridgeport detector with the flight controller programs.
# First argument: acquisition time
# Second argument: detector identifier to use (c1, m1, m5, x1)

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 [acquisition time in second] [detector identifier]"
    exit 1
fi


bash check_running.bash || exit 1

acq_time="$1"
det="$2"
source udpcap_ports.bash
echo "init" > $det_udp_dev
echo "debug hafx $det list_mode $acq_time" > $det_udp_dev
sleep $(($acq_time + 1))
echo "shutdown" > $det_udp_dev
