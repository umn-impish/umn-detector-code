# look_at_pps.py
# This file is used to verify that PPS is working

from umndet.common import helpers
import gzip
import os
import sys

try:
    directory = sys.argv[1]
except IndexError as e:
    msg = "Supply the directory with science data files as a command line argument"
    raise ValueError(msg) from e

data_files = [f"{directory}/{fn}" for fn in os.listdir(directory) if "time-slice" in fn]

data = []
for file in data_files:
    data += helpers.read_hafx_sci(file, gzip.open)

"""
You should see the buffer numbers start low and then get bigger.
Eventually you should see the numbers get reset to zero and then cycle from 0 to 31 (or 32)
repeatedly.
When you see the numbers cycling it means the PPS is connected properly.
If they do not cycle it means the PPS is disconnected or misconfigured.
"""
buffer_numbers = [d.buffer_number for d in data]
print(buffer_numbers)
