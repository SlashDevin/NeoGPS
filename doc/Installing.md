Installing
==========
**1.**  For processing NMEA sentences from almost _any_ GPS manufacturer, copy all the .H and .CPP files from the top directory of NeoGPS to your application directory.  You do not need the files from any other subdirectories, like **ublox**.  Most of the example programs only use these generic NMEA files:
```
    CosaCompat.h
    GPSfix.h
    GPSfix_cfg.h
    NMEAGPS.cpp
    NMEAGPS.h
    NMEAGPS_cfg.h
    NeoGPS_cfg.h
    Streamers.cpp
    Streamers.h
    Time.cpp
    Time.h
```
**2.**  Next, review and edit each of the configuration files: NeoGPS_cfg.h, GPSfix_cfg.h, and NMEAGPS_cfg.h.  Alternatively, you can copy a set of these files from one of the [example configurations](../configs).</li>

**3.**  To try the example programs, you can copy the entire examples tree, or just one example subdirectory, into your Arduino directory.  Simply start the IDE by double-clicking on the INO file in those subdirectories.  However, the sad state of Arduino library management will still require you to copy the library files as decribed above into *each* example subdirectory.  :(

**Note:**  Not all configurations will work with all example applications.  Because each example application has its own copy of the configuration files, you can have separate configurations.

##ublox-specific NMEA messages
If you want to handle $PUBX messages from a ublox Neo GPS device, you must copy the above files *and* also copy the ublox/ubxNMEA.* files into your application directory.  (This is required if you are trying the example/PUBX/PUBX.ino application.)

##ublox-specific binary protocol
If you want to handle the UBX binary protocol from a ublox Neo GPS device, you must copy the above files *and* also copy the ublox/ubxGPS.* and ublox/ubxmsg.* into your application directory.  (This is required if you are trying the example/ublox/ublox.ino application.)

To use ublox-specific messages, you must enable the following:
```
#define NMEAGPS_DERIVED_TYPES
```

You may also want to change the configured PUBX messages in ubxNMEA.h, or the UBX binary messages in ubxGPS.h.  They are currently configured to work with the example applications PUBX.ino and ublox.ino, respectively.
