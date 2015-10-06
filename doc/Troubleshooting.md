Troubleshooting
===============
Most problems are caused by these kinds of errors:
   1. [Device connection](#gps-device-connection-problems) (use `NMEAdiagnose.ino`)
   2. [Poor reception](#poor-reception) (use `NMEA.ino`)
   2. [Configuration](#configuration-errors) (use `NMEAtest.ino`)
   3. [Quiet Time Interval](#quiet-time-interval) (use `NMEAorder.ino`)
   4. [Trying to do too many things](#trying-to-do-too-many-things) at the same time

##GPS device connection problems
The example program `NMEAdiagnose.ino` can detect two kinds of problems: incorrect wiring and incorrect baud rate.

####Not correctly wired
If the GPS device is not correctly connected, the example program `NMEA.ino` will print lines of empty data:
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,Rx ok,Rx err,Rx chars,
,,,,,,,,0,0,0,
,,,,,,,,0,0,0,
```
This shows that **no** data is being received: no sentences ok and no characters.  You may have the GPS device connected to the wrong pins (GPS RX should be connected to Arduino TX, and GPS TX should be connected to Arduino RX), or the .INO may be using the wrong serial object: review `GPSport.h` for the expected connections.

####Wrong baud rate
If the example program `NMEA.ino` is using the wrong baud rate, it will print lines of empty data:
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,Rx ok,Rx err,Rx chars,
,,,,,,,,0,3,181,
,,,,,,,,0,1,422,
```
You may be using the wrong baud rate.  This says that characters are being received (Rx chars), but none of them were decoded successfully (Rx ok), and a few sentences had errors.  You can:
   1. Review your GPS device's documentation to find the correct baud rate.
   2. Try different values in `gps_port.begin`.  2400, 4800, 19200, 38400, and 115200 are also possible.
   3. Use `NMEAdiagnose.INO` auto-detect the correct baud rate.

##Poor reception
If `NMEA.ino` displays "mostly" empty fields:
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
If it continues to report only date and time, you do not have an unobstructed view of the sky: only one satellite signal is being received.

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
#define NMEAGPS_MAX_SATELLITES (20)

#define NMEAGPS_ACCUMULATE_FIX
```
This will display additional fields for how many satellites are in view, whether they are being "tracked", and their individual signal strengths.
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,VDOP,PDOP,Lat err,Lon err,Alt err,Sats,[sat elev/az @ SNR],Rx ok,Rx err,Rx chars,
2015-09-14 16:03:07,3,2015-09-14 20:03:07.00,,,,,,,,,,,,2,[2 71/27@14,5 65/197@33,],8,0,476,
2015-09-14 16:03:08,3,2015-09-14 20:03:08.00,,,,,,,,,,,,2,[2 71/27@14,5 65/197@33,],16,0,952,
```
This shows that only two satellites are being tracked.  You must move to a position with a better view of the sky.

##Quiet Time Interval
Because GPS devices send lots of data, it is possible for the Arduino input buffer to overflow.  Many other libraries' examples will fail when modified to print too much information, or write to an SD card (see also [next section](#trying-to-do-too-many-things)).

NeoGPS examples are structured in a way that takes advantage of a "quiet time" in the data stream.  GPS devices send a batch of sentences, once per second.  After the last sentence in the batch has been sent, the GPS device will not send any more data until the next interval.  The example programs watch for that last sentence.  When it is received, it is safe to perform time-consuming operations.

It is **critical** to know the order of the sentences sent by your specific device.  If you do not know what sentence is last in a batch, use the example program `NMEAorder.ino` to list the sentence order.  When you know the last sentence, review `NMEA.ino` to confirm the correct value on this line:

```
static const NMEAGPS::nmea_msg_t LAST_SENTENCE_IN_INTERVAL = NMEAGPS::NMEA_GLL;
```

Most example programs depend on this statement.  If `LAST_SENTENCE_IN_INTERVAL` is not correct for your device, the example programs will probably lose GPS data, especially on `SoftwareSerial` data ports.  The example programs may behave like they are using the wrong baud rate: empty fields, increasing Rx Errors, and increasing Rx Chars.  Basically, the example programs are [Trying To Do Too Much](#trying-to-do-too-many-things) at the wrong time.  With the correct `LAST_SENTENCE_IN_INTERVAL`, the example programs will not lose GPS data.

##Configuration errors
Because there are so many configurable items, it is possible that your configuration prevents acquiring the desired GPS information.

The compiler **cannot** catch message set dependencies: the enum 
`nmea_msg_t` is always available.  So even though a `fix` member is enabled, 
you may have disabled all messages that would have set its value.  
`NMEAtest.ino` can be used to check some configurations.

For example, if your application needs altitude, you **must** enable the GGA sentence.  No other sentence provides the altitude member (see [this table](Choosing.md)).  If `NMEA_PARSE_GGA` is not defined,  `gps.decode()` will return COMPLETED after a GGA is received, but no parts of the GGA sentence will have been parsed, and altitude will never be valid.  NeoGPS will _recognize_ the GGA sentence, but it will not be parsed.

The compiler will catch any attempt to use parts of a `fix` that have been 
configured out: you will see something like `gps_fix does not have member 
xxx`.

There are also compile-time checks to make sure the configuration is valid.  For example, if you enable `NMEAGPS_PARSE_TALKER_ID` so that you can handle GLONASS messages differently than GPS messages, you *must* enable `NMEAGPS_DERIVED_TYPES`.  An error message will tell you to do that.  Until you disable `NMEAGPS_PARSE_TALKER_ID` **or** enable `NMEAGPS_DERIVED_TYPES`, it will not compile.

##Trying to do too many things
Many libraries and their examples, and I mean almost all of 'em, are not structured in a way that lets you do more than one thing in a sketch.   The result: the example program works great, but adding anything to it breaks it.

####Printing too much
Many programmers run into trouble because they try to print too much debug info.  The Arduino `Serial.print` function will "block" until those output characters can be stored in a buffer.  While the sketch is blocked at `Serial.print`, the GPS device is probably still sending data.  The _input_ buffer on an Arduino is only 64 bytes, about the size of one NMEA sentence.  After 64 bytes have been received stored, all other data is dropped!  Depending on the GPS baud rate and the Serial Monitor baud rate, it may be very easy to lose GPS characters.

It is crucial to call `serial.read` frequently, and to _never_ call a blocking function that takes more than (64*11/baud) seconds.  If the GPS is running at 9600, you cannot block for more than 70ms.  If your debug `Serial` is also running at 9600, you cannot write more than 64 bytes in a row!

####Blocking operations
Most Arduino libraries are written in a blocking fashion.  That is, if you call a library's function, it will not return from that function until the operation has been completed.  If that operation takes a long time, GPS characters will be dropped.

Many programmers want to write GPS data to an SD card.  This is completely reasonable to do, but an `SD.write` can block long enough to cause the input buffer to overflow.  SD libraries have their own buffers, and when they are filled, the library performs SPI operations to "flush" the buffer to the SD card.  While that is happening, the GPS device is _still_ sending data, and it will eventually overflow the serial input buffer.

This is a very common problem!  Here's some diagrams to help explain the timing for the Adafruit_GPS library.  First, lets look at how the incoming GPS data relates to reading and parsing it:

<img src="images/GPS%20Timing%200.jpg"/>

Note how loop calls GPS.read, and when it has read all the chars that have been received up to that point, it returns.  loop may get called lots of times while it's waiting for the chars to come in.  Eventually, the whole sentence is received, newNMEAreceived returns true, and your sketch can `parse` the new data (not needed for **NeoGPS**).

The problem is that if you try to do anything that takes "too long", `GPS.read` won't get called.  The incoming chars stack up in the input buffer until it's full.  After that, the chars will be dropped:

<img src="images/GPS%20Timing%201.jpg"/>

The next sentence, a GPRMC, continues to come in while `Serial.print` and `SD.write` are doing their thing... and data gets lost.

Fortunately, there is a way to work around this.  It turns out that the GPS device is sending a batch of sentences once every second, maybe 5 at a time.  Most of that one-second interval is actually is a "quiet time" that is perfect for doing other things:

<img src="images/GPS%20Timing%202.jpg"/>

All you need to do is hold on to the GPS information (date, time, location, etc.) until the quiet time comes around.  You'll need to take the same approach for each additional task.  For additional sensors, hold on to the temperature, acceleration, whatever, until the quiet time comes around.  *Then* perform the blocking operation, like `SD.write`, and no GPS data will be lost.

This is why NeoGPS uses a `fix` structure: it can be
   * _populated_ as the characters are received,
   * _copied/merged_ when a sentence is complete, and then
   * _used_ anytime (for fast operations) or during the quiet time (for slow operations).

You do not have to call a "parse" function after a complete sentence has been received -- the data was parsed as it was received.  Essentially, the processing time for parsing is spread out across the receipt of all characters.  When the last character of the sentence is received (i.e. `gps.decode(c) == DECODE_COMPLETED`), the relevant members of `gps.fix()` have already been populated.

All the example programs are structured so that the (relatively) slow printing operations are performed during the GPS quiet time.  Simply replace those trace/print statements with your specific code.

If you still do not have enough time to complete your tasks during the GPS quiet time, you can
   * Increase the baud rate on the debug port (takes less time to print)
   * Increase the baud rate on the GPS port (increases quiet time)
   * Configure the GPS device to send fewer sentences (decreases parsing time, increases quiet time)
   * Use a binary protocol for your specific device (decreases parsing time, increases quiet time)
   * Watch for a specific message to be COMPLETED, then begin your specific processing.  This may cause some sentences to lose characters, but they may not be necessary.  See comments regarding `LAST_SENTENCE_IN_INTERVAL` above.
