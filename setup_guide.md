# How to install and run IMPRESS / EXACT / IMPISH flight code

This guide will show you how to setup the flight code on a fresh pi.

## Table of Contents
1. [Installation](#installation)
2. [Usage](#usage)
3. [PPS Setup](#pps-setup)
4. [Data Analysis](#data-Analysis)


## Installation

First install necessary libraries:
```
sudo apt install libusb-1.0-0-dev gpiod libgpiod-dev libboost-all-dev libgtest-dev libsystemd-dev
```

Install git and python/pip:
```
sudo apt install git
sudo apt install python3
```

Clone the flight code repository:
```
git clone https://github.com/umn-impish/umn-detector-code.git
```

Cd into the flight code directory and compile the code:
```
cd ~/umn-detector-code/flight-controller
mkdir build && cd build
cmake ..
make -j4
```

You will also have to move the usb.rules file into /etc/udev/rules.d/. This will give all usb devices read write access to the pi.
```
sudo mv ~/umn-detector-code/flight-controller/install/usb.rules /etc/udev/rules.d/
```

Now that the code is compiled you will have to get the Serial numbers of the bridgeport boards being used:

*words here*

Once you have the serial numbers you will have to edit the 'launch_detector.bash' script which starts up the detector service. Open it with nano:
```
nano ~/umn-detector-code/lab-scripts/launch-detector.bash
```
From there you will see these lines:
```
c1_serial_number="55FD9A8F4A344E5120202041131E05FF"
m1_serial_number="none"
m5_serial_number="none"
x1_serial_number="none"
```
The given serial number is the serial number of the board used at UMN (as of 7/24). Change these entries to be the serial numbers of the boards that you got from above. If you are using one board just change the c1 serial number and leave the rest.
Save it via ctrl-x, typing y, and hitting enter once.

## Usage

Run init.bash via
```
./init.bash
```
Once you see a blank terminal line hit enter once. This scripts starts the detector service and udp_capture processes for the hafx channels and x123. From there you can run the scripts like hafx_histogram.bash and x123_histogram.basg, the usage for which are in the github for /lab-scripts/. The histogram data will be put into the lab-scripts/downlink folder upon completion. From there you can take it off the pi via scp or work with it on the pi.

...

Once you are done with the detector quit.bash will stop the detector service and udp captures. You will see some lines printed out in the terminal, just hit enter.

## PPS Setup

PPS is requited for nominal science mode. In order to set it up...

## Data Analysis

The data is given in the form of gzip binary files (i.e. filename.bin.gz). The will also have some prefix (hafx_histgram, hafx_debug, ...) and a unix timestamp suffix, for example: 'hafx-debug-c1_2024-170-20-03-20_0.bin.gz'. The python decoders are used to decode into json or plot from .bin.gz format. To decode into json, run the json_decoders.py script in the /python/ subdirectory. You will have to install the python package William made.
```
cd ~/umn-detector-code/python
pip install -e . --break-system-packages
```
Then you can use it like a normal python script:
```
python3 json_decoders.py *input_file_name(s).bin.gz* *output_file_name.json*
```
You can enter multiple .bin.gz files at once to decode into one .json file, for example:
```
python3 json_decoders.py histogram1.bin.gz histogram2.bin.gz histograms.json
```
