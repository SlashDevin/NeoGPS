Data Model
==========
Rather than holding onto individual fields, the concept of a **fix** is used to group data members of the GPS acquisition.
This also facilitates the merging of separately received packets into a fused or coherent position.  Satellite-specific information, the latest received message type, talker IDs and manufacturer IDs are not part of a `fix`; they are stored in the main GPS object.

The nested structures that your program can access are:
* the main `NMEAGPS gps` variable you declare in your sketch (see [Usage](Data%20Model.md#usage) below), which contains
    * a gps_fix member called `gps.fix()`, which contains
        * a status
        * a latitude and longitude
        * an altitude
        * a speed and heading
        * HDOP, VDOP and PDOP
        * latitude, longitude and altitude error in centimeters
        * a satellite count
        * a `NeoGPS::time_t` structure called `dateTime`, which contains
            *  year, month, day-of-month, hours, minutes, and seconds
        * centiseconds
        * a collection of `valid` flags for each of the above members
    * message type
    * talker ID
    * manufacturer ID
    * a satellite array, where each element contains
        * ID
        * azimuth
        * elevation
        * signal strength
        * tracking flag

Some examples of accessing these values:
```
fix_copy = gps.fix(); // copies all current fix data

int32_t lat_10e7 = gps.fix().lat; // scaled integer value of latitude
float lat = fix_copy.latitude(); // float value of latitude

if (gps.fix().dateTime.month == 4) // test for the cruelest month
  cry();

for (uint8_t i=0; i < gps.sat_count; i++) {
  if (gps.satellites[i].tracked) {
    if (gps.satellites[i] . id <= 32)
      GPS_satellites++;
    if (gps.satellites[i] . id <= 64)
      SBAS_satellites++;
    if (gps.satellites[i] . id <= 96)
      GLONASS_satellites++;
  }
}
```

And some examples of accessing valid flags in a `fix` structure:
```
if (gps.fix().valid.location)
  // we have a lat/long!
if (fix_copy.valid.time)
  // the copy has hours, minutes and seconds
```

##Options
Except for `status`, each of these members is conditionally compiled; any, all, or *no* members can be selected for parsing, storing and fusing.  This allows configuring an application to use the minimum amount of RAM for the particular `fix` members of interest.  See [Configurations](Configurations.md) for how to edit [GPSfix_cfg.h](/GPSfix_cfg.h) and [NMEAGPS_cfg.h](/NMEAGPS_cfg.h#L67), respectively.

##Precision
Integers are used for all members, retaining full precision of the original data.
```
    if (gps.fix().valid.location) {
      // 32-bit ints have 10 significant digits, so you can detect very
      // small changes in position:
      d_lat = gps.fix().lat - last_lat;
    }
```

Optional floating-point accessors are provided.
```
    if (gps.fix().valid.location) {
      float lat = gps.fix().latitude();

      // floats only have about 6 significant digits, so this
      // computation is useless for detecting small movements:
      foobar = (lat - target_lat);
```

##Usage
First, declare an instance of `NMEAGPS`:

```
NMEAGPS gps;
```
Next, pass all the received bytes to the `gps` instance:
```
void loop()
{
  while (serial.available()) {
    char c = serial.read();
    gps.decode( c );
  }
  ...
```
As `gps` decodes those bytes, it will gradually fill out the pieces of its own `fix` data, `gps.fix()` (members described above).  When you want to use some of the fix data, you can access it like this:
```
  Serial.print( gps.fix().latitude() );
  Serial.print( ',' );
  Serial.println( gps.fix().longitude() );
```
However, there are two things to know *before* accessing the fix data:

(1) You must wait for the sentence to be completely decoded.  As bytes are received, they will gradually fill out a `gps.fix()` member.  For example, `gps.fix().speed` may be half-formed.  You can either do all your accessing after `gps.decode()` returns `DECODE_COMPLETED`:
```
void loop()
{
  // At this point of the code, speed could be half-decoded.
  if (gps.fix().speed <= 5)  // NOT A GOOD IDEA!
    Serial.println( F("Too slow!") );

  while (serial.available()) {
    char c = serial.read();
    if (gps.decode( serial.read() ) == NMEAGPS::DECODE_COMPLETED) {
    
      // Access any piece of gps.fix() in here...
    
      if (gps.fix().speed <= 5)  // OK!
        Serial.println( F("Too slow!") );
    
      if (gps.fix().lat ...
    }
  }
  
```
Or, you can copy `gps.fix()` into your own variable when `gps.decode()` returns `DECODE_COMPLETED`:
```
gps_fix my_fix;

void loop()
{
  while (serial.available()) {
    char c = serial.read();
    if (gps.decode( serial.read() ) == NMEAGPS::DECODE_COMPLETED) {
      my_fix = gps.fix(); // save for later...
    }
  }
  
  if (my_fix.speed <= 5)  // OK
    DigitalWrite( UNDERSPEED_INDICATOR, HIGH );
```
This technique is used in the example program, [NMEA.ino](/examples/NMEA/NMEA.ino#L96).

(2) You must check that the `fix` piece is valid.  Remember that the GPS device may *not* have a fix: it may not know the lat/long yet.  To check whether the a piece of fix data has been received, test the corresponding `valid` flag.  For example, to see if lat/long data has been received yet:
```
  if (my_fix.valid.location) {
    Serial.print( my_fix.latitude() );
    Serial.print( ',' );
    Serial.println( my_fix.longitude() );
  }
```
In fact, each `fix` member has its own validity flag (except lat/long, which share the `valid.location` flag as above).  See the example printing utility file, [Streamers.cpp](/Streamers.cpp#L100) for accessing all `fix` members.

This means that even though you have enabled a particular member (see [GPSfix_cfg.h](/GPSfix_cfg.h)), it will not have a value until the related NMEA sentence sets it.  Here's an example for accessing the altitude
```
    if (gps.fix().valid.altitude) {
      z2 = gps.fix().altitude_cm();
      vz = (z2 - z1) / dt;
      z1 = z2;

      // Note: if you only care about meters, you could also do this:
      //    z = gps.fix().alt.whole;
    }
```
You can also check a collection of flags before performing a calculation involving 
multiple members:
```
    if (gps.fix().valid.altitude && gps.fix().valid.date && gps.fix().valid.time) {
      dt = (clock_t) gps.fix().dateTime - (clock_t) gps.fix().dateTime;
      dz = gps.fix().alt.whole - last_alt;  // meters
      vz = dz / dt;                         // meters per second vertical velocity
    }
```
Bonus: The compiler will optimize this into a single bit mask operation.

##Merging
Because different NMEA sentences contain different pieces of a fix, they have to be "merged" to determine a complete picture.  Some sentences contain only date and time.  Others contain location and altitude, but not speed and heading.

There are several ways to use the GPS fix data: without merging, implicit merging, and explicit merging.  NeoGPS allows you to choose how you want multiple sentences to be merged:

###1. On a per-sentence basis (no merging)
If you are interested in a few pieces of information, and these pieces can be obtained from one or two sentences, you can wait for that specific sentence to arrive, and then use one or more members of the `gps.fix()` at that time:
```
void loop()
{
  while (serial.available()) {
    char c = serial.read();
    if (gps.decode( serial.read() ) == NMEAGPS::DECODE_COMPLETED) {

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC) {
        // All GPS-related work is performed inside this if statement

        if (gps.fix().valid.speed && (gps.fix().speed <= 5))
          Serial.println( F("Too slow!") );
          
        if (gps.fix().valid.location &&  ... or any gps.fix() member
      }
    }
  }
  
  // Can't access gps.fix() out here...
```

###2. On a free-running basis (implicit merging)
If you are interested in more pieces of information, perhaps requiring more kinds of sentences to be decoded, but don't really care about what time the pieces were received, you could enable `NMEAGPS_ACCUMULATE_FIX` (see [Configurations](Configurations.md#nmeagps) and 
[NMEAGPS_cfg.h](/NMEAGPS_cfg.h#L112)).  The `gps.fix()` data can be accessed only after DECODE_COMPLETED, or when it `is_safe()`.  This data is not necessarily coherent.  

By "coherent", I mean that the `gps.fix()` lat/long may have been set by the newest sentence, but the altitude may be from the previous time interval.  Most applications do not care that the `gps.fix()` members are not coherent.  However, if you are controlling a drone or other autonomous vehicle, you may need coherency.

It is possible to achieve coherency if you detect the "quiet" time between batches of sentences.  When new data starts coming in, simply call `gps.fix.init()`; all new sentences will set the fix members.  Note that if the GPS device loses its fix on the satellites, you can be left without _any_ valid data.  If this is not acceptable, you will have to use explicit merging (see next section).

[NMEA.ino](/examples/NMEA/NMEA.ino) can be used with implicit merging.  However, [NMEAfused.ino](/examples/NMEAfused/NMEAfused.ino) and [NMEAcoherent.ino](/examples/NMEAcoherent/NMEAcoherent.ino) should not be used with implicit merging, because they perform their own explicit merging.

###3. In selective batches (explicit merging)
If you are interested in pieces of information that are grouped by some criteria (e.g., coherency or satellites), you must perform explicit merging.  This also "buffers" the main loop from the constantly-changing `gps.fix()`, as described above.  The `merged' copy is safe to access at any time:

```
    gps_fix_t merged;

    void loop()
    {
      while (Serial1.available())
        if (gps.decode( Serial1.read() ) == NMEAGPS::DECODE_COMPLETED)
          // ...and other criteria
          merged |= gps.fix();

      check_position( merged );
```
Note how the `|=` operator is used to perform the explice merge.  See [NMEAfused.ino](/examples/NMEAfused/NMEAfused.ino) and [GPSfix.h](/GPSfix.h#L220).

For example, you may use the Talker ID to separate the fix data into independent groups.  Obviously, explicit merging requires one or more extra `gps_fix` copies.  

Explicit merging is also required to implement coherency.  Because a sentence has to be parsed to know its timestamp, invalidating old data (i.e., data from a previous update period) must be performed _before_ the sentence parsing begins.  That can only be accomplished with a second 'safe' copy of the fix data and explicit merging.    With implicit merging, new data has already been mixed with old data by the time DECODE_COMPLETED occurs and timestamps can be checked.

To implement coherency, you should clear out the merged fix when a new time 
interval begins:
```
    if (new_interval)
      merged  = gps.fix(); // replace
    else
      merged |= gps.fix(); // merge
```
See [NMEAcoherent.ino](/examples/NMEAcoherent/NMEAcoherent.ino#L67)
