# Detector program commands: IMPRESS + EXACT + IMPISH
All commands listed in this document may be sent to the
    IMPRESS/EXACT/IMPISH flight controller program via UDP packets.
They are assumed to be strings with no extra data attached.

Each command responds to the sender with either `ack-ok\nmessage` or `error\nmessage`.
If an error occurs you may check the `systemd` logs for more details.

Commands are listed and their arguments described, where appropriate.

All commands time out after 30s.
Timed acquisitions (like a debug histogram) do not block;
    the command response indicates whether or not initialization was successful.
To get the data saved by such a command,
    you need to fetch the appropriate "debug" data once it has been written.
If the data is sent to `udp_capture` to be saved,
    it may be retrieved as a specific file.

# Command listing

## `terminate`
Kills the controller process.


## `sleep`
Stops any current data acquisitions and frees the USB connections to the detectors.


## `wake`
Initializes the detectors (USB + settings send/recv) and connects to them.
Command assumes all desired detectors are powered.
If you want to power on a new detector you need to first `sleep`
    the controller program and then `wake` it up.


## `start-nominal`
Starts nominal data collection.
In the case of IMPRESS,
    this starts time slice acquisitions and
    enables the X-123 hardware-buffered acquisitions.
In the case of EXACT, starts list mode acquisitions.


## `stop-nominal`
Stops the nominal data collection.


## `start-periodic-health seconds_between address1 [address2] ... [addressN]`
Tell the detector process to periodically send out health packets.
Can be sent to arbitrary number of addresses specified in the command.
**Parameters**:
- `seconds_between`: time to wait between sending out health packets
- `address1...N`: IP addresses (like `a.b.c.d:port`) to send the health packets to.
   at least one is required.


## `stop-periodic-health`
Tell the detector process to stop sending out its health data.
Erases previous forwarding addresses.


## `settings-update [detector] . . .`
Modify detector settings (custom ones and "built-in" ones).
### Bridgeport detectors settings
    settings-update hafx c1 ...
    settings-update hafx m1 ...
    settings-update hafx m5 ...
    settings-update hafx x1 ...
Here "..." can be any one of the following:
- `adc_rebin_edges [bins]`:
    Custom edges we send to maps from the 4096 native Bridgeport
    ADC bins to the 123 `time_slice` ADC bins.
    The data format is described in the IMPRESS firmware document.
- `fpga_ctrl [registers]`: 
    fpga_ctrl registers (defined by Bridgeport)
- `arm_ctrl [registers]`:
    arm_ctrl registers (defined by Bridgeport)
- `arm_cal [registers]`:
    arm_cal registers (defined by Bridgeport)
- `fpga_weights [registers]`:
    fpga_weights registers (defined by Bridgeport)

Appropriate arguments must be supplied after the settings keyword.

#### Example:
    settings-update hafx c1 fpga_ctrl 48000 3 4 60 48 65280 100 1092 610 0 0 17 512 0 16399 128

### X-123 settings
    settings-update x123 [options ...]
#### Valid `[options ...]`:
- `ascii_settings [string to write]`
    These are the "built-in" X-123 settings that Amptek provides.
    The "string to write" format should follow the Amptek format like this:
        `settings-update x123 ascii_settings MCAC=256;OTHER=blah;DINGUS=bingus;`

- `adc_rebin_edges [edge 1] [edge 2] ... [edge N]`
    We want to be able to rebin our X-123 spectrum down adaptively.
    This should be an array of ADC bin edges to bin down the nominal X-123 spectrum
    to a smaller value.
        example:
        `settings-update x123 adc_rebin_edges 0 512 1024`
    The above example rebins the x-123 spectrum to three very big
    bins, assuming at least a 1024 bin spectrum acquisition.

- `ack_err_retries [retries_val]`
    Sometimes the X-123 replies with an "ACK ERROR" packet
    even when nothing catastropic happens.
    If we get an "ACK ERROR" packet from the X-123,
    we want to retry a couple times before giving up.
    This parameter enables this.

#### Example:
    settings-update x123 ack_err_retries 3


## Debug
There are debug commands for the Bridgeport and X-123 detectors.
They let you query speific pieces of data and save them to files.

    debug [detector] [options...]
Request a "debug" acquisition for a particular detector.
This could be querying a particular setting or set of settings,
or a histogram or list mode acquisition.

### Bridgeport debug format
    debug hafx [channel] [operation] (wait_time)
- `channel`:
    HaFX channel to operate on: `c1, m1, m5, x1`

- `operation`: What to debug. Valid are:
    - `arm_ctrl`
    - `arm_cal`
    - `arm_status`
    - `fpga_ctrl`
    - `fpga_statistics`
    - `fpga_weights`
    - `histogram`
    - `time_slice`
    - `list_mode`

- `wait_time` is only used for data acquisitions like histogram or list mode.
Units: seconds.
Says how long to wait before polling the detector after starting data acquisition.
Max value: 3600s (one hour)
Min value: 1s

**Example:**
Take a "normal" histogram for 10s.

    debug hafx c1 histogram 10

### X123 debug format:

    debug x123 [operation] (operation params)
- `operation`: What to query from the X-123. Valid are:
    - `histogram`
    - `diagnostic`
    - `ascii_settings`

- `(operation params)`: Only ascii_settings and histogram have these options. These vary depending on the type.
    - Examples:
    - Debug a histogram with a 55s collection time:

            debug x123 histogram 55
    - Query the MCAC (# of bins used by the Multi-Channel Analyzer [MCA]) setting from the X-123

            debug x123 ascii_settings MCAC=;
