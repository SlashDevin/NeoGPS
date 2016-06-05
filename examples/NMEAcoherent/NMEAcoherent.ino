#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAcoherent.ino
//
//  Prerequisites:
//     1) NMEA.ino works with your device
//     2) At least one NMEA sentence has been enabled in NMEAGPS_cfg.h
//     3) NMEAGPS_COHERENT is enabled in NMEAGPS_cfg.h
//     3) Explicit or Implicit merging is enabled in NMEAGPS_cfg.h
//
//  Description:  This program guarantees coherency in the fix data.  
//     When a sentence is received with a new time interval, 
//     the 'coherent' fix will start with just that new data.  
//     All data from the previous interval is replaced or deleted.  
//     As new sentences are received, data from this new interval 
//     are merged into the 'coherent' fix.
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

#ifdef NMEAGPS_NO_MERGING
  #error You must define EXPLICIT or IMPLICIT merging in NMEAGPS_cfg.h!
#endif

#ifdef NMEAGPS_INTERRUPT_PROCESSING
  #error You must *NOT* define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
#endif

//------------------------------------------------------------

static NMEAGPS gps;
static gps_fix coherent;

//----------------------------------------------------------------

static void doSomeWork( const gps_fix & fix )
{
  // Print all the things!
  trace_all( DEBUG_PORT, gps, fix );

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
  while (gps.available( gps_port )) {
    coherent = gps.read();
    doSomeWork( coherent );
  }

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

    DEBUG_PORT.println( F("\nWARNING: No NMEA sentences are enabled: no fix data will be displayed.") );
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
