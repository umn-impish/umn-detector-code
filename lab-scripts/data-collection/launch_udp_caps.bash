#!/bin/bash

# Start the detector UDP capture processes

mkdir -p live
mkdir -p 'completed'
mkdir -p "completed/rebinned"
# The post-process command for time_slice data has strange
# string expansion, so we just manually replace the two
# directories above if we want to change their names
post_process_time_slice_cmd='
gzip "$out_file"; 
impress-rebinner time+energy "$out_file.gz";
mv "$(dirname $out_file)/time+energy-$(basename $out_file).gz" completed/rebinned;
mv "$out_file.gz" completed;'

source udpcap_ports.bash
default_timeout=5

post_process_cmd='
gzip "$out_file"; 
mv "$out_file.gz" completed'

# x-123 udp capture processes
udp_capture -m 64000 \
    -T "$default_timeout" \
    -t "$default_timeout" \
    -l $((x123_port + 0)) \
    -b "live/x123-sci" \
    -p "$post_process_cmd" &
udp_capture -m 64000 \
    -T "$default_timeout" \
    -t "$default_timeout" \
    -l $((x123_port + 1)) \
    -b "live/x123-debug" \
    -p "$post_process_cmd" &

# scintillator (hafx) udp capture processes
# time_slice data listeners (IMPRESS)
# update these as appropriate
time_slice_size=512
time_slices_per_sec=32
num_seconds_in_file=30
max_data_sz=$((time_slice_size * time_slices_per_sec * num_seconds_in_file))

nom_ports_names=( [0]='hafx-time-slice-c1' \
    [3]='hafx-time-slice-m1' \
    [6]='hafx-time-slice-m5' \
    [9]='hafx-time-slice-x1')
for offst in "${!nom_ports_names[@]}"; do
    udp_capture -m "$max_data_sz" \
        -T "$default_timeout" \
        -t "$default_timeout" \
        -l $((hafx_port + $offst)) \
        -b "live/${nom_ports_names[$offst]}" \
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
        -b "live/${dbg_ports_names[$offst]}" \
        -p "$post_process_cmd" &
done

# Health listener
udp_capture -m 60000 -T "$default_timeout" -t "$default_timeout" -l "$health_port" -b "live/detector-health" -p "$post_process_cmd" &
