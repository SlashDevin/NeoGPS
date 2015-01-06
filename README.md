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

Data Model
==========
Rather than holding onto individual fields, the concept of a **fix** is used to group data members of the GPS acquisition.
This also facilitates the merging of separately received packets into a fused or coherent position.

The members of `gps_fix` include 

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

The compiler **cannot** catch message set dependencies: the `enum 
nmea_msg_t` is always available.  So even though a `fix` member is enabled, 
you may have disabled all messages that would have set its value.  
NMEAtest.ino can be used to check some configurations.

####Typical configurations
A few common configurations are defined as follows

**Minimal**: no fix members, no messages (pulse-per-second only)

**DTL**: date, time, lat/lon, GPRMC messsage only.

**Nominal**: date, time, lat/lon, altitude, speed, heading, number of 
satellites, HDOP, GPRMC and GPGGA messages.

**Full**: Nominal plus VDOP, PDOP, lat/lon/alt errors, satellite array with satellite info, and all messages.

(**TinyGPS** uses the **Nominal** configuration.)

RAM requirements
=======
As data is received from the device, various portions of a `fix` are 
modified.  In fact, _**no buffering RAM is required**_.  Each character 
affects the internal state machine and may also contribute to a data member 
(e.g., latitude). 

The **Minimal** configuration requires only 2 bytes, and the NMEA state 
machine requires 7 bytes, for a total of **10 bytes** (structure alignment 
may add 1 byte). 

The **DTL** configuration requires 18 bytes, for a total of **25 bytes**.

The **Nominal** configuration requires only 34 bytes, for a total of **41 
bytes**.  For comparison, TinyGPS requires about 180 bytes (120 bytes for 
members plus about 60 bytes of string data), and TinyGPS++ requires about 
240 bytes.

The **Full** configuration requires 44 bytes, and the full NMEA message set 
configuration adds 130 bytes of satellite data, for a total of **174 
bytes**.  For comparison, satellite tracking in TinyGPS++ requires over 1100 
bytes.

If your application only requires an accurate one pulse-per-second, you 
can configure it to parse *no* sentence types and retain *no* data members. 
This is the **Minimal** configuration.  Although the 
`fix().status` can be checked, no valid flags are available.  Even 
though no sentences are parsed and no data members are stored, the 
application will  still receive a `decoded` message type once per second:

```
while (uart1.available())
  if (gps.decode( uart1.getchar() )) {
    if (gps.nmeaMessage == NMEAGPS::NMEA_RMC)
      sentenceReceived();
  }
```

The `ubloxNMEA` derived class doesn't use any extra bytes of RAM.

The `ubloxGPS` derived class adds 20 bytes to handle the more-complicated protocol, 
plus 5 static bytes for converting GPS time and Time Of Week to UTC.

Program Space requirements
=======

The **Minimal** configuration requires 866 bytes.

The **DTL** configuration requires 2072 bytes.

The **Nominal** configuration requires 2800 bytes.  TinyGPS uses about 2400 bytes.

The **Full** configuration requires 3462 bytes.

(All configuration numbers include 166 bytes PROGMEM.)

Performance
===========

####**NeoGPS** is **33% faster _or more_**.

Most libraries use extra buffers to accumulate parts of the sentence so they 
can be parsed all at once.  For example, an extra field buffer may hold on 
to all the characters between commas.  That buffer is then parsed into a 
single data item, like `heading`.  Some libraries even hold on to the 
*entire* sentence before attempting to parse it.  In addition to increasing 
the RAM requirements, this requires **extra CPU time** to copy the bytes and 
index through them... again.
 
**NeoGPS** parses each character immediately into the data item.  When the 
delimiting comma is received, the data item has been fully computed *in 
place* and is marked as valid.

Most libraries parse all fields of their selected sentences.  Although most 
people use GPS for obtaining lat/long, some need only time, or even just one 
pulse-per-second.

**NeoGPS** configures each item separately.  Disabled items are 
conditionally compiled, which means they will not use any RAM, program space 
or CPU time.  The characters from those fields are simply skipped; they are 
never copied into a buffer or processed.

For comparison, the following sentences were parsed by Nominal configurations of **NeoGPS** and **TinyGPS** on a 16MHz Arduino Mega2560.

```
$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,8,1.01,499.6,M,48.0,M,,0*5B
$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*57
```

**NeoGPS** takes 1074uS to completely parse a GPGGA sentence, and **TinyGPS** takes 1448uS.

**NeoGPS** takes 1106uS to completely parse a GPRMC sentence, and **TinyGPS** takes 1435uS.
 
The **DTL** configuration of **NeoGPS** takes 912uS to 
completely parse a GPGGA sentence, and 971uS to completely parse a GPRMC sentence.

Extending NeoGPS
=========
Using features that are unique to your device fall into three categories:
####1. Configuring the device with special commands
Many devices allow you to configure which standard messages are emitted, or the rate at which they are emitted.  It may be as simple as sending a proprietary command to the device.  Simply use the NMEAGPS `send` or `send_P` method.

For example, to set the baudrate of the ublox NEO-6M gps device, send it a 
`UBX,41` message:
```
  gps.send_P( F("PUBX,41,1,0007,0003,19200,0") );
```
####2. Parsing additional message types
Some devices provide additional messages with extra information, or more efficient groupings.  This will require deriving a class from `NMEAGPS`.  The derived class needs to
* declare a PROGMEM table of the new message types,
* point that table back to the NMEAGPS table
* override the `parseField` method to extract information from each new message type

Please see ubxNMEA.h and .cpp for an example of adding two ublox-proprietary messages.

####3. Handling new protocols
Some devices provide additional protocols.  They are frequently binary, which requires 
fewer bytes than NMEA 0183.  Because they can both be transmitted on the same port, it is 
very likely that they can be distinguished at the message framing level.

For example, NMEA messages always start with a '$' and end with CR/LF.  ublox messages start 
with 0xB5 and 0x62 bytes, a message class and id, and a 2-byte message length.  There is no 
terminating character; the message completed when `length` bytes have been received.

This will require deriving a class from `NMEAGPS`.  The derived class needs 
to 
* define new `rxState` values for the protocol state machine.  These should 
be unique from the NMEA state values, but they should share the IDLE state 
value.
* override the `decode` method to watch for its messages.  As bytes are 
received, it may transition out of the IDLE state and into its own set of 
state values.  If the character is not valid for this protocol, it should 
delegate it to the NMEAGPS base clase, which may begin processing an NMEAGPS 
message.  If the rxState is not one of the derived states (i.e., it is in 
one of the NMEAGPS states), the character should be delegated to 
NMEAGPS::decode.
* implement something like the `parseField` method if parse-in-place 
behavior is desirable.  This is not necessarily `virtual`, as it will only 
be called from the derived `decode`.
* You are free to add methods and data members as required for handling the 
protocol.  Only `decode` must be overridden.

Please see ubxGPS.h and .cpp for an example of implementing the 
ublox-proprietary protocol, UBX.  The derived `ubloxGPS` class provides both 
parse-in-place *and* buffered messages.  See the `send` and `poll` methods.

Tradeoffs
=========

There's a price for everything, hehe...

####Parsing without buffers, or *in place*, means that you must be more careful about when you access data items.

In general, you should wait to access the fix until after the entire sentence has been parsed.  Most of the examples simply `decode` until a sentence is COMPLETED, then do all their work with `fix`.  See `loop()` in [NMEA.ino](examples/NMEA.ino). 
Member function `is_safe()` can also be used to determine when it is safe.

If you need to access the fix at any time, you will have to double-buffer the fix: simply copy the `fix` when it is safe to do so.  (See NMEAGPS.h comments regarding a
`safe_fix`.)  Also, received data errors can cause invalid field values to be set *before* the CRC is fully computed.  The CRC will
catch most of those, and the fix members will then be marked as invalid.

####Configurability means that the code is littered with #ifdef sections.

I've tried to increase white space and organization to make it more readable, but let's be honest... 
conditional compilation is ugly.

####Accumulating parts of a fix into group means knowing which parts are valid.

Before accessing a part, you must check its `valid` flag.  Fortunately, this adds only one bit per member.  See GPSfix.cpp for an example of accessing every data member.

####Correlating timestamps for coherency means extra date/time comparisons for each sentence before it is fused.

This is optional: compare NMEAcoherent.ino and NMEAfused.ino to see code that determines when a new time interval has been entered.

####Full C++ OO implementation is more advanced than most Arduino libraries.

You've been warned!  ;)

####"fast, good, cheap... pick two."

Although most of the RAM reduction is due to eliminating buffers, some of it is from trading RAM
for additional code (see **Nominal** Program Space above).  And, as I mentioned, the readabilty (i.e., goodness) suffers from its configurability.

Examples
======
Several programs are provided to demonstrate how to use the classes in these different styles:

* [NMEA](examples/NMEA/NMEA.ino) - sync, polled, single, standard NMEA only
* [NMEAblink](examples/NMEAblink/NMEAblink.ino) - sync, polled, single, standard NMEA only, minimal example, only blinks LED
* [NMEAfused](examples/NMEAfused/NMEAfused.ino) - sync, polled, fused, standard NMEA only
* [NMEAcoherent](examples/NMEAcoherent/NMEAcoherent.ino) - sync, polled, coherent, standard NMEA only
* [PUBX](examples/PUBX/PUBX.ino) - sync, polled, coherent, standard NMEA + ublox proprietary NMEA
* [ublox](examples/ublox/ublox.ino) - sync, polled, coherent, ublox protocol

Preprocessor symbol `USE_FLOAT` can be used in [GPSfix.cpp](GPSfix.cpp) to select integer or floating-point output.

A self-test test program is also provided:

* [NMEAtest.ino](examples/NMEAtest/NMEAtest.ino) - sync, polled, not fused, standard NMEA only

No GPS device is required; the bytes are streamed from PROGMEM character arrays.  Various strings are passed to `decode` and the expected pass or fail results are displayed.  If **NeoGPS** is correctly configured, you should see this on your SerialMonitor:

```
NMEA test: started
fix object size = 34
NMEAGPS object size = 64
Test string length = 75
PASSED 6 tests.
------ Samples ------
Input:
  $GPGGA,092725.00,4717.11399,N,00833.91590,E,1,8,1.01,499.6,M,48.0,M,,0*5B
Results:  3,1970-00-00 09:27:25.00,472852332,85652650,,,49960,8,1010,
Input:
  $GPRMC,092725.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*5E
Results:  3,2002-12-09 09:27:25.00,472852395,85652537,7752,4,,,,
Input:
  $GPGGA,064951.000,2307.1256,N,12016.4438,E,1,8,0.95,39.9,M,17.8,M,,*63
Results:  3,2002-12-09 06:49:51.00,231187600,1202740633,,,3990,8,950,
Input:
  $GPRMC,064951.000,A,2307.1256,N,12016.4438,E,0.03,165.48,260406,3.05,W,A*2C
Results:  3,2006-04-26 06:49:51.00,231187600,1202740633,16548,30,,,,
Input:
  $GPVTG,165.48,T,,M,0.03,N,0.06,K,A*36
Results:  3,,,,16548,30,,,,
Input:
  $GPGSA,A,3,29,21,26,15,18,09,06,10,,,,,2.32,0.95,2.11*00
Results:  3,,,,,,,,950,
Input:
  $GPGSV,3,1,09,29,36,029,42,21,46,314,43,26,44,020,43,15,21,321,39*7D
Results:  ,,,,,,,9,,
Input:
  $GPGSV,3,2,09,18,26,314,40,09,57,170,44,06,20,229,37,10,26,084,37*77
Results:  ,,,,,,,9,,
Input:
  $GPGSV,3,3,09,07,,,26*73
Results:  ,,,,,,,9,,,[29,21,26,15,18,9,6,10,7,]
```

Acknowledgements
==========
Mikal Hart's [TinyGPS](https://github.com/mikalhart/TinyGPS) for the generic `decode` approach.

tht's [initial implementation](http://forum.arduino.cc/index.php?topic=150299.msg1863220#msg1863220) of a Cosa `IOStream::Device`.

Paul Stoffregen's excellent [Time](https://github.com/PaulStoffregen/Time) library.
