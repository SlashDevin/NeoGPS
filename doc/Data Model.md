Data Model
==========
Rather than holding onto individual fields, the concept of a **fix** is used to group data members of the GPS acquisition.
This also facilitates the merging of separately received packets into a fused or coherent position.

##Members
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

The members of `NMEAGPS` include
* Talker ID (usually GP)
* Manufacturer ID (from proprietary sentences)
* Satellite Constellation (ID, azimuth, elevation, SNR and tracking)

You should declare an instance of `NMEAGPS`, which contains an instance of `gps_fix`, called `fix()`:

```
NMEAGPS gps;

void loop()
{
  ...
  if (gps.fix().valid.status && gps.fix().status != gps_fix::STATUS_NONE)
    // We can hear satellites!
```

Except for `status`, each member is conditionally compiled; any, all, or *no* members can be selected for parsing, storing and fusing.  This allows configuring an application to use the minimum amount of RAM for the particular `fix` members of interest.

To access the members of the current `fix`, please note the following:

####Each member has its own validity flag.
Even though you have enabled a particular member, it will not have a value until the related NMEA sentence sets it.  Be sure to check the validity flag before attempting to use its value:
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
    if (gps.fix().valid.altitude && gps.fix().valid.dateTime) {
      dt = (clock_t) gps.fix().dateTime - (clock_t) gps.fix().dateTime;
      dz = gps.fix().alt.whole - last_alt;  // meters
      vz = dz / dt;                         // meters per second vertical velocity
    }
```
Bonus: The compiler will optimize this into a single bit mask operation.

####Integers are used for all members, retaining full precision of the original data.
```
    if (gps.fix().valid.location) {
      // 32-bit ints have 10 significant digits, so you can detect very
      // small changes in position:
      d_lat = gps.fix().lat - last_lat;
    }
```

####Optional floating-point accessors are provided.
```
    float lat;
    if (gps.fix().valid.location)
      lat = gps.fix().latitude();

    // floats only have about 6 significant digits, so this
    // computation is useless for detecting small movements:
    foobar = (lat - last_lat);
```

##Merging
There are several ways to use the GPS fix data: without merging, implicit merging, and explicit merging.

###1. On a per-sentence basis (no merging)
If you are interested in a few pieces of information, and these pieces can be obtained from one or two sentences, you can wait for that specific sentence to arrive, and then use one or more members of the `fix()` at that time.  See NMEA.ino.

###2. On a free-running basis (implicit merging)
If you are interested in more pieces of information, perhaps requiring more kinds of sentences to be decoded, but don't really care about what time the pieces were received, you could enable `NMEAGPS_ACCUMULATE_FIX` (see [Configurations](Configurations.md#nmeagps) and 
[NMEAGPS.h](/NMEAGPS.h#L66)).  The `fix()` data can be accessed only after DECODE_COMPLETED, or when it `is_safe()`.This data is not necessarily coherent.  

It is possible to achieve coherency if you detect the "quiet" time between batches of sentences.  When new data starts coming in, simply call `gps.fix.init()`; all new sentences will set the fix members.  Note that if the GPS device loses its fix on the satellites, you can be left without _any_ valid data.

example/NMEA.ino can be used with implicit merging.  However, NMEAfused.ino and NMEAcoherent.ino should not, because they perform their own explicit merging.

###3. In selective batches (explicit merging)
If you are interested in pieces of information that are grouped by some criteria, you must perform explicit merging.  Additionally, the `merged` object "buffers" the main loop from the constantly-changing `gps.fix()`.  The `merged' copy is safe to access at any time:

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
See [NMEAfused.ino](/examples/NMEAfused/NMEAfused.ino).

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
