Coherency
==========
Coherency guarantees that all members of a fix are from the same GPS time.

If coherency is disabled, lat/long members may have been set by the newest sentence, but the altitude may be from the previous time interval.  Most applications do not care that the fix members are not coherent.  However, if you are controlling a drone or other autonomous vehicle, you may need coherency.

To enable coherency, make sure this line is uncommented in NMEAGPS_cfg.h:
```
#define NMEAGPS_COHERENT
```
NeoGPS achieves coherency by detecting the "quiet" time between batches of sentences.   When new data starts coming in, the fix will get emptied or initialized, and all new sentences will be accumulated in the internal fix.

**NOTE: This requires that you have selected the correct LAST_SENTENCE_IN_INTERVAL.**  If you're not sure which sentence is sent last (and therefore, when the quiet time begins), use NMEAorder.ino to analyze your GPS device.

If the GPS device loses its fix on the satellites, you can be left without _any_ valid data.  If this is not acceptable, you will have save an extra copy of the last good fix structure.

[NMEA.ino](/examples/NMEA/NMEA.ino) can be used with implicit merging.  However,  [NMEAcoherent.ino](/examples/NMEAcoherent/NMEAcoherent.ino) should not be used with implicit merging, because they perform their own explicit merging for coherency.

The main reason implicity merging cannot be used with coherency is because a sentence has to be parsed to know its timestamp.  If it were implicitly merged, the old data would not have been invalidated.  Invalidating data from a previous update period must be performed _before_ the sentence parsing begins.  That can only be accomplished with a second 'safe' copy of the fix data and explicit merging (i.e., FIX_MAX >= 1).  With implicit merging, new data has already been mixed with old data by the time DECODE_COMPLETED occurs and timestamps can be checked.
