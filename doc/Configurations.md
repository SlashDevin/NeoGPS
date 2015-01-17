Configuration
=============
All configuration items are conditional compilations: a `#define` controls an `#if`/`#endif` section.

####gps_fix
The following configuration items are near the top of GPSfix.h:
```
// Enable/Disable individual parts of a fix, as parsed from fields of a $GPxxx sentence
#define GPS_FIX_DATE
#define GPS_FIX_TIME
#define GPS_FIX_LOCATION
#define GPS_FIX_ALTITUDE
#define GPS_FIX_SPEED
#define GPS_FIX_HEADING
#define GPS_FIX_SATELLITES
#define GPS_FIX_HDOP
#define GPS_FIX_VDOP
#define GPS_FIX_PDOP
#define GPS_FIX_LAT_ERR
#define GPS_FIX_LON_ERR
#define GPS_FIX_ALT_ERR
```
####NMEAGPS
The following configuration items are near the top of NMEAGPS.h:
```
// Enable/Disable parsing the fields of a $GPxxx sentence
#define NMEAGPS_PARSE_GGA
#define NMEAGPS_PARSE_GLL
#define NMEAGPS_PARSE_GSA
#define NMEAGPS_PARSE_GSV
#define NMEAGPS_PARSE_GST
#define NMEAGPS_PARSE_RMC
#define NMEAGPS_PARSE_VTG
#define NMEAGPS_PARSE_ZDA

// Although normally enabled, this can be disabled if you *do not*
// derive any classes from NMEAGPS, for slightly smaller/faster code.
#define NMEAGPS_DERIVED_TYPES

// Enable/disable tracking the current satellite array and,
// optionally, all the info for each satellite.
#define NMEAGPS_PARSE_SATELLITES
#define NMEAGPS_PARSE_SATELLITE_INFO

// Enable/disable accumulating fix data across sentences.
#define NMEAGPS_ACCUMULATE_FIX
```
####ubloxNMEA
This derived class has the following configuration items near the top of ubxNMEA.h:
```
#define NMEAGPS_PARSE_PUBX_00
#define NMEAGPS_PARSE_PUBX_04
```

GPSfix.cpp has a choice for using floating-point output.
```
#define USE_FLOAT
```
This is local to this file, and is only used by the example programs.  This file is not required, unless you need to stream a `gps_fix`.

Most example programs have a choice for displaying fix information once per day.  (Untested!)
```
#define PULSE_PER_DAY
```

Delete or comment out any items to be excluded from your build.  Where 
possible, checks are performed to verify that you have chosen a "valid" 
configuration: you may see `#error` messages in the build log.

####Troubleshooting
The compiler will catch any attempt to use parts of a `fix` that have been 
configured out: you will see something like `gps_fix does not have member 
xxx`.

The compiler **cannot** catch message set dependencies: the enum 
`nmea_msg_t` is always available.  So even though a `fix` member is enabled, 
you may have disabled all messages that would have set its value.  
NMEAtest.ino can be used to check some configurations.

####Typical configurations
A few common configurations are defined as follows

**Minimal**: no fix members, no messages (pulse-per-second only)

**DTL**: date, time, lat/lon, GPRMC messsage only.

**Nominal**: date, time, lat/lon, altitude, speed, heading, number of 
satellites, HDOP, GPRMC and GPGGA messages.

**Full**: Nominal plus VDOP, PDOP, lat/lon/alt errors, satellite array with satellite info, and all messages.

(**TinyGPS** uses the **Nominal** configuration + a second `fix`.)

