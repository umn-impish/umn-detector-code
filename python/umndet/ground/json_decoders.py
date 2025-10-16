import argparse
import datetime as dt
import json
import gzip
import numpy as np
import datetime
from umndet.common import helpers as hp
from umndet.common import impress_exact_structs as ies
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
    '''Decodes specifically 4 Byte 'stripped' NRL List mode data. 
    Assumes the last PPS lines up with the unix time that is placed on while saving. 
    Corrects for timestamp looping and then formats to absolute times based on the unix time.
    Formats absolute times as: YYYY-DDD-HH-MM-SS_NNNNNNNNN
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

    jsonified = list()
    for fn in args.files:
        cur_buffers = hp.read_nrl_list(fn, gzip.open)
        for buffer in cur_buffers:
            jsonified += jsonify_exact_buffer(buffer)

    # Generally, these decoded data are massive.
    # So, save them compressed.
    output_fn: str = args.output_fn
    if not output_fn.endswith('.gz'):
        output_fn += '.gz'

    with gzip.open(output_fn, 'w') as f:
        bytes_ = json.dumps(jsonified).encode('utf-8')
        del jsonified
        f.write(bytes_)


def jsonify_exact_buffer(buffer: dict[str, object]) -> dict[str, object]:
    '''Take a list of EXACT NRL buffers which have been read into
       a dict format of {'timestamp': timestamp, 'buffers': [buffers]}
       and "jsonify" them into a list of JSON objects
       represented as dictionaries. 
    '''
    all_events, rel_times = list, list()
    last_pps_rel_time = None
    for i, e in enumerate(buffer['events']):
        all_events.append(e)
        rel_times.append(e.relative_timestamp)

        if not e.was_pps:
            continue

        # Save the relative timestamp of the last PPS for later
        last_pps_rel_time = e.relative_timestamp

    # The last PPS event is assumed to be aligned with the
    # absolute time saved immediately after the buffer readout.
    # So, we save the values for calibration in the next step.
    time_after = buffer['unix_time']
    anchor = datetime.datetime.fromtimestamp(time_after, datetime.UTC)

    all_events = [events.to_json() for events in all_events]

    # Put the events into a structure with absolute times
    for (evt, rel_time) in zip(all_events, rel_times):
        # Remove the relative time key; we will replace it
        del evt['relative_timestamp']

        # Datetime can't format nanoseconds natively, so add it manually after
        ns_delta = (rel_time - last_pps_rel_time) * ies.FullSizeNrlDataPoint.NS_PER_TICK
        delta = datetime.timedelta(microseconds=int(ns_delta / 1e3))

        abs_time = (anchor + delta).strftime('%Y-%j-%H-%M-%S')
        abs_time += f'_{int(ns_delta % 1e9)}'
        
        evt['absolute_timestamp'] = abs_time
    
    return all_events
