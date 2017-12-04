NeoGPS support for u-blox GPS devices
=================
To use either ublox-specific NMEA messages ($PUBX) or the UBX binary protocol, you must enable the following in `NMEAGPS_cfg.h`:
```
#define NMEAGPS_PARSE_MFR_ID

#define NMEAGPS_DERIVED_TYPES
```

## ublox-specific NMEA messages
**NeoGPS** implements the following additional NMEA messages:

#NMEA 0183 Proprietary Messages
* UBX,00 - Lat/Long Position Data
* UBX,04 - Time of Day and Clock Information

You may want to change the configured PUBX messages in `ubxNMEA.h`.  It is currently configured to work with the example application, `PUBX.ino`.

The derived class `ubloxNMEA` has the following configuration items near the top of ubxNMEA.h:
```
#define NMEAGPS_PARSE_PUBX_00
#define NMEAGPS_PARSE_PUBX_04
```

## ublox-specific binary protocol

**NeoGPS** implements the following messages in the UBX binary protocol:

#UBX Protocol Messages

* NAV_STATUS - Receiver Navigation Status
* NAV_TIMEGPS - GPS Time Solution
* NAV_TIMEUTC - UTC Time Solution
* NAV_POSLLH - Geodetic Position Solution
* NAV_VELNED - Velocity Solution in NED (North/East/Down)
* NAV_SVINFO - Space Vehicle Information

You may want to change the configured UBX messages in `ubx_cfg.h`.  It is currently configured to work with the example application `ublox.ino`.

The configuration file `ubx_cfg.h` has the following configuration items near the top of the file:
```
#define UBLOX_PARSE_STATUS
#define UBLOX_PARSE_TIMEGPS
#define UBLOX_PARSE_TIMEUTC
#define UBLOX_PARSE_POSLLH
//#define UBLOX_PARSE_DOP
#define UBLOX_PARSE_VELNED
#define UBLOX_PARSE_SVINFO
```

**Note:** Disabling some of the UBX messages may prevent the `ublox.ino` example sketch from working.  That sketch goes through a process of first acquiring the current GPS leap seconds and UTC time so that "time-of-week" milliseconds can be converted to a UTC time.

The POSLLH and VELNED messages use a Time-Of-Week timestamp.  Without the TIMEGPS and TIMEUTC messages, that TOW timestamp cannot be converted to a UTC time.

* If your application does not need the UTC date and/or time, you could disable the TIMEGPS and TIMEUTC messages.

* If your application does not need latitude, longitude or altitude, you could disable the POSLLH message.

* If your application does not need speed or heading, you could disable the VELNED message.

* If your application does not need satellite information, you could disable the SVINFO message.
