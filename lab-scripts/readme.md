# Scripts for taking data with detector flight code and quickly processing it afterwards

## Table of Contents
- [General instructions](#instructions)
- [Description of what's happening](#what-is-going-on)
- [How to read in data in general](#how-do-i-read-the-data-in-afterwards)
- [How to take and analyze HaFX data](#how-to-take-and-analyze-hafx-data)

## INSTRUCTIONS
### Requirements before you begin
- run `make install` on the `flight-controller` code (see its readme).
- run `pip install -e .` in the Python package directory to install it (see its readme).

### Steps to take some data
0. Change directory into the `data-collection` subdirectory
1. Run `bash init.bash`. Launches the programs needed to take and save data.
    - If you need to update serial numbers: modify `launch_detector.bash`
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

## How to take and analyze HaFX data
1. Follow the directions given in the [instructions](#instructions) for acquiring the data.
2. Copy the resulting output files into an analysis directory on your local computer.
3. Open the data using the `ground` code from the Python package.
   There are examples in the Python package root in the form of Jupyter notebooks.
   There is a script in the `util` directory which may be used to plot "nominal science" data.

**If you want to know the settings used for each detector,
you need to read out the
[arm_ctrl](https://www.bridgeportinstruments.com/products/software/wxMCA_doc/documentation/english/mds/mca3k/mca3k_arm_ctrl.html)
and [fpga_ctrl](https://www.bridgeportinstruments.com/products/software/wxMCA_doc/documentation/english/mds/mca3k/mca3k_fpga_ctrl.html)
registers
using the `hafx_registers_read.bash` script.**
