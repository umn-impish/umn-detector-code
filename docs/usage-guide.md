# How to install and run IMPRESS / EXACT / IMPISH flight code

This guide will show you how to setup the flight code on a newly-wiped Raspberry Pi.

We recommend running this code on a 64-bit Raspberry Pi 3 board.

Use the [Raspberry Pi imager](https://www.raspberrypi.com/software/) to set up a fresh install onto an SD card or
    via USB boot.

## Table of contents
1. [Installation](#installation)
3. [Detector hardware setup](#detector-setup)
2. [Usage](#usage)
4. [Data Analysis](#data-analysis)


## Installation

### Install libraries and clone the code
First, install required libraries on the 
```
sudo apt install libusb-1.0-0-dev gpiod libgpiod-dev libboost-all-dev libgtest-dev libsystemd-dev cmake build-essential git python3 python3-pip
```

Clone the flight code repository:
```
git clone https://github.com/umn-impish/umn-detector-code.git
```

For the flight code to run properly you will need to install this repo's associated Python package:
```
cd umn-detector-code/python
pip install -e . --break-system-packages
```

### Set up environment variables
Before you compile the code you will have to edit a file that sets up environment variables for the service to use.
You will edit the serial numbers in the file so the service can communicate with the Bridgeport boards.

First, find the serial numbers of the boards you want to use.
1. Plug the boards into a computer which has the [Bridgeport wxMCA](https://www.bridgeportinstruments.com/products/software.html)
   software installed.
2. Run `run_mds.cmd` or `run_mds.sh` and in the command line it will print 'Attached MCA' and then hex strings in brackets.
   These strings are the serial numbers of the Bridgeport boards. Write them down somewhere safe. 

Next open the file that stores the environment variables using [`nano`](https://linux.die.net/man/1/nano).

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

After compiling the code on the Pi,
    you need to perform some hardware setup which is described below.

## Detector Setup

## Miscellaneous Setup
The Bridgeport detectors need some settings configured each time they are run,
    namely the `fpga_ctrl` and `arm_ctrl` registers.
The easiest way to get these registers is probably from the Bridgeport software.
Alternatively,
    you can send the flight code commands to read the data
    and save it to debug .bin.gz files.

Then, you will want to modify `umn-detector-code/lab-scripts/data-collection/first_boot.bash`
    with your registers and then run that.
If your registers are already saved on the Bridgeport board's nonvolatile memory,
    then you can just comment out the lines which update `fpga_ctrl` and `arm_ctrl`
    in the `first_boot.bash` script.

**NOTE** that the IMPRESS FPGA ADC re-binning is set to an approximately linear mapping in this case.
When you run `first_boot.bash`,
    a new a new bin map is sent to the SiPM-3000.
The bin map is generated based off of configuration files in the Python package--
    see `umn-detector-code/python/umndet/common/constants.py`.
During data analysis, the ADC bin map is undone to map the 123 IMPRESS histoghttps://linux.die.net/man/1/scpram
    bins back to the original 4096 Bridgeport ADC bins.

`first_boot.bash` also sets the number of bins used by the X-123,
    so if you wish to take data with the X-123 you'll need to run the script.

### PPS Setup
#### _Use only 3.3V level PPS signals!_

A PPS is required for nominal science mode to have very accurate timestamps.
It is not required for the Bridgeport boards to function properly,
    but for nominal science mode,
    the X-123 requires a 1Hz hardware strobe to step through its data collection buffers.
We are using the
["hardware-controlled sequential buffer operation" (pg 219)](https://www.amptek.com/-/media/ametekamptek/documents/resources/products/user-manuals/amptek-digital-products-programmers-guide-b3.pdf)

**For the X-123**,
    you need to plug the PPS (or equivalent 1Hz strobe) into the AUX_IN_2 port on the back of it.
A pinout for the X-123 can be found [here, pg 6](https://www.amptek.com/-/media/ametekamptek/documents/resources/products/user-manuals/dp5-user-manual-b0.pdf).

**For the Bridgeport boards**,
    you need to plug the PPS (or equivalent 1Hz strobe)
    into [GPIO pin S0](https://www.bridgeportinstruments.com/products/sipm/sipm3k/sipm3k_pinout.pdf).
The cables that MSU and Lestat have made should have that connection already.

**For the Raspberry Pi**,
    the code is expecting a PPS to come in on GPIO pin 23,
    so make sure that pin is configured for input on the Pi.

## Usage
Once the hardware is set up and the software has been installed,
    you may begin data collection.

If you have not already, run the `first_boot.bash` script with the correct
    `fpga_ctrl` and `arm_ctrl` registers updated.
Or, you can comment those lines out if the Bridgeport board settings have already been
    properly configured in their nonvolatile memory.

Next, run `init.bash` in the `lab-scripts/data-collection` directory via
```
./init.bash
```
Once you see a blank terminal line hit enter once.
This scripts starts the flight controller process and `udp_capture` processes for the HaFX channels and X-123.

Next, run the `nominal_science.bash` script to take "nominal" science data.
You should see files start to fill up the `live` folder and
    eventually the `completed` folder.
After the data collection is done,
    copy the files off to another computer.
You can do that using a flash drive or [`scp`](https://linux.die.net/man/1/scp).

Once you are done with the detector,
    run the `quit.bash` script to kill the programs.

If you are having issues,
    you can view the output logs using the `view_logs.bash` script,
    or via [journalctl](https://man7.org/linux/man-pages/man1/journalctl.1.html) by-hand.

## Data Analysis
### Quick plotting for a "check-out"

Located in `umn-detector-code/python/examples/nominal-science-example`
    is a Jupyter notebook which shows how to read in health and time_slice science data.

Additionaly,
    the "checkout" script in `lab-scripts/util/plot_nominal_checkout.py`
    will accept a folder containing time_slice files and generate two plots:
    a spectrogram and a spectrum of the counts within those files.
You can run it like this:
```bash
# (cd into the right directory)
python plot_nominal_checkout.py /path/to/folder/with/gzipped/data
```

### More in-depth analysis
The data is saved to gzipped binary files (i.e. filename.bin.gz).
The file will also have some prefix (hafx-time-slice, hafx-debug, ...)
    and a timestamp suffix, for example: 'hafx-debug-c1_2024-170-20-03-20_0.bin.gz'.
You can decode the binary files into JSON if you'd like by using the decoder scripts.
There are decode options for decoding health,
    hafx science ("time_slice"), hafx debug, and x123 science data.
You can enter multiple files for decoding into one json. Example usage:
```
decode-impress-health health_file1.bin.gz health_file2.bin.gz ... health_fileN.bin.gz output_fn.json
decode-impress-hafx ...                       
decode-hafx-debug-hist ...
decode-x123-science ...
```

Alternatively, you may load the `.bin.gz` files directly into Python using the `umndet.common.helpers` functions.
Some examples are given in `python/examples`.
