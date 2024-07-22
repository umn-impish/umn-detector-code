#!/bin/bash

# Take health packets for a specified amount of time.
# First argument: acquisition time
# Second argument: time between packets

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 [acquisition time in seconds] [time between health packets]"
    exit 1
fi


bash check_running.bash || exit 1

acq_time="$1"
time_between="$2"
source udpcap_ports.bash
echo $health_port
echo "wake" > $det_udp_dev
echo "start-periodic-health $time_between 127.0.0.1:$health_port" > $det_udp_dev
echo "SLEEPIN FOR $acq_time"
sleep $(($acq_time))
echo "sleep" > $det_udp_dev
