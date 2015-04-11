####u-blox NEO-6
#####NMEA 0183 Proprietary Messages
* UBX,00 - Lat/Long Position Data
* UBX,04 - Time of Day and Clock Information

#####UBX Protocol Messages
* NAV_STATUS - Receiver Navigation Status
* NAV_TIMEGPS - GPS Time Solution
* NAV_TIMEUTC - UTC Time Solution
* NAV_POSLLH - Geodetic Position Solution
* NAV_VELNED - Velocity Solution in NED (North/East/Down)
* NAV_SVINFO - Space Vehicle Information

####ubloxNMEA
This derived class has the following configuration items near the top of ubxNMEA.h:
```
#define NMEAGPS_PARSE_PUBX_00
#define NMEAGPS_PARSE_PUBX_04
```

