# Detector/EXACT setup guide
Assumes you have the following equipment:
- DS3231 real-time clock or other 1Hz square wave source
- Bridgeport board + SiPMs + scintillator with the NRL list mode firwmare
- RPi CM3+ or other compatible device

The steps are in `## heading 2` size

## flash a microsd with rpi os lite 64 bit using rpi imager
You can download the RPi imager from its website: [imager](https://www.raspberrypi.com/software/)

## boot into the Pi and set up the timezone and localization options via `raspi-config`
- timezone: chicago
- keyboard: generic, US layout
- locale: `EN_US`
- to pick the locale you need to use space bar, kind of weird but yeah

While you're at it, make a couple more changes in raspi-config:
- enable the SSH server
- enable the i2c interface

## 3. set a static IP address for the Pi using nmtui
- run `sudo nmtui`
- hit `Edit a connection`
- Wired connection 1
- Edit
- ipv6: disable
- ipv4: set to manual
- addresses: 192.168.2.8/24
- gateway: 192.168.2.1
- DNS: 192.168.2.1

test by pinging 1.1.1.1
should reply with bytes
if it doesn't work, double check things are configured properly

**when the network is set up, you can connect via SSH so it's easier to copy and paste**

## 4. install required packages; this will take a few minutes
```bash
sudo apt update
sudo apt upgrade -y
sudo apt install git cmake gcc g++ build-essential libboost-thread-dev libusb-1.0-0-dev libgtest-dev libsystemd-dev libgpiod-dev python3-smbus
```

## 5. clone and build the umn-detector-code
```bash
git clone https://github.com/umn-impish/umn-detector-code.git
cd umn-detector-code/flight-controller
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j3
sudo make install
```

## 6. once the flight utility programs are installed, we need to configure the DS3231 real-time clock to work as a PPS source for testing.
If you are yousing a real GPS for the PPS, the configuration will be completely different.

The RTC data sheet is here: [DS3231 data sheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf).
It contains a pinout for the chip if you want to start from scratch.

In lab we have it set up on a bread board.
It is powered from the Pi I/O board,
    and the SDA and SCL I2C lines are connected appropriately.
The SQW output from the data sheet is connected to 3.3V via a pull-up resistor,
    and the output available for other devices to connect to.
The SQW output needs to go to GPIO S0 on the detector,
    and to GPIO pin 31 on the Pi.
![rtc](rtc.png)

run `i2cdetect -y 1` to make sure the clock is properly connected.
You should see address 68 pop up.

To configure its square wave output, run the following Python snippet on the Pi.
Save it to a file and then run it from the shell. Thanks, ChatGPT
```py
import smbus

# Constants
I2C_BUS = 1                # Default I2C bus on CM3+
DS3231_ADDRESS = 0x68      # I2C address of the DS3231
REG_CONTROL = 0x0E         # Control register
REG_STATUS = 0x0F          # Status register

# Square wave rate options:
# 00 = 1 Hz, 01 = 1024 Hz, 10 = 4096 Hz, 11 = 8192 Hz
RS1 = 0
RS2 = 0

# Enable square wave output:
# EOSC = 0 (oscillator on)
# BBSQW = 0 (not needed unless Vbat only)
# CONV = 0 (no conversion)
# RS2/RS1 = 00 (1 Hz)
# INTCN = 0 (square wave mode)
control_byte = (RS2 << 4) | (RS1 << 3)

bus = smbus.SMBus(I2C_BUS)

# Read current control register value (optional)
original_control = bus.read_byte_data(DS3231_ADDRESS, REG_CONTROL)
print(f"Original control register: 0x{original_control:02X}")

# Write new control value to enable 1 Hz square wave
bus.write_byte_data(DS3231_ADDRESS, REG_CONTROL, control_byte)

# Verify new control value
new_control = bus.read_byte_data(DS3231_ADDRESS, REG_CONTROL)
print(f"New control register: 0x{new_control:02X}")

bus.close()
```

Once you run the snippet,
    check that the square wave output is working with either an oscilloscope or multimeter.

## 7. run the program tests
Go back to `umn-detector-code/flight-controller/build` if you left it.
Then run `make test` to run some program tests.
Most of them should pass, except for the `sipm3k.TimeSliceRead`, if everything
is configured properly.
If some tests fail, go back and double check everything.

## 8. go to the lab-scripts folder and run some scripts
For NRL data acquistion,
    a 1Hz square wave needs to be connected to GPIO 31 on the Pi,
    and to the GPIO S0 on the Bridgeport board.
The 1Hz strobe synchronizes the system clock and data.

To get some data, do the following

In one terminal, run the `view_logs` script and monitor it for issues
```bash
cd umn-detector-code/lab-scripts/data-collection
./view_logs.bash
```

In another terminal, run the scripts to start data acquisition
```bash
cd umn-detector-code/lab-scripts/data-collection/exact_specific

# Start relevant programs
./nrl_init.bash

# i'm just using c1 as the identifier here, depending on the envars.bash file,
# you can use c1, m1, m5, or x1
./start_nrl_list.bash c1
```

If you see errors in `view_logs`, particularly a `std::out_of_range`, then
the serial numbers configured in `envars.bash` are probably wrong and you need to
go in and update them. Quit the programs using `quit.bash`, update the envars,
and try again.

You should see something along these lines print out in the logs
```
Jul 18 14:06:03 raspberrypi det-controller[3820]: Serial numbers available
Jul 18 14:06:03 raspberrypi det-controller[3820]: serial number: 7A65CD294A344E51202020412B2404FF
Jul 18 14:06:03 raspberrypi det-controller[3820]: Destructing LibUsb::DeviceList
Jul 18 14:06:03 raspberrypi det-controller[3820]: udp port is: 61000
Jul 18 14:06:03 raspberrypi det-controller[3820]: udp port is: 61000
Jul 18 14:06:03 raspberrypi det-controller[3820]: udp port is: 61001
Jul 18 14:06:03 raspberrypi det-controller[3820]: Constructing LibUsb::Context
Jul 18 14:06:03 raspberrypi det-controller[3820]: Constructing LibUsb::DeviceHandle from existing handle
Jul 18 14:06:03 raspberrypi det-controller[3820]: Destructing LibUsb::Context
Jul 18 14:06:03 raspberrypi det-controller[3820]: USB issue: Existing handle is null (not connected?)
Jul 18 14:06:03 raspberrypi det-controller[3820]: udp port is: 61008
Jul 18 14:06:03 raspberrypi det-controller[3820]: udp port is: 61009
Jul 18 14:06:03 raspberrypi det-controller[3820]: X-123 disconnected; using 1024 bins as default
Jul 18 14:06:13 raspberrypi det-controller[3820]: 1 is full
Jul 18 14:06:17 raspberrypi det-controller[3820]: 0 is full
Jul 18 14:06:21 raspberrypi det-controller[3820]: 1 is full
Jul 18 14:06:24 raspberrypi det-controller[3820]: 0 is full
Jul 18 14:06:28 raspberrypi det-controller[3820]: 1 is full
Jul 18 14:06:31 raspberrypi det-controller[3820]: 0 is full
Jul 18 14:06:35 raspberrypi det-controller[3820]: 1 is full
Jul 18 14:06:38 raspberrypi det-controller[3820]: 0 is full
Jul 18 14:06:42 raspberrypi det-controller[3820]: 1 is full
Jul 18 14:06:45 raspberrypi det-controller[3820]: 0 is full
Jul 18 14:06:48 raspberrypi det-controller[3820]: 1 is full
Jul 18 14:06:54 raspberrypi det-controller[3820]: 0 is full
Jul 18 14:07:00 raspberrypi det-controller[3820]: 1 is full
Jul 18 14:07:05 raspberrypi det-controller[3820]: Destructing LibUsb::DeviceHandle
Jul 18 14:07:05 raspberrypi det-controller[3820]: Destructing LibUsb::Context
Jul 18 14:07:05 raspberrypi det-controller[3820]: detector sleep
```

The data is output into the `live` folder and after a time delay, it is gzipped and moved to the `completed` folder.

You can copy the files to your computer using `scp`.
