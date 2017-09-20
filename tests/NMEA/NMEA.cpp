//======================================================================
//  Description:  This program uses the fix-oriented methods available() and
//    read() to handle complete fix structures.
//
//    When the last character of the LAST_SENTENCE_IN_INTERVAL (see NMEAGPS_cfg.h)
//    is decoded, a completed fix structure becomes available and is returned
//    from read().  The new fix is saved the 'fix' structure, and can be used
//    anywhere, at any time.
//
//    If no messages are enabled in NMEAGPS_cfg.h, or
//    no 'gps_fix' members are enabled in GPSfix_cfg.h, no information will be
//    parsed, copied or printed.
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

// We fake the GPS using FakeGPS.
#include <FakeGPS.h>

// platform.h defines delctype(std::cout) as the NEO_GPS_PRINT type.
// We can therefore pass std::cout to all functions which expect a
// NEO_GPS_PRINT object.
#include <iostream>

#include <NMEAGPS.h>

#include <Streamers.h>

//------------------------------------------------------------
// This object parses received characters
//   into the gps.fix() data structure

static NMEAGPS  gps;

//------------------------------------------------------------
//  Define a set of GPS fix information.  It will
//  hold on to the various pieces as they are received from
//  an RMC sentence.  It can be used anywhere in your sketch.

static gps_fix  fix;

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

  trace_all( DEBUG_PORT, gps, fix );

} // doSomeWork

//------------------------------------
//  This is the main GPS parsing loop.

static void GPSloop(FakeGPS & fakeGPS)
{
  while (gps.available( fakeGPS )) {
    fix = gps.read();
    doSomeWork();
  }

} // GPSloop

//--------------------------

void outputHeader()
{
  DEBUG_PORT << "NMEA.cpp: started" << std::endl;
  DEBUG_PORT << "  fix object size = " << +sizeof(gps.fix()) << std::endl;
  DEBUG_PORT << "  gps object size = " << +sizeof(gps) << std::endl;

  #ifndef NMEAGPS_RECOGNIZE_ALL
    #error You must define NMEAGPS_RECOGNIZE_ALL in NMEAGPS_cfg.h!
  #endif

  #ifdef NMEAGPS_INTERRUPT_PROCESSING
    #error You must *NOT* define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
  #endif

  #if !defined( NMEAGPS_PARSE_GGA ) && !defined( NMEAGPS_PARSE_GLL ) && \
      !defined( NMEAGPS_PARSE_GSA ) && !defined( NMEAGPS_PARSE_GSV ) && \
      !defined( NMEAGPS_PARSE_RMC ) && !defined( NMEAGPS_PARSE_VTG ) && \
      !defined( NMEAGPS_PARSE_ZDA ) && !defined( NMEAGPS_PARSE_GST )

    DEBUG_PORT << std::endl << "WARNING: No NMEA sentences are enabled: no fix data will be displayed.";

  #else
    if (gps.merging == NMEAGPS::NO_MERGING) {
      DEBUG_PORT << std::endl;
      DEBUG_PORT << "WARNING: displaying data from " << gps.string_for( LAST_SENTENCE_IN_INTERVAL )
                 << " sentences ONLY, and only if " << gps.string_for( LAST_SENTENCE_IN_INTERVAL )
                 << " is enabled." << std::endl;
      DEBUG_PORT << "  Other sentences may be parsed, but their data will not be displayed.";
    }
  #endif

  DEBUG_PORT << std::endl << "GPS quiet time is assumed to begin after a "
             << gps.string_for( LAST_SENTENCE_IN_INTERVAL )
             << " sentence is received." << std::endl;
  DEBUG_PORT << "  You should confirm this with NMEAorder.cpp" << std::endl << std::endl;

  trace_header( DEBUG_PORT );
}

//--------------------------


int main() {
  outputHeader();
  auto fakeGPS = FakeGPS(fake_gps_content::INTERNET_SAMPLE, true);
  for (;;) {
      GPSloop(fakeGPS);
  }
  return 0;
}
