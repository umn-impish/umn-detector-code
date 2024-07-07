#!/bin/bash

# Makes sure programs are running before proceeding in other scripts.

user=$(whoami)
procs=$(ps -U $user)

if [[ $procs =~ "det-controller" ]] && [[ $procs =~ "udp_capture" ]]; then
    exit 0
fi

echo "detector processes not yet running."
exit 1
