## Library list
### `logging`
Logs to the `systemd` central logger which can be
    viewed using e.g.
    `journalctl --lines=100 _COMM='det-controller'`.
See `man journalctl` for more info.

### `det-messages`
Commands and data products that the detector code can
    process and/or save.
Some can be sent from the ground,
    while some are simply meant for internal functional use.

### `det-support`
Various classes and other code relating to data saving via
    UDP sockets,
    as well as common exception classes,
    and a way to save/read settings.

### `sipm3k-interface`
Code for interfacing with SiPM-3000 detectors from Bridgeport.
Tests show basic usage with `SingleDevManager`.

New commands may be added by inheriting from `IoContainer<>`.

### `x123-interface`
Code to talk to the Amptek X-123.
Allows for sending/receiving X-123 packets.
Automatically validates the proper checksum,
    ensures correct data length,
    verifies the command ID,
    and handles/throws "ack" packet errors properly.

### `hafx-ctrl`
Wrapper around the `sipm3k-interface` that supports higher-level
    operations and data saving.
Incorporates the interface along with data savers from
    `det-support`.

### `x123-ctrl`
Same idea as `hafx-ctrl` but for the X-123.

### `det-service`
Contains the logic for how we want the satellite to respond to commands.
Arguably this is the most important "layer" of the bunch.
Can only be communicated with via a thread safe queue and promises/futures.

### `det-controller`
This program is what other programs can talk to.
It receives and decodes text commands and pushes them onto the DetectorService queue.
It replies with "ok" if nothing goes wrong and says when something does go wrong.
