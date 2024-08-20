#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [detector identifier]"
    exit 1
fi
 
bash check_running.bash || exit 1

det="$1"
source udpcap_ports.bash
echo "wake" > $det_udp_dev
echo "debug hafx $det fpga_oscilloscope_trace 1" > $det_udp_dev
sleep 3
echo "sleep" > $det_udp_dev
exit 0
