Tradeoffs
=========

There's a price for everything, hehe...

####Configurability means that the code is littered with #ifdef sections.

I've tried to increase white space and organization to make it more readable, but let's be honest... 
conditional compilation is ugly.

####Accumulating parts means knowing which parts are valid.

Before accessing a part, you must check its `valid` flag.  Fortunately, this adds only one bit per member.  See [Streamers.cpp](/src/Streamers.cpp#L100) for an example of accessing every data member.  That file also shows how to accommodate different builds: all references to 'gps_fix' members are wrapped with conditional compilation `#ifdef`/`#endif` statements.  If you do not plan to support multiple configurations, you do not need to use `#ifdef`/`#endif` statements.

####Parsing without buffers, or *in place*, means that you must be more careful about when you access data items.

In general, you should wait to access the fix until after the entire sentence has been parsed.  Most of the examples simply `decode` until a sentence is COMPLETED, then do all their work with `fix`.  See `loop()` in [NMEA.ino](/examples/NMEA/NMEA.ino). 
Member function `is_safe()` can also be used to determine when it is safe.

If you need to access the fix at any time, you will have to double-buffer the fix: simply copy the `fix` when it is safe to do so.  (See NMEAGPS.h comments regarding a
`safe_fix`.)  Also, received data errors can cause invalid field values to be set *before* the CRC is fully computed.  The CRC will
catch most of those, and the fix members will then be marked as invalid.

####Accumulating parts into *one* fix means less RAM but more complicated code

By enabling `NMEAGPS_ACCUMULATE_FIX`, the fix will accumulate data from all received sentences.  Each
fix member will contain the last value received from any sentence that
contains that information.  While it avoids the need for a second copy of the merged fix, it has several restrictions:
* Fix members can only be accessed while it `is_safe()`.  There is no double-buffered fix.
* Fix members may contain information from different time intervals (i.e., they are not 
coherent).  It is possible to acheive coherency if the `fix` is re-initialzed at the correct time.
* All fix members may be invalidated if a received sentence is rejected for any reason (e.g., CRC
error).  No members will be valid until new sentences are received, parsed, accepted and *safe*.  Your application
must accommodate possible gaps in fix availability.

You are not restricted from having other instances of fix; you can copy or merge the accumulating fix into another copy if you want.  This is just a way to minimize RAM requirements and still have a fused fix.

####Full C++ OO implementation is more advanced than most Arduino libraries.

You've been warned!  ;)

####"fast, good, cheap... pick two."

Although most of the RAM reduction is due to eliminating buffers, some of it is from trading RAM
for additional code (see **Nominal** Program Space above).  And, as I mentioned, the readabilty (i.e., goodness) suffers from its configurability.

