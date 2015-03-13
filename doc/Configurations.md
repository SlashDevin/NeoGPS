Configuration
=============
All configuration items are conditional compilations: a `#define` controls an `#if`/`#endif` section.
Delete or comment out any items to be excluded from your build.  Where 
possible, checks are performed to verify that you have chosen a "valid" 
configuration: you may see `#error` messages in the build log.

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
The following configuration items are near the top of NMEAGPS.h.
####Enable/Disable parsing the fields of a $GPxxx sentence
```
#define NMEAGPS_PARSE_GGA
#define NMEAGPS_PARSE_GLL
#define NMEAGPS_PARSE_GSA
#define NMEAGPS_PARSE_GSV
#define NMEAGPS_PARSE_GST
#define NMEAGPS_PARSE_RMC
#define NMEAGPS_PARSE_VTG
#define NMEAGPS_PARSE_ZDA
```
### Enable/disable the talker ID and manufacturer ID processing.
There are two kinds of NMEA sentences:

1. Standard NMEA sentences begin with "$ttccc", where
      "tt" is the talker ID (e.g., GP), and
      "ccc" is the variable-length sentence type (e.g., RMC).

    For example, "$GPGLL,..." is a GLL sentence (Geographic Lat/Long) 
    transmitted by talker "GP".  This is the most common talker ID.  Some devices
    may report "$GNGLL,..." when a mix of GPS and non-GPS satellites have been
    used to determine the GLL data (e.g., GLONASS+GPS).

2. Proprietary NMEA sentences (i.e., those unique to a particular manufacturer) 
    begin with "$Pmmmccc", where
      "P" is the NMEA-defined prefix indicator for proprietary messages,
      "mmm" is the 3-character manufacturer ID, and
      "ccc" is the variable-length sentence type (it can be empty).

No validation of manufacturer ID and talker ID is performed in this
base class.  For example, although "GP" is a common talker ID, it is not
guaranteed to be transmitted by your particular device, and it IS NOT REQUIRED. 
If you need validation of these IDs, or you need to use the extra information
provided by some devices, you have two independent options:

1. Enable SAVING the ID: When `decode` returns DECODE_COMPLETED, the `talker_id`
and/or `mfr_id` members will contain ID bytes.  The entire sentence will be
parsed, perhaps modifying members of `fix`.  You should enable one or both IDs
if you want the information in all sentences *and* you also want to know the ID
bytes.  This adds 2 bytes of RAM for the talker ID, and 3 bytes of RAM for the
manufacturer ID.

2. Enable PARSING the ID:  The virtual `parse_talker_id` and `parse_mfr_id` will
receive each ID character as it is received.  If it is not a valid ID, return
`false` to abort processing the rest of the sentence.  No CPU time will be wasted
on the invalid sentence, and no `fix` members will be modified.  You should
enable this if you want to ignore some IDs.  You must override `parse_talker_id`
and/or `parse_mfr_id` in a derived class.

```
#define NMEAGPS_SAVE_TALKER_ID
#define NMEAGPS_PARSE_TALKER_ID

#define NMEAGPS_SAVE_MFR_ID
#define NMEAGPS_PARSE_MFR_ID
```

###Disable derived types
Although normally enabled, this can be disabled if you *do not*
derive any classes from NMEAGPS, for slightly smaller/faster code.
```
#define NMEAGPS_DERIVED_TYPES
```

###Enable/disable tracking the current satellite array
You can also enable tracking the detailed information for each satellite, and how many satellites you want to track.
Although many GPS receivers claim to have 66 channels of tracking, 16 is usually the maximum number of satellites 
tracked at any one time.
```
#define NMEAGPS_PARSE_SATELLITES
#define NMEAGPS_PARSE_SATELLITE_INFO
#define NMEAGPS_MAX_SATELLITES (20)
```

###Enable/disable accumulating fix data across sentences.
When enabled, the fix data in a new sentence will replace _only_ the members that are in that sentence.  Any fix data that was filled by a previous sentence _is not_ invalidated.  This can eliminate the need for a second `fix` (i.e., reduced RAM), but it could prevent coherency.  Because a sentence has to be parsed to know its timestamp, invalidating old data must be performed _before_ the sentence parsing begins.  By the time DECODE_COMPLETED occurs, new data has been mixed with old data.

It is possible to achieve coherency if you detect the "quiet" time between batches of sentences.  When new data starts coming in, simply call `gps.fix.init()`; all new sentences will set the fix members.  Note that if the GPS device loses its fix on the satellites, you can be left without _any_ valid data.
```
#define NMEAGPS_ACCUMULATE_FIX
```

####ubloxNMEA
This derived class has the following configuration items near the top of ubxNMEA.h:
```
#define NMEAGPS_PARSE_PUBX_00
#define NMEAGPS_PARSE_PUBX_04
```

Streamers.cpp has a choice for using floating-point output.
```
#define USE_FLOAT
```
This is local to this file, and is only used by the example programs.  This file is _not_ required unless you need to stream one of these types: bool, char, uint8_t, int16_t, uint16_t, int32_t, uint32_t, F() strings, `gps_fix` or `NMEAGPS`.

Most example programs have a choice for displaying fix information once per day.  (Untested!)
```
#define PULSE_PER_DAY
```

####Troubleshooting
The compiler will catch any attempt to use parts of a `fix` that have been 
configured out: you will see something like `gps_fix does not have member 
xxx`.

The compiler **cannot** catch message set dependencies: the enum 
`nmea_msg_t` is always available.  So even though a `fix` member is enabled, 
you may have disabled all messages that would have set its value.  
NMEAtest.ino can be used to check some configurations.

####Typical configurations
A few common configurations are defined as follows

**Minimal**: no fix members, no messages (pulse-per-second only)

**DTL**: date, time, lat/lon, GPRMC messsage only.

**Nominal**: date, time, lat/lon, altitude, speed, heading, number of 
satellites, HDOP, GPRMC and GPGGA messages.

**Full**: Nominal plus talker ID, VDOP, PDOP, lat/lon/alt errors, satellite array with satellite info, all messages, and parser statistics.

(**TinyGPS** uses the **Nominal** configuration + a second `fix`.)
