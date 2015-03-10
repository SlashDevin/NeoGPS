NeoGPS
======

This fully-configurable Arduino library uses _**minimal**_ RAM, PROGMEM and CPU time, 
requiring as few as _**10 bytes of RAM**_, **866 bytes of PROGMEM**, and **less than 1mS of CPU time** per sentence.  

It supports the following protocols and messages:

####NMEA 0183
* GPGGA - GPS system fix data
* GPGLL - Geographic Latitude and Longitude
* GPGSA - GPS/GNSS DOP and active satellites
* GPGST - GNSS Pseudo Range Error Statistics
* GPGSV - GPS/GNSS Satellites in View
* GPRMC - Recommended Minimum specific GPS/Transit data
* GPVTG - Course over ground and Ground speed
* GPZDA - UTC Time and Date

Actually, _all_ Talker IDs are accommodated, not just GP.  That is, messages GNRMC, GLRMC, etc. will also be correctly parsed.  See discussion in [Configurations](doc/Configurations.md).

####u-blox NEO-6
#####NMEA 0183 Protocol Messages
* UBX,00 - Lat/Long Position Data
* UBX,04 - Time of Day and Clock Information

#####UBX Protocol Messages
* NAV_STATUS - Receiver Navigation Status
* NAV_TIMEGPS - GPS Time Solution
* NAV_TIMEUTC - UTC Time Solution
* NAV_POSLLH - Geodetic Position Solution
* NAV_VELNED - Velocity Solution in NED (North/East/Down)
* NAV_SVINFO - Space Vehicle Information

(This is the plain Arduino version of the [CosaGPS](https://github.com/SlashDevin/CosaGPS) library for [Cosa](https://github.com/mikaelpatel/Cosa).)

Goals
======
In an attempt to be reusable in a variety of different programming styles, this library supports:
* resource-constrained environments (e.g., ATTINY targets)
* sync or async operation (main `loop()` vs interrupt processing)
* event or polling (deferred handling vs. continuous fix() calls in `loop()`)
* single, fused or coherent fixes (multiple reports into one)
* optional buffering of fixes
* optional floating point
* configurable message sets, including hooks for implementing proprietary NMEA messages
* configurable message fields
* multiple protocols from same device
* any kind of input stream (Serial, SoftwareSerial, PROGMEM arrays, etc.)

Inconceivable!
=============

Don't believe it?  Check out these detailed sections:

Section  |  Description
-------- |  ------------
[Data Model](doc/Data Model.md) | Aggregating pieces into a *fix*
[Configurations](doc/Configurations.md) | Tailoring NeoGPS to your needs
[Performance](doc/Performance.md) | 33% faster!  Really!
[RAM requirements](doc/RAM.md) | Doing it without buffers!
[Program Space requirements](doc/Program.md) | Making it fit
[Extending NeoGPS](doc/Extending.md) | Using specific devices
[Tradeoffs](doc/Tradeoffs.md) | Comparing to other libraries
[Examples](doc/Examples.md) | Programming styles
[Acknowledgements](doc/Acknowledgements.md) | Thanks!
