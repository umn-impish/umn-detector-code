#!/bin/bash

# Read out a specified register value from the specified hafx detector
# Valid options are enumerated in the detector-commands file


if [ "$#" -ne 2 ]; then
    echo "Usage: $0 [registers name] [detector identifier]"
    exit 1
fi

bash check_running.bash || exit 1

regs="$1"
det="$2"
source udpcap_ports.bash
echo "init" > $det_udp_dev
echo "debug hafx $det $regs" > $det_udp_dev
sleep 1
echo "shutdown" > $det_udp_dev
