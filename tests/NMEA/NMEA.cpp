//======================================================================
//  Description:  This program uses the fix-oriented methods available() and
//    read() to handle complete fix structures.
//
//    When the last character of the LAST_SENTENCE_IN_INTERVAL (see NMEAGPS_cfg.h)
//    is decoded, a completed fix structure becomes available and is returned
//    from read().  The new fix is saved the 'fix_data' structure, and can be used
//    anywhere, at any time.
//
//    If no messages are enabled in NMEAGPS_cfg.h, or
//    no 'gps_fix' members are enabled in GPSfix_cfg.h, no information will be
//    parsed, copied or printed.
//
//  Prerequisites:
//     1) Your GPS device has been correctly powered.
//     2) Your GPS device is correctly connected using a serial adapter.
//          By default /dev/ttyUSB0 is used.
//     3) You know the default baud rate of your GPS device.
//          By default 9600 is assumed.  If this doesn't work change it in serial.cpp
//     4) LAST_SENTENCE_IN_INTERVAL is defined to be the sentence that is
//          sent *last* in each update interval (usually once per second).
//          The default is NMEAGPS::NMEA_RMC (see NMEAGPS_cfg.h).  Other
//          programs may need to use the sentence identified by NMEAorder.ino.
//     5) NMEAGPS_RECOGNIZE_ALL is defined in NMEAGPS_cfg.h
//
//======================================================================

//------------------------------------------------------------
// For the NeoGPS example programs, "Streamers" is common set
//   of printing and formatting routines for GPS data, in a
//   Comma-Separated Values text format (aka CSV).  The CSV
//   data will be printed to the "debug output device".
// If you don't need these formatters, simply delete this section.

// platform.h must be included before NMEAGPS.h
#include <platform.h>
#include <FakeGPS.h>
#include <NMEAGPS.h>

#include <Streamers.h>

#include <iostream>

//------------------------------------------------------------
// This object parses received characters
//   into the gps.fix() data structure

static NMEAGPS  gps;

//------------------------------------------------------------
//  Define a set of GPS fix information.  It will
//  hold on to the various pieces as they are received from
//  an RMC sentence.  It can be used anywhere in your sketch.

static gps_fix  fix_data;

//----------------------------------------------------------------
//  This function gets called about once per second, during the GPS
//  quiet time.  It's the best place to do anything that might take
//  a while: print a bunch of things, write to SD, send an SMS, etc.
//
//  By doing the "hard" work during the quiet time, the CPU can get back to
//  reading the GPS chars as they come in, so that no chars are lost.

static auto & DEBUG_PORT = std::cout;

static void doSomeWork()
{
  // Print all the things!

  trace_all( DEBUG_PORT, gps, fix_data );

} // doSomeWork

//------------------------------------
//  This is the main GPS parsing loop.

static void GPSloop(FakeGPS & fakeGPS)
{
  while (gps.available( fakeGPS )) {
    fix_data = gps.read();
    doSomeWork();
  }

} // GPSloop

//--------------------------

int main() {
  auto fakeGPS = FakeGPS(fake_gps_content::INTERNET_SAMPLE, true);
  for (;;) {
      GPSloop(fakeGPS);
  }
  return 0;
}
