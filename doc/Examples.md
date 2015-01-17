Examples
======
Several programs are provided to demonstrate how to use the classes in these different styles:

* [NMEA](examples/NMEA/NMEA.ino) - sync, polled, single, standard NMEA only
* [NMEAblink](examples/NMEAblink/NMEAblink.ino) - sync, polled, single, standard NMEA only, minimal example, only blinks LED
* [NMEAfused](examples/NMEAfused/NMEAfused.ino) - sync, polled, fused, standard NMEA only
* [NMEAcoherent](examples/NMEAcoherent/NMEAcoherent.ino) - sync, polled, coherent, standard NMEA only
* [PUBX](examples/PUBX/PUBX.ino) - sync, polled, coherent, standard NMEA + ublox proprietary NMEA
* [ublox](examples/ublox/ublox.ino) - sync, polled, coherent, ublox protocol

Preprocessor symbol `USE_FLOAT` can be used in [Streamers.cpp](Streamers.cpp) to select integer or floating-point output.

A self-test test program is also provided:

* [NMEAtest.ino](examples/NMEAtest/NMEAtest.ino) - sync, polled, not fused, standard NMEA only

No GPS device is required; the bytes are streamed from PROGMEM character arrays.  Various strings are passed to `decode` and the expected pass or fail results are displayed.  If **NeoGPS** is correctly configured, you should see this on your SerialMonitor:

```
NMEA test: started
fix object size = 44
NMEAGPS object size = 82
Test string length = 75
PASSED 6 tests.
------ Samples ------
Results format:
  Status,Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,VDOP,PDOP,Lat err,Lon err,Alt err,Sats,[sat],Rx ok,Rx err,

Input:  $GPGGA,092725.00,4717.11399,N,00833.91590,E,1,8,1.01,499.6,M,48.0,M,,0*5B
Results:  3,2000-00-00 09:27:25.00,472852332,85652650,,,49960,1010,,,,,,8,[],1,0,

Input:  $GPRMC,092725.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*5E
Results:  3,2002-12-09 09:27:25.00,472852395,85652537,7752,4,,,,,,,,,[],2,0,

Input:  $GPGGA,064951.000,2307.1256,N,12016.4438,E,1,8,0.95,39.9,M,17.8,M,,*63
Results:  3,2002-12-09 06:49:51.00,231187600,1202740633,,,3990,950,,,,,,8,[],3,0,

Input:  $GPRMC,064951.000,A,2307.1256,N,12016.4438,E,0.03,165.48,260406,3.05,W,A*2C
Results:  3,2006-04-26 06:49:51.00,231187600,1202740633,16548,30,,,,,,,,,[],4,0,

Input:  $GPVTG,165.48,T,,M,0.03,N,0.06,K,A*36
Results:  3,,,,16548,30,,,,,,,,,[],5,0,

Input:  $GPGSA,A,3,29,21,26,15,18,09,06,10,,,,,2.32,0.95,2.11*00
Results:  3,,,,,,,950,2110,2320,,,,,[],6,0,

Input:  $GPGSV,3,1,09,29,36,029,42,21,46,314,43,26,44,020,43,15,21,321,39*7D
Results:  ,,,,,,,,,,,,,9,[],7,0,

Input:  $GPGSV,3,2,09,18,26,314,40,09,57,170,44,06,20,229,37,10,26,084,37*77
Results:  ,,,,,,,,,,,,,9,[],8,0,

Input:  $GPGSV,3,3,09,07,,,26*73
Results:  ,,,,,,,,,,,,,9,[29,21,26,15,18,9,6,10,7,],9,0,
```

