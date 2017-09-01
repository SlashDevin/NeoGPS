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
#include <platforms/linux/platform.h>

#include <platforms/linux/GpsPort.h>
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

static void GPSloop(GpsPort gps_port)
{
  while (gps.available( gps_port )) {
    fix_data = gps.read();
    doSomeWork();
  }

} // GPSloop

//--------------------------

GpsPort setup(const char * usbDev, const char * speed)
{
  // Start the normal trace output
  DEBUG_PORT << "NMEA: started\n";
  DEBUG_PORT << "  fix object size = ";
  DEBUG_PORT << (uint32_t)sizeof(gps.fix());
  DEBUG_PORT << "\n  gps object size = ";
  DEBUG_PORT << (uint32_t)sizeof(gps);
  DEBUG_PORT << "\nLooking for GPS device on ";
  DEBUG_PORT << ( usbDev == nullptr ? "default" : usbDev );
  DEBUG_PORT << " with speed ";
  DEBUG_PORT << ( speed == nullptr ? "default" : speed );

  #ifndef NMEAGPS_RECOGNIZE_ALL
    #error You must define NMEAGPS_RECOGNIZE_ALL in NMEAGPS_cfg.h!
  #endif

  #ifdef NMEAGPS_INTERRUPT_PROCESSING
    #error You must *NOT* define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
  #endif

  #if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
      !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
      !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
      !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST )

    DEBUG_PORT << ( F("\nWARNING: No NMEA sentences are enabled: no fix data will be displayed.\n") );

  #else
    if (gps.merging == NMEAGPS::NO_MERGING) {
      DEBUG_PORT << F("\nWARNING: displaying data from ");
      DEBUG_PORT << gps.string_for( LAST_SENTENCE_IN_INTERVAL );
      DEBUG_PORT << F(" sentences ONLY, and only if ");
      DEBUG_PORT << gps.string_for( LAST_SENTENCE_IN_INTERVAL );
      DEBUG_PORT << F(" is enabled.\n"
                            "  Other sentences may be parsed, but their data will not be displayed.");
      DEBUG_PORT << "\n";
    }
  #endif

  DEBUG_PORT << "\nGPS quiet time is assumed to begin after a ";
  DEBUG_PORT << gps.string_for( LAST_SENTENCE_IN_INTERVAL );
  DEBUG_PORT << " sentence is received.\n"
                    "  You should confirm this with NMEAorder.ino\n";
  DEBUG_PORT << "\n";
  
  GpsPort gps_port = GpsPort(usbDev, speed);

  trace_header( DEBUG_PORT );
  
  return gps_port;
}

//--------------------------

int main(int argc, char *argv[]) {
  auto dev = argc >= 2 ? argv[1] : nullptr;
  auto speed = argc >=3 ? argv[2] : nullptr;
  
  auto gps_port = setup(dev, speed);
  for (;;) {
      GPSloop(gps_port);
  }
  return 0;
}
