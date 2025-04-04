#!/bin/bash

# Take a "debug" histogram acquisition for a Bridgeport detector with the flight controller programs.
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
echo "wake" > $det_udp_dev
echo "debug hafx $det all_channel_histogram $acq_time" > $det_udp_dev
sleep $(($acq_time + 1))
echo "sleep" > $det_udp_dev
