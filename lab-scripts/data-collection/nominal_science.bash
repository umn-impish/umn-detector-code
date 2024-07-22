#!/bin/bash

# Take time slice histograms for a given duration for all connected detectors.
# Also takes health data by default simultaneously, every 1s.
# Sole argument: acquisition time

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [acquisition time in second]"
    exit 1
fi

bash check_running.bash || exit 1

# For health port and UDP device
source udpcap_ports.bash

acq_time="$1"
time_between_health=1

echo "wake" > $det_udp_dev

# Start real data acquisition
echo "start-periodic-health $time_between_health 127.0.0.1:$health_port" > $det_udp_dev
echo "start-nominal" > $det_udp_dev
sleep $(( $acq_time + 1 ))

# Clean up
echo "stop-nominal" > $det_udp_dev
echo "stop-periodic-health" > $det_udp_dev
echo "sleep" > $det_udp_dev
