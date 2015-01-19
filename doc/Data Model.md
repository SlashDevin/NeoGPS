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

####Merge two fixes with `operator |=`
As sentences are received, you can merge the information from multiple sentences into 
one fix object:
```
    gps_fix_t merged;

    void loop()
    {
      while (Serial1.available())
        if (gps.decode( Serial1.read() ) == NMEAGPS::DECODE_COMPLETED)
          merged |= gps.fix();

      check_position( merged );
```
This is an explicit merging of fix information into a second copy.  The `merged` 
object "buffers" the main loop from the constantly-changing `gps.fix()`, and it is 
safe to access at any time.  See [NMEAfused.ino](/examples/NMEAfused/NMEAfused.ino).

Implicit merging into `gps.fix()` can be enabled, eliminating the need for the 
`merged` copy.  See [Configurations](Configurations.md#nmeagps) and 
[NMEAGPS.h](/NMEAGPS.h#L66).

If you need coherency, you should clear out the merged fix when a new time 
interval begins:
```
    if (new_interval)
      merged  = gps.fix(); // replace
    else
      merged |= gps.fix(); // merge
```
See [NMEAcoherent.ino](/examples/NMEAcoherent/NMEAcoherent.ino#L67)
