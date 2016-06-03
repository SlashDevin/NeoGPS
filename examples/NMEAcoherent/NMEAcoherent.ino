#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAcoherent.ino
//
//  Prerequisites:
//     1) NMEAfused.ino works with your device
//     2) At least one NMEA sentence has been enabled.
//     3) Implicit Merging is disabled.
//
//  Description:  This program guarantees coherency in the fix data.  
//     When a sentence is received with a new time interval, 
//     the 'coherent' fix will start with just that new data.  
//     All data from the previous interval is replaced or deleted.  
//     As new sentences are received, data from this new interval 
//     are Explicitly Merged into the 'coherent' fix.
//     
//     This program also shows how to 'poll' for a specific message 
//     if it is not sent by default.
//
//  Serial is for debug output to the Serial Monitor window.
//
//======================================================================

#if defined( UBRR1H )
  // Default is to use Serial1 when available.  You could also
  // use NeoHWSerial, especially if you want to handle GPS characters
  // in an Interrupt Service Routine.
  //#include <NeoHWSerial.h>
#else  
  // Only one serial port is available, uncomment one of the following:
  //#include <NeoICSerial.h>
  //#include <AltSoftSerial.h>
  #include <NeoSWSerial.h>
  //#include <SoftwareSerial.h> /* NOT RECOMMENDED */
#endif
#include "GPSport.h"

#include "Streamers.h"
#ifdef NeoHWSerial_h
  #define DEBUG_PORT NeoSerial
#else
  #define DEBUG_PORT Serial
#endif

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
    !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
    !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
    !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST ) & \
    !defined( NMEAGPS_RECOGNIZE_ALL )

  #error No NMEA sentences enabled

#endif

#ifndef NMEAGPS_COHERENT
  #error You must define NMEAGPS_COHERENT in NMEAGPS_cfg.h!
#endif

//------------------------------------------------------------

static NMEAGPS gps;
static gps_fix coherent;
static bool    building; // true if 'coherent' is in the process of accumulating
                         // fix pieces for the current interval.
//----------------------------------------------------------------

static void doSomeWork( const gps_fix & coherent )
{
  // Print all the things!
  trace_all( DEBUG_PORT, gps, coherent );

  #ifdef NMEAGPS_PARSE_GST
    // Now is a good time to ask for a GST.  Most GPS devices
    //   do not send GST, and some GPS devices may not even
    //   respond to this poll.  Other may let you request
    //   these messages once per second by sending a 
    //   configuration command in setup().
    gps.poll( &gps_port, NMEAGPS::NMEA_GST );

  #endif

} // doSomeWork

//------------------------------------

static void GPSloop()
{
  static bool assignNext = false; // write over 'coherent' next time instead of merging

  while (gps.available( gps_port )) {
    const gps_fix & fix = gps.read();

    if (gps.merging == NMEAGPS::IMPLICIT_MERGING) {
      coherent = fix;
      building = false; // all ready!
      doSomeWork( coherent );
    } else {
      if (assignNext) {
        assignNext = false;
        building   = true;
        coherent   = fix; // replace everything from the last interval
      } else
        coherent  |= fix; // merge one sentence

      if (gps.intervalComplete()) {
        assignNext = true;
        building   = false; // all ready!
        doSomeWork( coherent );
        // This leaves 'coherent' available for other parts of your sketch
        //   until the next interval begins.
      }
    }
  }

  if (gps.merging == NMEAGPS::IMPLICIT_MERGING)
    building = !gps.intervalComplete();

} // GPSloop
  
//--------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("NMEAcoherent: started\n") );
  DEBUG_PORT.print( F("fix object size = ") );
  DEBUG_PORT.println( sizeof(gps.fix()) );
  DEBUG_PORT.print( F("NMEAGPS object size = ") );
  DEBUG_PORT.println( sizeof(gps) );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );

  // Check configuration

  #if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
      !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
      !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
      !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST ) & \
      defined( NMEAGPS_RECOGNIZE_ALL )

    DEBUG_PORT.println( F("Warning: no sentences are enabled, so no fix data is available for fusing.") );
  #endif

  trace_header( DEBUG_PORT );

  DEBUG_PORT.flush();
  
  // Start the UART for the GPS device
  gps_port.begin(9600);

  #ifdef NMEAGPS_PARSE_GST
    gps.poll( &gps_port, NMEAGPS::NMEA_GST );
  #endif
}

//--------------------------

void loop()
{
  GPSloop();
}
