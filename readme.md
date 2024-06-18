# Detector flight code for use on the IMPISH piggyback

This code is taken straight from IMPRESS/EXACT flight code and
controls the Bridgeport SiPM-3000 boards and Amptek X-123 DP5 communication.

It is just the "controller" code which talks to the hardware and pipes the data around.

## How to build/install

Clone the repository;
    then, from this directory, do:
```bash
mkdir build
cd build
cmake ..
make -jN
```
**Replace `N` with the number of threads you want to use for building.**

You might need to install some libraries:
### packages from apt
```sh
sudo apt install libusb-1.0-0-dev gpiod libgpiod-dev libboost-all-dev libgtest-dev libsystemd-dev
```
