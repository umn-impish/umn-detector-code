# port that detector controller listens on
# listens on ip INADDR_ANY
det_port="61999"

# serial numbers for the variuos detectors
# for testing, set them all to one valid serial number
# the serial number "AB28CB7F4A344E51202020382E2BBFF" is the board that has no custom firmware on it
# as of 13 June 2023 that is the board that is in the engineering model. subject to change, though.
c1_serial_number="55FD9A8F4A344E5120202041131E05FF"
m1_serial_number="none"
m5_serial_number="none"
x1_serial_number="none"

# base ports for the UDP capture processes that will
# listen for data
hafx_udp_capture_base_port="61000"
x123_udp_capture_base_port="61200"

# split these up because the command is really long
exc1="./bin/det-controller $det_port"
exc2=" $c1_serial_number $m1_serial_number $m5_serial_number $x1_serial_number"
exc3=" $x123_udp_capture_base_port $hafx_udp_capture_base_port"

exc="$exc1$exc2$exc3"

echo "executing following command: '$exc'"
eval $exc
