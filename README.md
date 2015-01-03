NeoGPS
======

This fully-configurable Arduino library uses _**minimal**_ RAM and CPU time, 
requiring as few as _**10 bytes of RAM**_.  It supports the following protocols and messages:

####NMEA 0183
* GPGGA - GPS system fix data
* GPGLL - Geographic Latitude and Longitude
* GPGSA - GPS/GNSS DOP and active satellites
* GPGST - GNSS Pseudo Range Error Statistics
* GPGSV - GPS/GNSS Satellites in View
* GPRMC - Recommended Minimum specific GPS/Transit data
* GPVTG - Course over ground and Ground speed
* GPZDA - UTC Time and Date

####u-blox NEO-6
* NAV_STATUS - Receiver Navigation Status
* NAV_TIMEGPS - GPS Time Solution
* NAV_TIMEUTC - UTC Time Solution
* NAV_POSLLH - Geodetic Position Solution
* NAV_VELNED - Velocity Solution in NED (North/East/Down)
* NAV_SVINFO - Space Vehicle Information

Goals
======
In an attempt to be reusable in a variety of different programming styles, this library supports:
* resource-constrained environments (e.g., ATTINY targets)
* sync or async operation (main `loop()` vs interrupt processing)
* event or polling (deferred handling vs. continuous fix() calls in `loop()`)
* fused or not fused (multiple reports into one fix)
* optional buffering of fixes
* optional floating point
* configurable message sets, including hooks for implementing proprietary NMEA messages
* configurable message fields
* multiple protocols from same device

(This is the plain Arduino version of the [CosaGPS](https://github.com/SlashDevin/CosaGPS) library for [Cosa](https://github.com/mikaelpatel/Cosa).)

Data Model
==========
Rather than holding onto individual fields, the concept of a **fix** is used to group data members of the GPS acquisition.
This also facilitates the merging of separately received packets into a coherent position.  The members of `gps_fix` include 
* fix status
* date
* time
* latitude and longitude
* altitude
* speed
* heading
* number of satellites
* horizontal, vertical and position dilutions of precision (HDOP, VDOP and PDOP)
* latitude, longitude and altitude error in centimeters

Constellation Information is also available in the GPS instance:
* satellite ID, azimuth, elevation and SNR

Except for `status`, each member is conditionally compiled; any, all, or *no* members can be selected for parsing, storing and fusing.  This allows configuring an application to use the minimum amount of RAM for the particular `fix` members of interest:

* Separate validity flag for each member.
* Integers are used for all members, retaining full precision of the original data.
* Optional floating-point accessors are provided.
* `operator |=` can be used to merge two fixes:
```
NMEAGPS gps_device;
gps_fix_t merged;
.
.
.
merged |= gps_device.fix();
```

RAM requirements
=======
As data is received from the device, various portions of a `fix` are modified.  In 
fact, _**no buffering RAM is required**_.  Each character affects the internal state machine and may 
also contribute to a data member (e.g., latitude).

The minimal `fix` configuration requires only 2 bytes, and the NMEA state machine requires 
7 bytes, for a total of **10 bytes** (structure alignment may add 1 byte).

A nominal `fix` configuration requires only 34 bytes, for a total of **41 bytes**.  For comparison, TinyGPS requires about 180 bytes (120 bytes for members plus about 60 bytes of string data), and TinyGPS++ requires about 240 bytes.

The complete `fix` configuration requires 44 bytes, and the full NMEA message set configuration adds 130 bytes of satellite data, for a total of **174 bytes**.  For comparison, satellite tracking in TinyGPS++ requires over 1100 bytes.

If your application only requires an accurate one pulse-per-second, you 
can configure it to parse *no* sentence types and retain *no* data members.  Although the 
`fix().status` can be checked, no valid flags are available.  Even 
though no sentences are parsed and no data members are stored, the application will 
still receive a `decoded` message type once per second:
```
while (uart1.available())
  if (gps.decode( uart1.getchar() )) {
    if (gps.nmeaMessage == NMEAGPS::NMEA_RMC)
      sentenceReceived();
  }
```

The `ubloxGPS` derived class adds 20 bytes to handle the more-complicated protocol, 
plus 5 static bytes for converting GPS time and Time Of Week to UTC.

Tradeoffs
=========

There's a price for everything, hehe...
* Parsing without buffers, or *in place*, means that you must be more careful about when you access data items.

In general, you should wait to access the fix until after the entire sentence has been parsed (See loop() in NMEA.ino). 
Member function `is_coherent()` can also be used to determine when it is safe.

If you need to access the fix at any time, you will have to double-buffer the fix.  (See NMEAGPS.h comments regarding a
`safe_fix`.)  Also, data errors can cause invalid field values to be set before the CRC is fully computed.  The CRC will
catch most of those, and the fix members will then be marked as invalid.

* Configurability means that the code is littered with #ifdef sections.

I've tried to increase white space and organization to make it more readable, but let's be honest... 
conditional compilation is ugly.

* Accumulating parts of a fix into group means knowing which parts are valid.

Before accessing a part, you must check its `valid` flag.  Fortunately, this adds only one bit per member.  See GPSfix.cpp for an example of accessing every data member.

* Correlating timestamps for coherency means extra date/time comparisons for each sentence before it is fused.

See NMEAfused.ino for code that determines when a new time interval has been entered.

* Full C++ OO implementation is more advanced than most Arduino libraries.

You've been warned!  ;)

* "fast, good, cheap... pick two."

Although most of the RAM reduction is due to eliminating buffers, some of it is from trading RAM
for additional code.  And, as I mentioned, the readabilty (i.e., goodness) suffers from its configurability.

Examples
======
Several programs are provided to demonstrate how to use the classes in these different styles:

* [NMEA](examples/NMEA.ino) - sync, polled, not fused, standard NMEA only
* [NMEAblink](examples/NMEAmin.ino) - sync, polled, not fused, standard NMEA only, minimal example, only blinks LED
* [NMEAfused](examplesNMEAfused.ino) - sync, polled, fused, standard NMEA only
* [PUBX](examples/PUBX.ino) - sync, polled, fused, standard NMEA + ublox proprietary NMEA
* [ublox](examples/ublox.ino) - sync, polled, fused, ublox protocol
* [NMEAtest.ino](examples/NMEAtest.ino) - sync, polled, not fused, standard NMEA only (This is a self-test program.  Various strings are passed to `decode` and the expected pass or fail results are displayed.  No GPS device is required.)

Preprocessor symbol `USE_FLOAT` can be used in [GPSfix.cpp](GPSfix.cpp) to select integer or floating-point output.

Acknowledgements
==========
Mikal Hart's [TinyGPS](https://github.com/mikalhart/TinyGPS) for the generic `decode` approach.

tht's [initial implementation](http://forum.arduino.cc/index.php?topic=150299.msg1863220#msg1863220) of a Cosa `IOStream::Device`.

Paul Stoffregen's excellent [Time](https://github.com/PaulStoffregen/Time) library.
