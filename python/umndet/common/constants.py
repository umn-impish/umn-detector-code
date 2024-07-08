import numpy as np
import datetime as dt

DATE_FMT = '%Y-%j-%H-%M-%S'

# Keep track of rebin param revisions here

# June 26, 2024 - testing revision
FIRST_REVISION = dt.datetime.strptime('2024-178', '%Y-%j')
FIRST_NUM_TIMES_REBIN = 128
FIRST_NEW_ENERGY_EDGES = (0, 10, 20, 30, 40, 60, 90, 124)

# July 7, 2024 - testing revision
_num_adc = 2048
_num_to_map = 123
_first_bins = [
    # Start at 5 because the first 4 bins are housekeeping
    # in the IMPRESS firmware
    5 + histogram_bin

    # 123 bins (5 through 127)
    for histogram_bin in range(_num_to_map)

    # mapped linearly to 2048 ADC bins
    for _ in range(int(_num_adc / _num_to_map))
]
# 2048 % 123 = 80 so we need to add extra bins
# just make them "overflow"
_first_bins += [127] * (_num_adc - len(_first_bins))

# give the bins a "public" name
FIRST_BRIDGEPORT_EDGES = tuple(
    _first_bins
)


# Set revision in use here
REV = 'FIRST'
# Variables get selected based on REV
fetcher = lambda rev, ident: globals()[rev + ident]


# Current parameters in-use
NUM_TIMES_REBIN = fetcher(REV, '_NUM_TIMES_REBIN')
NEW_ENERGY_EDGES = fetcher(REV, '_NEW_ENERGY_EDGES')

## These are the bin edges which get sent to the Bridgeport detectors
BRIDGEPORT_EDGES = fetcher(REV, '_BRIDGEPORT_EDGES')
