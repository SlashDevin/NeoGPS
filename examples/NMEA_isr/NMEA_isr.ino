#include <Arduino.h>
#include <NMEAGPS.h>

//======================================================================
//  Program: NMEA_isr.ino
//
//  Prerequisites:
//     1) NMEA.ino works with your device
//
//  Description:  This minimal program parses the GPS data during the 
//     RX character interrupt.  The ISR passes the character to
//     the GPS object for parsing.  The GPS object will add gps_fix 
//     structures to a buffer that can be later read() by loop().
//======================================================================

#if defined( UBRR1H ) | defined( ID_USART0 )
  // Default is to use NeoSerial1 when available.  You could also
  #include <NeoHWSerial.h>
  // NOTE: There is an issue with IDEs before 1.6.6.  The above include 
  // must be commented out for non-Mega boards, even though it is
  // conditionally included.  If you are using an earlier IDEs, 
  // comment the above include.
#else  
  // Only one serial port is available, uncomment one of the following:
  //#include <NeoICSerial.h>
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

// Check configuration

#ifndef NMEAGPS_INTERRUPT_PROCESSING
  #error You must define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
#endif

static NMEAGPS   gps;

//--------------------------

static void GPSisr( uint8_t c )
{
  gps.handle( c );

} // GPSisr

//--------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("NMEA_isr.INO: started\n") );
  DEBUG_PORT.print( F("fix object size = ") );
  DEBUG_PORT.println( sizeof(gps.fix()) );
  DEBUG_PORT.print( F("NMEAGPS object size = ") );
  DEBUG_PORT.println( sizeof(gps) );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );

  trace_header( DEBUG_PORT );

  DEBUG_PORT.flush();

  // Start the UART for the GPS device
  gps_port.attachInterrupt( GPSisr );
  gps_port.begin( 9600 );
}

//--------------------------

void loop()
{

  if (gps.available()) {
    // Print all the things!
    trace_all( DEBUG_PORT, gps, gps.read() );
  }

  if (gps.overrun()) {
    gps.overrun( false );
    DEBUG_PORT.println( F("DATA OVERRUN: took too long to use gps.read() data!") );

  }
}
