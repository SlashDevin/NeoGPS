NeoGPS
======

This fully-configurable Arduino library uses _**minimal**_ RAM, PROGMEM and CPU time, 
requiring as few as _**10 bytes of RAM**_, **866 bytes of PROGMEM**, and **less than 1mS of CPU time** per sentence.  

It supports the following protocols and messages:

####NMEA 0183
* GPGGA - System fix data
* GPGLL - Geographic Latitude and Longitude
* GPGSA - DOP and active satellites
* GPGST - Pseudo Range Error Statistics
* GPGSV - Satellites in View
* GPRMC - Recommended Minimum specific GPS/Transit data
* GPVTG - Course over ground and Ground speed
* GPZDA - UTC Time and Date

The "GP" prefix usually indicates an original [GPS](https://en.wikipedia.org/wiki/Satellite_navigation#GPS) source.  NeoGPS parses *all* Talker IDs, including
  * "GL" ([GLONASS](https://en.wikipedia.org/wiki/Satellite_navigation#GLONASS)),
  * "BD" or "GB" ([BeiDou](https://en.wikipedia.org/wiki/Satellite_navigation#BeiDou)),
  * "GA" ([Galileo](https://en.wikipedia.org/wiki/Satellite_navigation#Galileo)), and
  * "GN" (mixed)

This means that GLRMC, GBRMC or BDRMC, GARMC and GNRMC will also be correctly parsed.  See discussion of Talker 
IDs in [Configurations](doc/Configurations.md#enabledisable-the-talker-id-and-manufacturer-id-processing).

Most applications can be fully implemented with the standard NMEA messages above.  They are supported by almost all GPS manufacturers.

However, to illustrate how unique capabilities of a particular device can be utilized, derived classes are also provided for ublox devices.

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
[Installing] (doc/Installing.md) | Copying files
[Data Model](doc/Data Model.md) | Aggregating pieces into a *fix*
[Configurations](doc/Configurations.md) | Tailoring NeoGPS to your needs
[Performance](doc/Performance.md) | 37% to 72% faster!  Really!
[RAM requirements](doc/RAM.md) | Doing it without buffers!
[Program Space requirements](doc/Program.md) | Making it fit
[Examples](doc/Examples.md) | Programming styles
[Troubleshooting](doc/Troubleshooting.md) | Troubleshooting
[Extending NeoGPS](doc/Extending.md) | Using specific devices
[ublox](doc/ublox.md) | ublox-specific code
[Tradeoffs](doc/Tradeoffs.md) | Comparing to other libraries
[Acknowledgements](doc/Acknowledgements.md) | Thanks!
