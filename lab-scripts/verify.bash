#!/bin/bash

if ! command -v det-controller &>/dev/null; then
    echo "det-controller executable not found.  Please add it to your PATH."
    exit 1
fi

if ! command -v udp_capture &>/dev/null; then
    echo "udp_capture executable not found.  Please add it to your PATH."
    exit 1
fi
