#!/bin/bash

# Start the detector UDP capture processes

data_folder="data"
mkdir -p "$data_folder"
mkdir -p "downlink"
mkdir -p "downlink/rebinned"

source udpcap_ports.bash
default_timeout=5

post_process_cmd='
gzip "$out_file"; 
mv "$out_file.gz" downlink'

# x-123 udp capture processes
udp_capture -m 64000 \
    -T "$default_timeout" \
    -t "$default_timeout" \
    -l $((x123_port + 0)) \
    -b "data/x123-sci" \
    -p "$post_process_cmd" &
udp_capture -m 64000 \
    -T "$default_timeout" \
    -t "$default_timeout" \
    -l $((x123_port + 1)) \
    -b "data/x123-debug" \
    -p "$post_process_cmd" &

# scintillator (hafx) udp capture processes
# time_slice data listeners (IMPRESS)
# update these as appropriate
time_slice_size=512
time_slices_per_sec=32
num_seconds_in_file=30
max_data_sz=$((time_slice_size * time_slices_per_sec * num_seconds_in_file))

post_process_time_slice_cmd='
gzip "$out_file"; 
./rebin.py time+energy "$out_file.gz";
mv "$(dirname $out_file)/time+energy-$(basename $out_file).gz" downlink/rebinned;
mv "$out_file.gz" downlink;'

nom_ports_names=( [0]='hafx-time-slice-c1' \
    [3]='hafx-time-slice-m1' \
    [6]='hafx-time-slice-m5' \
    [9]='hafx-time-slice-x1')
for offst in "${!nom_ports_names[@]}"; do
    udp_capture -m "$max_data_sz" \
        -T "$default_timeout" \
        -t "$default_timeout" \
        -l $((hafx_port + $offst)) \
        -b "data/${nom_ports_names[$offst]}" \
        -p "$post_process_time_slice_cmd" &
done

dbg_ports_names=( [2]='hafx-debug-c1' \
    [5]='hafx-debug-m1' \
    [8]='hafx-debug-m5' \
    [11]='hafx-debug-x1')
for offst in "${!dbg_ports_names[@]}"; do
    # -t argument: Only wait 1s after a read to save the file
    udp_capture -m 60000 \
        -T "$default_timeout" \
        -t "1" \
        -l $((hafx_port + $offst)) \
        -b "data/${dbg_ports_names[$offst]}" \
        -p "$post_process_cmd" &
done

# Health listener
# forward to CDH ip/port
udp_capture -m 60000 -T "$default_timeout" -t "$default_timeout" -l "$health_port" -b "data/detector-health" -p "$post_process_cmd" &
