import datetime as dt

DATE_FMT = '%Y-%j-%H-%M-%S'

# Keep track of rebin param revisions here

# June 26, 2024 - testing revision
FIRST_REVISION = dt.datetime.strptime('2024-178', '%Y-%j')
FIRST_NUM_TIMES_REBIN = 128
FIRST_NEW_ENERGY_EDGES = [0, 10, 20, 30, 40, 60, 90, 124]

# Current parameters in-use
NUM_TIMES_REBIN = FIRST_NUM_TIMES_REBIN
NEW_ENERGY_EDGES = FIRST_NEW_ENERGY_EDGES