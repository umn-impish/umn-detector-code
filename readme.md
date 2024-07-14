# IMPRESS / EXACT / IMPISH : flight and some ground code
## This repo contains flight and some ground code for decoding data from IMPRESS, EXACT, and IMPISH missions.

## Please see [docs](./docs) for some documentation on how to use the code in the repo.

### Contents

#### `flight-controller`
- Contains code for interfacing with the detector hardware
- Has a utility called `udp_capture` which pipes data to files and other UDP sockets.

#### `python`
- In-flight utilities for rebinning science data along time or energy axes
- Ground code for decoding binary files into JSON

#### `lab-scripts`
- Scripts that can be used in-lab which runs the detector controller code
    and saves files via udp_capture.
