# Detector configurations using flight code

This document outlines how to change the configurations of the HaFX and X-123 detectors
    using flight code utilities.

It assumes that the `det-controller` process is already running.
Follow directions in either of the `-usage.md` guides for how to get this running.

Detailed documentation on the settings commands is available in
    `detector-commands.md`.

## Prerequesites
To make use of this guide, you need to know a little bit about how to use the shell.
Here are a few resources that describe some important aspects.
Please watch/read them if you think it would be helpful.
- [Intro video (youtube)](https://www.youtube.com/watch?v=KZZdof6RvP0)
- [Bash tutorial on w3schools (probably overkill but might be useful)](https://www.w3schools.com/bash/). Focus on the "Basic Commands" section at least.
- [Bash/linux learning game on OverTheWire](https://overthewire.org/wargames/bandit/).
This one is quite the time sink,
    but if you want to become proficient at Linux,
    it's a great way to learn.

Make sure the detectors are powered and connected via USB.

## X-123 settings updates
### How to query current X-123 settings
To query the current X-123 settings, you send a `debug` command to the
    `det-controller` process.
The process will print some information to the systemd logs,
    and output a debug file.

All possible X-123 ASCII configuration settings can be found
    in section 5 of the DP5 Programmer's Guide,
    which you can find via Google.

**Important**: the maximum readback size is 512 bytes.
To dump the entire X-123 configuration,
    you will need to send multiple debug read commands sequentially.

#### Querying X123 settings
Here is an example of how to query the number of MCA bins (MCAC),
    the slow threshold (THSL),
    the gain (GAIN),
    and the fast threshold (THFA) values:
```bash
# Assumes that the det-controller has been launched by running init.bash
# Assumes you are in the `lab-scripts/data-acquisition` subdirectory

# In one window, run the view_logs scripts
./view_logs.bash

# In a **separate window**, run these commands
# Import (source) the variables from udpcap_ports.bash
source udpcap_ports.bash

# Send a query command to the detector to check settings.
# These will be printed in the systemd journal (view_logs.bash),
# and saved to a debug .bin.gz file in the `completed` directory.
echo 'debug x123 ascii_settings MCAC=;THFA=;THSL=;GAIN=;' > $det_udp_dev
```

At this point,
    check the current ASCII configurations for the detector,
    either by looking at the logs or by copying and decoding the debug
    file.
You can decode the debug file much like in the
`python/examples/Read X123 debug data.ipynb` file,
    except the output data won't be a histogram,
    it will be an ASCII string showing the settings you queried.

#### Setting X123 settings
To configure the X-123,
    you run commands similar to above,
    but you need to supply arguments to the ASCII string.

**Important**: the maximum size of the string sent to the X-123 is
$2^{16}$ characters.
This should be more than enough to send all of the configuration data,
    but it's important to keep this in mind.

Here is how to update the MCAC, THFA, THSL, and GAIN settings simultaneously:
```bash
# Assumes that the det-controller has been launched by running init.bash
# Assumes you are in the `lab-scripts/data-acquisition` subdirectory

# In one window, run the view_logs scripts
./view_logs.bash

# In a **separate window**, run these commands
# Import (source) the variables from udpcap_ports.bash
source udpcap_ports.bash

# Send the updated settings using a command to the controller program
# Notice that there are no spaces or quotes surrounding the configuration string.
# Do not put spaces in the configuration string.
echo 'settings-update x123 ascii_settings MCAC=1024;THFA=22;THSL=0.378;GAIN=10;' > $det_udp_dev

# (Optional) read the settings back out to verify they changed as you wanted
echo 'debug x123 ascii_settings MCAC=;THFA=;THSL=;GAIN=;' > $det_udp_dev
```

## HaFX settings updates
`TODO`