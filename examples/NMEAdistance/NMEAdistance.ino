#include <Arduino.h>
#include <NMEAGPS.h>

//======================================================================
//  Program: NMEAdistance.ino
//
//  Description:  Display distance from a base location.
//
//  Prerequisites:
//     1) NMEA.ino works with your device (correct TX/RX pins and baud rate)
//     2) GPS_FIX_LOCATION has been enabled.
//     3) A sentence that contains lat/long has been enabled (GGA, GLL or RMC).
//     4) Your device sends at least one of those sentences.
//
//  'Serial' is for debug output to the Serial Monitor window.
//
//======================================================================

#if defined( UBRR1H ) | defined( ID_USART0 )
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

#ifdef NeoHWSerial_h
  #define DEBUG_PORT NeoSerial
#else
  #define DEBUG_PORT Serial
#endif

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_RMC ) &  \
    !defined( NMEAGPS_PARSE_GGA ) &  \
    !defined( NMEAGPS_PARSE_GLL )
  #error You must uncomment at least one of NMEAGPS_PARSE_RMC, NMEAGPS_PARSE_GGA or NMEAGPS_PARSE_GLL in NMEAGPS_cfg.h!
#endif

#if !defined( GPS_FIX_LOCATION )
  #error You must uncomment GPS_FIX_LOCATION in GPSfix_cfg.h!
#endif

NMEAGPS gps;

// The base location, in degrees * 10,000,000
NeoGPS::Location_t base( -253448688L, 1310324914L ); // Ayers Rock, AU

void setup()
{
  DEBUG_PORT.begin(9600);
  DEBUG_PORT.println( F("NMEAdistance.ino started.") );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );

  gps_port.begin(9600);

} // setup

void loop()
{
  while (gps.available( gps_port )) {
    gps_fix fix = gps.read(); // save the latest

    // When we have a location, calculate how far away we are from the base location.
    if (fix.valid.location) {
      float range = fix.location.DistanceMiles( base );

      DEBUG_PORT.print( F("Range: ") );
      DEBUG_PORT.print( range );
      DEBUG_PORT.println( F(" Miles") );
    } else
      // Waiting...
      DEBUG_PORT.print( '.' );
  }
} // loop
