Troubleshooting
===============

##Configuration errors
Because there are so many configurable items, it is possible that your configuration prevents acquiring the desired GPS information.

The compiler **cannot** catch message set dependencies: the enum 
`nmea_msg_t` is always available.  So even though a `fix` member is enabled, 
you may have disabled all messages that would have set its value.  
NMEAtest.ino can be used to check some configurations.

For example, if your application needs altitude, you **must** enable the GGA sentence.  No other sentence provides the altitude member.  If `NMEA_PARSE_GGA` is not defined,  `gps.decode()` will return COMPLETED after a GGA is received, but no parts of the GGA sentence will have been parsed, and altitude will never be valid.  NeoGPS will _recognize_ the GGA sentence, but it will not be parsed.

The compiler will catch any attempt to use parts of a `fix` that have been 
configured out: you will see something like `gps_fix does not have member 
xxx`.

There are also compile-time checks to make sure the configuration is valid.  For example, if you enable `NMEAGPS_PARSE_TALKER_ID` so that you can handle GLONASS messages differently than GPS messages, you *must* enable `NMEAGPS_DERIVED_TYPES`.  An error message will tell you to do that.  Until you disable `NMEAGPS_PARSE_TALKER_ID` **or** enable `NMEAGPS_DERIVED_TYPES`, it will not compile.

##GPS device connection problems
You can use the NMEA.INO example program to verify that your GPS device is correctly connected and operating.

If the GPS device is not correctly connected, or it **does not** have a fix yet, you will see lines of empty data:
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,
,,,,,,,,
,,,,,,,,
,,,,,,,,
```
This does not tell you whether you are using the correct pins, serial port, baud rate, or whether you are receiving any data.  Uncomment this line in NMEGPS_cfg.h:
```
   #define NMEAGPS_STATS
```
Rebuild the sketch, and you should see additional data fields:
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,Rx ok,Rx err,Rx chars,
,,,,,,,,0,0,0,
,,,,,,,,0,0,0,
```
This shows that **no** data is being received: no characters and no sentences.  You may have the GPS device connected to the wrong pins (GPS RX should be connected to Arduino TX, and GPS TX should be connected to Arduino RX), or the .INO may be using the wrong serial object: review the comments at the top of the example program for the expected connections.

If you are seeing
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,Rx ok,Rx err,Rx chars,
,,,,,,,,0,3,181,
,,,,,,,,0,1,422,
```
You may be using the wrong baud rate.  This says that characters are being received (Rx chars), but none of them were decoded successfully (Rx err).  Review your GPS device's documentation to find the correct baud rate, or try different values in `setup()`:
```
  // Start the UART for the GPS device
  gps_port.begin(4800); // 2400, 19200, 38400, and 115200 are also possible
```
If you are seeing
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,Rx ok,Rx err,Rx chars,
,0,,,,,,,3,0,259,
,0,,,,,,,6,0,521,
```
This means that the GPS device is correctly connected, and the sketch is receiving complete NMEA sentences, without data errors.

Because the fields are all empty, the GPS device is not receiving signals from any satellites.  Check the antenna connection and orientation, and make sure you have an unobstructed view of the sky: go outside for the best reception, although getting close to a window may help.

Although different GPS devices behave differently when they do not have a fix, you may be able to determine how many satellites are being received from what is being reported.

As soon as the GPS device receives a signal from just one satellite, the date and time will be reported:
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,Rx ok,Rx err,Rx chars,
2015-09-14 15:07:30,1,2015-09-14 19:07:30.00,,,,,,3,0,259,
2015-09-14 15:07:31,1,2015-09-14 19:07:30.00,,,,,,3,0,521,
```
If it continues to report only date and time, you do not have an obstructed view of the sky: only one satellite signal is being received.

When signals from three satellites are being received, the GPS device will start reporting latitude and longitude.
When signals from four or more satellites are being received, the GPS device will start reporting altitude.

You can also enable other `gps_fix` fields or NMEA sentences.  In **GPSfix_cfg.h**, uncomment this line:
```
#define GPS_FIX_SATELLITES
```
In **NMEAGPS_cfg.h** uncomment these lines:
```
#define NMEAGPS_PARSE_GGA
#define NMEAGPS_PARSE_GSA
#define NMEAGPS_PARSE_GSV

#define NMEAGPS_PARSE_SATELLITES
#define NMEAGPS_PARSE_SATELLITE_INFO
#define NMEAGPS_MAX_SATELLITES (40)

#define NMEAGPS_ACCUMULATE_FIX
```
This will display additional fields for how many satellites are in view, whether they are being "tracked", and their individual signal strengths.
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,VDOP,PDOP,Lat err,Lon err,Alt err,Sats,[sat elev/az @ SNR],Rx ok,Rx err,Rx chars,
2015-09-14 16:03:07,3,2015-09-14 20:03:07.00,,,,,,,,,,,,2,[2 71/27@14,5 65/197@33,],8,0,476,
2015-09-14 16:03:08,3,2015-09-14 20:03:08.00,,,,,,,,,,,,2,[2 71/27@14,5 65/197@33,],16,0,952,
```
This shows that only two satellites are being tracked.  You must move to a position with a better view of the sky.
