Installing
==========
For processing NMEA sentences from almost _any_ GPS manufacturer, copy all the .H and .CPP files from the top directory of NeoGPS to your application directory.  You do not need the files from any other subdirectories, like **ublox**.  Most of the example programs only use these generic NMEA files.

If you want to handle $PUBX messages from a ublox Neo GPS device, you must copy the above files *and* also copy the ublox/ubxNMEA.* files into your application directory.  (This is required if you are trying the example/PUBX/PUBX.ino application.)

If you want to handle the UBX binary protocol from a ublox Neo GPS device, you must copy the above files *and* also copy the ublox/ubxGPS.* and ublox/ubxmsg.* into your application directory.  (This is required if you are trying the example/ublox/ublox.ino application.)

You can copy the entire example tree into your Arduino directory, and start the IDE by double-clicking on the INO file from those subdirectories.  However, the sad state of Arduino library management will still require you to copy the library files as decribed above into *each* example subdirectory.  :(

There are also several example configurations in the [config](/config) subdirectory.  Not all configurations will work with all example applications.  Specifically, PUBX.ino and ublox.ino both require the following to be enabled in **NMEAGPS_cfg.h**:

```
#define NMEAGPS_DERIVED_TYPES
```

You may also want to change the configured PUBX messages in ubxNMEA.h, or the UBX binary messages in ubxGPS.h.  They are currently configured to work with the example applications PUBX.ino and ublox.ino, respectively.
