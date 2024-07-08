'''
As a command line argument, pass this script a folder 
which contains a bunch of .bin.gz "level zero" data files
from IMPRESS.

Two plots are generated:
    - a spectrogram showing the counts spectrum over time
    - a spectrum, aka the spectrogram summed along time

The 2048-to-123 Bridgeport bin mapping is undone
so the histogram ADC bins line up with the "normal"
Bridgeport bins.
'''

import argparse
import gzip
import os

import matplotlib.pyplot as plt
import numpy as np

from umndet import plotting
from umndet.common import helpers, constants

def main():
    p = argparse.ArgumentParser(
        description='Take in a folder of IMPRESS data and produce some nice plots')
    p.add_argument('data_folder', help='folder containing .bin.gz IMPRESS "Level0" files')

    args = p.parse_args()

    data = []
    files = os.listdir(args.data_folder)
    for fn in sorted(files):
        if not (fn.startswith('hafx-time-slice')):
            continue
        data += helpers.read_hafx_sci(f'{args.data_folder}/{fn}', gzip.open)

    fig, ax = plt.subplots(layout='constrained')
    _, _, pcm = plotting.plot_raw_time_slice_spectrogram(
        data, fig, ax
        # If you used unusual ADC bin mapping you can pass it in here
    )

    spectrum = np.array([
        hd.histogram for hd in data
    ]).sum(axis=0)
    adc_bins = helpers.reverse_bridgeport_mapping(
        constants.BRIDGEPORT_EDGES
    )

    fig, ax = plt.subplots(layout='constrained')
    ax.stairs(spectrum, adc_bins)
    ax.set(
        xlabel='Bridgeport ADC bin',
        ylabel='Counts',
        title='IMPRESS spectrum'
    )

    plt.show()


if __name__ == '__main__':
    main()
