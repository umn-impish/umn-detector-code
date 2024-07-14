# How to install and run IMPRESS / EXACT / IMPISH flight code

This guide will show you how to setup the flight code on a fresh pi.

## Table of Contents
1. [Installation](#installation)
2. [Usage](#usage)
3. [PPS Setup](#pps-setup)
4. [Data Analysis](#data-analysis)


## Installation

First install necessary libraries:
```
sudo apt install libusb-1.0-0-dev gpiod libgpiod-dev libboost-all-dev libgtest-dev libsystemd-dev cmake build-essential git python3 python3-pip
```

Clone the flight code repository:
```
cd
git clone https://github.com/umn-impish/umn-detector-code.git
```

For the flight code to run properly you will need to install this repo's associated Python package:
```
cd umn-detector-code/python
pip install -e . --break-system-packages
```

Before you compile the code you will have to edit one file that will setup environment variables for the service to use. You will be editing the serial numbers in the file so the service can communicate with the bridgeport boards. First you will have to find the serial number of the boards. Plug in the boards you want to use to a computer with the `run_mds` scripts from the Bridgeport software. Then run `run_mds.cmd` or `run_mds.sh` and in the commnd prompt pop up it will list 'Attached MCA' and then hex strings in brackets. These strings are the serial numbers of the Bridgeport boards. Write them down somewhere safe. 

Next open the file that stores the environment variables via `nano`.

```
nano umn-detector-code/flight-controller/controller-code/install/envars.bash
```
From there you will see these lines:
```
HAFX_C1_SERIAL="55FD9A8F4A344E5120202041131E05FF"
HAFX_M1_SERIAL="none"
HAFX_M5_SERIAL="none"
HAFX_X1_SERIAL="none"
```
The `HAFX_C1_SERIAL` is the serial number of the board used at UMN (as of 7/24). Change these entries to be the serial numbers of the boards that you got from the Bridgeport software. If you are using only one board just change the c1 serial number and leave the rest.
Save it via ctrl-x, typing y, and hitting enter once.

Next, `cd` into the flight code directory and compile the code:
```
cd umn-detector-code/flight-controller
mkdir build && cd build
cmake ..
sudo make -j4
sudo make install
sudo udevadm control --reload-rules
sudo udevadm trigger
exec $SHELL
```

## Usage

Run `init.bash` in the `lab-scripts/data-collection` directory via
```
./init.bash
```
Once you see a blank terminal line hit enter once. This scripts starts the detector service and udp_capture processes for the hafx channels and x123. From there you can run the scripts like hafx_histogram.bash and x123_histogram.bash, the usage for which are in the github for lab-scripts/. The histogram data will be put into the lab-scripts/completed folder upon completion. From there you can take it off the pi via scp or work with it on the pi.

...

Once you are done with the detector quit.bash will stop the detector service and udp captures. You will see some lines printed out in the terminal, just hit enter.

## Detector Setup

## Miscellaneous Setup
The Bridgeport detectors need some settings configured each time they are run,
    namely the `fpga_ctrl` and `arm_ctrl` registers.
The easies way to get these registers is probably from the Bridgeport software.
Alternatively,
    you can send the flight code commands to read the data
    and save it to debug .bin.gz files.

Then, you will want to modify `lab-scripts/data-collection/first_boot.bash`
    with your registers and then run that.

**NOTE** that the IMPRESS FPGA ADC re-binning is set to pseudo-linear bins in this case.
`first_boot.bash` sends a new bin map to the SiPM-3000.
The bin map is generated based off of configuration files in the Python package.
During data analysis, the ADC bin map is undone to map the 123 IMPRESS histogram
    bins back to the original 4096 Bridgeport ADC bins.

### PPS Setup

PPS is required for nominal science mode to have very accurate timestamps.
However, it is not required if you want to "just" take a spectrum.

...

## Data Analysis
### Quick plotting for a "check-out"

Located in `python/examples/nominal-science-example` is a Jupyter notebook which shows how to read in health and time_slice science data.

Additionaly,
    the "checkout" script in `lab-scripts/util/plot_nominal_checkout.py`
    will accept a folder containing time_slice files and generate two plots:
    a spectrogram and a spectrum of the counts within those files.

### More in-depth analysis
The data is saved to gzipped binary files (i.e. filename.bin.gz).
The file will also have some prefix (hafx-time-slice, hafx-debug, ...) and a unix timestamp suffix, for example: 'hafx-debug-c1_2024-170-20-03-20_0.bin.gz'.
The python decoders are used to decode into json or plot from .bin.gz format.
Then you can run a decoder like a script from the command line.
There are decode options for decoding health,
    hafx science ("time-slice"), hafx debug, and x123 science data.
You can enter multiple files for decoding into one json. Example usage:
```
decode-impress-health health_file1.bin.gz health_file2.bin.gz ... health_fileN.bin.gz output_fn.json
decode-impress-hafx ...                       
decode-hafx-debug-hist ...
decode-x123-science ...
```

Alternatively, you may load the `.bin.gz` files directly into Python using the `umndet.common.helpers` functions.
Some examples are given in `python/examples`.
