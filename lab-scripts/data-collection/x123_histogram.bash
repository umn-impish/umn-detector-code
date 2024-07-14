#!/bin/bash

# Take a "debug" histogram acquisition for a X-123 with the flight code
# Sole argument: acquisition time

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [acquisition time in second]"
    exit 1
fi

bash check_running.bash || exit 1

acq_time="$1"
source udpcap_ports.bash
echo "init" > $det_udp_dev
echo "debug x123 histogram $acq_time" > $det_udp_dev
sleep $(($acq_time + 1))
echo "shutdown" > $det_udp_dev
