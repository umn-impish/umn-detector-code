## Sample database JSON decoder
## Instruction:
##  - Place the nominal folder in the same directory as database_json_decoder.py
##  - Run the python file with Python

import umndet.common.helpers as hp
import umndet.ground.json_decoders as hp2
import gzip
import numpy as np
import matplotlib.pyplot as plt
import datetime
import os
import json
import time
import datetime as dt

from umndet.common.constants import BRIDGEPORT_EDGES

def to_unix(input_time):
    return time.mktime(input_time.timetuple())

def to_int(input_int):
    return int(input_int)

hafx_data = []

data_folder = 'nominal'
for fn in sorted(os.listdir(data_folder)):
    full_fn = f'{data_folder}/{fn}'
    if fn.startswith('hafx-time-slice'):
        hafx_data += hp.read_hafx_sci(full_fn, gzip.open)

counts_spectrogram = np.array([
    hd.histogram for hd in hafx_data
])

from_timestamp = lambda ts: datetime.datetime.fromtimestamp(ts, tz=datetime.UTC)
recent = from_timestamp(hafx_data[0].time_anchor)
times = [recent]
idx = 1
for hd in hafx_data[1:]:
    if hd.time_anchor != 0:
        recent = from_timestamp(hd.time_anchor)
    times.append(recent + datetime.timedelta(seconds=((idx % 32) / 32)))
    idx += 1

times.append(times[-1] + datetime.timedelta(seconds=1/32))

unixtime = map(to_unix, times)

adc_bins=BRIDGEPORT_EDGES

reversed_bins = hp.reverse_bridgeport_mapping(adc_bins)

spectrum_prev = np.array([
    hd.histogram for hd in hafx_data
]).sum(axis=0)



spectrum = map(to_int, spectrum_prev)

final_data = {}
final_data['timestamp'] = list(unixtime)
final_data['histogram'] = counts_spectrogram.T.tolist()
final_data['reversed_bins'] = reversed_bins
final_data['spectrum'] = list(spectrum)

with open("output.json", 'w') as f:
    json.dump(final_data, f)