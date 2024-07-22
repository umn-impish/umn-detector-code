#!/bin/bash

# Set up the detectors for their first boot.
# Sends the registers and ADC bin map which we specify.
# The ADC bin map is generated from Python and the other registers
# are just specified here until we get a better versioning system for them.

if [ "$#" -ne 0 ]; then
    echo "Usage: $0"
    echo "[sends initialization data to the detectors]"
    exit 1
fi


bash check_running.bash || exit 1


## Now send the commands to actually update all the settings
source udpcap_ports.bash
echo "wake" > $det_udp_dev

### X-123 registers
# tell controller we want to read 1024-bins histograms from X-123
echo 'settings-update x123 ascii_settings MCAC=1024;' > $det_udp_dev
# X-123 gets binned down to just 4 big histogram bins
# Leave this empty for now (don't rebin the X-123 data at all)
# echo 'settings-update x123 adc_rebin_edges 0 256 512 768 1024' > $det_udp_dev

### HaFX registers
# From Dec 2023
# These may be extracted with the Bridgeport software
fpga_ctrl='48000 3 4 60 48 65280 100 1092 610 0 0 17 512 0 16399 128'
# fpga_ctrl registers: 
# https://www.bridgeportinstruments.com/products/software/wxMCA_doc/documentation/english/mds/mca3k/mca3k_fpga_ctrl.html
echo "settings-update hafx c1 fpga_ctrl $fpga_ctrl" > $det_udp_dev
echo "settings-update hafx m1 fpga_ctrl $fpga_ctrl" > $det_udp_dev
echo "settings-update hafx m5 fpga_ctrl $fpga_ctrl" > $det_udp_dev
echo "settings-update hafx x1 fpga_ctrl $fpga_ctrl" > $det_udp_dev

# arm_ctrl registers:
# From Dec 2023
# These may be extracted with the Bridgeport software
arm_ctrl='0.0 0.0 1.0 1.0 2.0 0.10000000149011612 23.0 35.380001068115234 9000.0 20000.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0'
# https://www.bridgeportinstruments.com/products/software/wxMCA_doc/documentation/english/mds/mca3k/mca3k_arm_ctrl.html
echo "settings-update hafx c1 fpga_ctrl $fpga_ctrl" > $det_udp_dev
echo "settings-update hafx m1 fpga_ctrl $fpga_ctrl" > $det_udp_dev
echo "settings-update hafx m5 fpga_ctrl $fpga_ctrl" > $det_udp_dev
echo "settings-update hafx x1 fpga_ctrl $fpga_ctrl" > $det_udp_dev

# update the ADC bin mapping
# this is the custom mapping from 2048 ADC bins to 123 IMPRESS bins
# ---
# NB: next line requres `pip install -e .` inside the
# detector Python directory
generate-impress-bin-txt
# generates a file called bin-map.txt
# with the latest 2048-ADC to 123-bin conversion
bin_mapping=$(cat 'bin-map.txt')
rm 'bin-map.txt'
# ---
echo "settings-update hafx c1 adc_rebin_edges $bin_mapping" > $det_udp_dev
echo "settings-update hafx m1 adc_rebin_edges $bin_mapping" > $det_udp_dev
echo "settings-update hafx m5 adc_rebin_edges $bin_mapping" > $det_udp_dev
echo "settings-update hafx x1 adc_rebin_edges $bin_mapping" > $det_udp_dev


echo "sleep" > $det_udp_dev
