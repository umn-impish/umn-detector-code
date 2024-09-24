#!/bin/bash

# Start NRL list mode data acquisition.

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [acquisition time in second]"
    exit 1
fi

bash ../check_running.bash || exit 1

# For health port and UDP device
source ../udpcap_ports.bash

acq_time="$1"

echo "wake" > $det_udp_dev

# Start real data acquisition
echo "start-nrl-list" > $det_udp_dev
sleep $(( $acq_time + 2 ))

# Clean up
echo "stop-nrl-list" > $det_udp_dev
echo "sleep" > $det_udp_dev
