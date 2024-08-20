#!/bin/bash
source udpcap_ports.bash
echo "wake" > $det_udp_dev
echo "debug hafx c1 fpga_oscilloscope_trace 1" > $det_udp_dev
sleep 3
echo "sleep" > $det_udp_dev
exit 0
