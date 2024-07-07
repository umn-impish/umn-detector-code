#!/bin/bash

# Launch the detector program with correct arguments!

# serial numbers for the variuos detectors
c1_serial_number="55FD9A8F4A344E5120202041131E05FF"
m1_serial_number="none"
m5_serial_number="none"
x1_serial_number="none"

source udpcap_ports.bash
hafx_udp_capture_base_port="$hafx_port"
x123_udp_capture_base_port="$x123_port"

exc1="det-controller $det_port"
exc2=" $c1_serial_number $m1_serial_number $m5_serial_number $x1_serial_number"
exc3=" $x123_udp_capture_base_port $hafx_udp_capture_base_port"

exc="$exc1$exc2$exc3"

echo "executing following command: '$exc'"
eval $exc
