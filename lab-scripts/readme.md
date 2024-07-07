# Scripts for taking data with detector flight code and quickly processing it afterwards

=======
### NB: assumes that `make install` has been run on the controller!
### See the readme in `flight/controller` for how to install the required programs.
### Also assumes that the associated Python package is installed

## Table of Contents
- [General instructions](#instructions)
- [How to take and analyze HaFX data](#how-to-take-and-analyze-a-hafx-histogram)
- [Description of what's happening](#what-is-going-on)
- [How to read in data in general](#how-do-i-read-the-data-in-afterwards)
- [How to plot data for detector checkout](#how-do-i-plot-data-to-make-sure-the-detector-works)

## INSTRUCTIONS
0. Change directory into the `data-collection` subdirectory
1. Run `bash init.bash`. Launches the programs needed to take and save data.
2. Run the script you wanna use with proper arguments, for example:
- `./hafx_histogram.bash [acquisition time] [detector identifier]`
- `./hafx_registers_read.bash [registers name] [detector identifier]`
- `./x123_histogram.bash [acquisition time]`
3. The data gets output by udpcapture files into "live" and "completed" directories.

## What is going on
These scripts just send commands to the detector "flight" program and dump the data into files
    via `udp_capture` processes.
The flight code handles all the low-level details and the scripts contain more abstract commands.

## How do I read the data in afterwards
Install the Python package at the repo root and use its functions to read in data.

**Some utility scripts are in the `util` folder.**

## How to take and analyze a HaFX histogram
1. Follow the directions given in the [instructions](#instructions) for acquiring the data.
2. Copy the resulting `hafx_debug...` file(s) into an analysis directory on your local computer.
3. Open the data using the `ground` code from the Python package.
An example is shown in the `hafx-histogram-analysis-example` directory.

**If you want to know the settings used for each detector,
you need to read out the arm_ctrl and fpga_ctrl registers
using the `hafx_registers_read.bash` script.**

## How do I plot data to make sure the detector works?
1. Make sure you have done `pip install` in the Python package at the root
    of the repository.
2. Take the output binary gzipped files from the lab scripts and supply them as arguments to `plot_science_spectra.py`.
3. Look at the images and JSON output: they are a spectrogram and a spectrum.

