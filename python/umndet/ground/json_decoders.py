import argparse
import datetime as dt
import json
import gzip
import numpy as np
import datetime
from umndet.common import helpers as hp
import umndet.common.constants as umncon

# Monkeypatch JSON to output only 2 decmials
# https://stackoverflow.com/a/69056325
class RoundingFloat(float):
    __repr__ = staticmethod(lambda x: format(x, '.2f'))

json.encoder.c_make_encoder = None
json.encoder.float = RoundingFloat


def decode_health():
    p = argparse.ArgumentParser(
        description='Decode health files to JSON')
    p.add_argument(
        'health_files', nargs='+',
        help='files to decode to JSON')
    p.add_argument(
        'output_fn',
        help='output file name to write JSON')
    args = p.parse_args()

    health_data = []
    for fn in args.health_files:
        health_data += hp.read_det_health(fn, gzip.open)

    jsonified = [hd.to_json() for hd in health_data]
    jsonified.sort(key=lambda e: e['timestamp'])
    collapsed = collapse_health(jsonified)

    processed_data = {}
    processed_data['start_time'] = collapsed['timestamp'][0]
    
    for i in ['c1', 'm1', 'm5', 'x1']:
        processed_data[i] = {}
        for j in ['arm_temp', 'sipm_temp', 'sipm_operating_voltage']:
            processed_data[i][j] = (cur_proc := {})
            cur_data = collapsed[i][j]['value']

            cur_proc['avg'] = np.mean(cur_data)
            cur_proc['min'] = min(cur_data)
            cur_proc['max'] = max(cur_data)

    processed_data['x123'] = (x123_proc := {})
    for j in ['board_temp', 'det_high_voltage', 'det_temp']:
        x123_proc[j] = (cur_proc := {})
        cur_data = collapsed['x123'][j]['value']

        cur_proc['avg'] = np.mean(cur_data)
        cur_proc['min'] = min(cur_data)
        cur_proc['max'] = max(cur_data)

    final_data = {}
    final_data['processed_data'] = processed_data
    final_data['raw_data'] = collapsed
    with open(args.output_fn, 'w') as f:
        json.dump(final_data, f, indent=1)


def decode_x123_sci():
    p = argparse.ArgumentParser(
        description='Decode X123 science files to JSON')
    p.add_argument(
        'x123_files', nargs='+',
        help='gzipped files to decode to JSON')
    p.add_argument(
        'output_fn',
        help='output file name to write JSON')
    args = p.parse_args()

    x123_data = []
    for fn in args.x123_files:
        x123_data += hp.read_x123_sci(fn, gzip.open)

    json_out = [xd.to_json() for xd in x123_data]
    json_out.sort(key=lambda e: e['timestamp'])
    with open(args.output_fn, 'w') as f:
        json.dump(json_out, f, indent=1)


def decode_hafx_debug_hist():
    p = argparse.ArgumentParser(
        description='Decode HaFX debug histograms to JSON')
    p.add_argument(
        'files', nargs='+',
        help='debug histogram files to decode to JSON')
    p.add_argument(
        'output_fn',
        help='output file name to write JSON')
    args = p.parse_args()

    data = []
    for fn in args.files:
        data += hp.read_hafx_debug(fn, gzip.open)
    
    decoded = [d.decode() for d in data]
    if any(d['type'] != 'histogram' for d in decoded):
        raise ValueError(
            "Cannot decode debug other than histograms,"
            " and at least one file given is not a histogram."
        )

    out = {
        'histograms': [d['registers'] for d in decoded]
    }
    with open(args.output_fn, 'w') as f:
        json.dump(out, f, indent=1)


def get_proper_timedelta(file_name):
    '''
    Rebinned science data will have different time deltas between events.
    This is because if we sum along the time axis, the counts in the
    spectrogram can be considered to be bounded by wider time edges.

    Make this a function so that we can update it if we change the rebinning scheme down the line.
    '''
    # File name format is: IDENT_DATE_#.extension
    identifier, date_str, _ = file_name.split('_')
    date = dt.datetime.strptime(date_str, umncon.DATE_FMT)

    slice_width = dt.timedelta(seconds=1 / 32) 

    if 'time' in identifier:
        # Add more revisions as appropriate
        if date >= umncon.FIRST_REVISION:
            return umncon.FIRST_NUM_TIMES_REBIN * slice_width

    return slice_width


def get_data_format(fn: str) -> str:
    '''
    Depending on the file naming convention used by the rebinner,
    we can either be dealing with:
        - "raw" aka full-resolution data
        - rebinned across time
        - rebinned across energy
        - rebinneda cross time and energy
    '''
    possibilities = ('time+energy', 'time', 'energy')
    for p in possibilities:
        if fn.startswith(p): return p

    # No rebinning has happened; return something useful
    return 'full_resolution'


def decode_hafx_sci():
    '''
    Decode science data from binary structures to JSON.
    Assumes:
        - The first record in the binary file has a valid UNIX timestamp
        - The time and energy rebinning has been updated in the `umndet.common.constants`

    Note that the timestamps correspond to the "left" edges of the
    times where counts are recorded.
    '''
    p = argparse.ArgumentParser(
        description='Decode HaFX science files to JSON')
    p.add_argument(
        'files', nargs='+',
        help='files to decode to JSON')
    p.add_argument(
        'output_fn',
        help='output file name to write JSON')
    args = p.parse_args()

    hafx_data = []
    time_deltas = []
    data_type = []
    for fn in args.files:
        hafx_data += (cur_data := hp.read_hafx_sci(fn, gzip.open))
        # Give as many timedeltas and data formats
        # as there are data points per file,
        # so that we can easily align them later
        time_deltas += ([get_proper_timedelta(fn)] * len(cur_data))
        data_type += [get_data_format(fn)] * len(cur_data)

    jsonified = [hd.to_json() for hd in hafx_data]
    
    # Default value: start of UNIX epoch
    utc_time = dt.datetime.fromtimestamp(0, dt.UTC)
    for i, json_dat in enumerate(jsonified):
        frame_num = json_dat['buffer_number']['value']
        step = time_deltas[i]
        anchor = int(json_dat.pop('time_anchor')['value'])

        if anchor != 0:
            utc_time = dt.datetime.fromtimestamp(anchor, dt.UTC)
        json_dat['timestamp'] = {
            'value': (utc_time + (frame_num % 32) * step).isoformat() + 'Z',
            'unit': 'N/A'
        }
        type_ = data_type[i]
        json_dat['datatype'] = {
            'value': type_,
            'unit': 'N/A'
        }

    jsonified.sort(key=lambda e: e['timestamp']['value'])
    collapsed = collapse_json(jsonified)
    with open(args.output_fn, 'w') as f:
        json.dump(collapsed, f)


def collapse_json(data: list[dict[str, object]]):
    collapse_keys = tuple(data[0].keys())
    ret = dict()

    for datum in data:
        for k in collapse_keys:
            try:
                ret[k]['value'].append(datum[k]['value'])
            except KeyError:
                ret[k] = {
                    # Only assign unit once here, not above in the `try`
                    'unit': datum[k]['unit'],
                    'value': [datum[k]['value']]
                }

    return ret


def collapse_health(dat: list[dict[str, object]]) -> list[dict[str, object]]:
    detectors = ('c1', 'm1', 'm5', 'x1', 'x123')
    ret = dict()

    for detector in detectors:
        ret[detector] = collapse_json([d[detector] for d in dat])

    ret |= {
        'timestamp': [d['timestamp'] for d in dat]
    }
    return ret

def decode_exact_sci():
    '''
    :P It's really ugly but it works
    There probably a little but of unintended behavior
    I have not looekd at every single data entry in the json to compare times
    '''
    p = argparse.ArgumentParser(
        description='Decode EXACT science files to JSON')
    p.add_argument(
        'files', nargs='+',
        help='files to decode to JSON')
    p.add_argument(
        'output_fn',
        help='output file name to write JSON')
    args = p.parse_args()
    exact_data, jsonified, pps_indices, pps_times, times, anchor, abs_times = [], [], [], [], [], [], []

    for fn in args.files:
        exact_data += hp.read_stripped_nrl_list(fn, gzip.open)
    for j in range(len(exact_data)):
        # get relative timestamp and pps times
        times = [r.relative_timestamp for r in exact_data[j]['events']]

        # correct for looping of the relative timestamp
        corrected_times = times.copy()
        rel_stamp_loops = 0
        for i in range(1,len(times)):
            if times[i] < times[i-1]:
                rel_stamp_loops += 1
            corrected_times[i] = times[i] + rel_stamp_loops * (2**25-1)

        for i, e in enumerate(exact_data[j]['events']):
            if not e.was_pps: continue
            pps_times.append(e.relative_timestamp)
            pps_indices.append(i)

        # do a bunch of stuff to convert from relative timestamp to absolute timestamp
        # assumes last pps aligns with anchor 

        # get corrected last pps, if not corrected all times are off by 'rel_stamp_loops' amount of cycles
        time_after = exact_data[j]['unix_time']
        adjusted_last_pps = corrected_times[pps_indices[-1]]

        adjusted = [t - adjusted_last_pps for t in corrected_times]
        deltas = [datetime.timedelta(microseconds=t/5) for t in adjusted]
        anchor = datetime.datetime.fromtimestamp(time_after, datetime.UTC)

        # get abs_times from anchor and deltas then convert to str
        abs_times = [(anchor + d).strftime('%Y-%j-%H-%M-%S-%f') for d in deltas]

        # make usable
        exact_data[j]['events'] = [events.to_json() for events in exact_data[j]['events']]
        # Swap relative timestamp for absolute timestamp
        for k in range(len(exact_data[j]['events'])):
            exact_data[j]['events'][k]['relative_timestamp'] = abs_times[k]
            if 'relative_timestamp' in exact_data[j]['events'][k]:
                # most beautiful line ever \/
                # there is probably a better way to rename the dict entry but ¯\(ツ)/¯
                exact_data[j]['events'][k]['absolute_timestamp'] = exact_data[j]['events'][k].pop('relative_timestamp')

        jsonified += [exact_data[j]]
        #clear for reuse next loop
        abs_times = []

    with open(args.output_fn, 'w') as f:
        json.dump(jsonified, f)

if __name__ == '__main__':
    print(" 1 for decoding health packets ")
    print(" 2 for decoding x123 science ")
    print(" 3 for hafx debug histogram ")
    print(" 4 for hafx time slice ")
    print(" 5 for exact science data")
    dtype = int(input("Please decode type (1-5): "))
    if dtype == 1:
        decode_health()
    elif dtype == 2:
        decode_x123_sci()
    elif dtype == 3:
        decode_hafx_debug_hist()
    elif dtype == 4:
        decode_hafx_sci()
    elif dtype == 5:
        decode_exact_sci()
    else:
        print("Please enter a valid decode type (1-4)")