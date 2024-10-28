#!/bin/bash

# Start the detector UDP capture processes for NRL
# list mode data collection

mkdir -p 'live'
mkdir -p 'completed'

source ../udpcap_ports.bash
default_timeout=5

# Default: zip and move
post_process_cmd='
gzip "$out_file"; 
mv "$out_file.gz" completed'

# adjust as needed
science_lifetime=10
nom_ports_names=( [$HAFX_C1_SCI_PORT]='hafx-nrl-listmode-c1' \
    [$HAFX_M1_SCI_PORT]='hafx-nrl-listmode-m1' \
    [$HAFX_M5_SCI_PORT]='hafx-nrl-listmode-m5' \
    [$HAFX_X1_SCI_PORT]='hafx-nrl-listmode-x1')
for port in "${!nom_ports_names[@]}"; do
    udp_capture \
        -T $science_lifetime \
        -t $science_lifetime \
        -l $port \
        -b "live/${nom_ports_names[$port]}" \
        -p "$post_process_cmd" &
done

dbg_ports_names=( [$HAFX_C1_DBG_PORT]='hafx-nrl-debug-c1' \
    [$HAFX_M1_DBG_PORT]='hafx-nrl-debug-m1' \
    [$HAFX_M5_DBG_PORT]='hafx-nrl-debug-m5' \
    [$HAFX_X1_DBG_PORT]='hafx-nrl-debug-x1')
for port in "${!dbg_ports_names[@]}"; do
    # -t argument: Only wait 1s after a read to save the file
    udp_capture -m 60000 \
        -T "$default_timeout" \
        -t "1" \
        -l $port \
        -b "live/${dbg_ports_names[$port]}" \
        -p "$post_process_cmd" &
done

# Health listener
health_fwd_port=54001
cdh_port=51000
mpls_addr="10.133.225.126:$health_fwd_port"
stpaul_addr="192.168.2.50:$health_fwd_port"
cdh_addr="127.0.0.1:51000"

# 10 mins
health_timeout=$((10 * 60))
udp_capture -m 60000 -T "$health_timeout" -t "$health_timeout" -l "$DET_HEALTH_PORT" -f $cdh_addr -f $stpaul_addr -f $mpls_addr -b "live/detector-health" -p "$post_process_cmd" &
