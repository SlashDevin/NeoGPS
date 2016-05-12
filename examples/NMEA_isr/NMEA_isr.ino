#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEA_isr.ino
//
//  Prerequisites:
//     1) NMEA.ino works with your device
//
//  Description:  This program parses the GPS data during the RX character
//     interrupt.  The ISR will set a flag when a new GPS data has
//     been received.  This flag can be tested elsewhere.  When you have 
//     used the GPS data, simply clear the flag.
//======================================================================

#if defined( UBRR1H )
  // Default is to use Serial1 when available.  You could also
  // use NeoHWSerial, especially if you want to handle GPS characters
  // in an Interrupt Service Routine.
  //#error You must comment this line out and uncomment the next include
  #include <NeoHWSerial.h>
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

static NMEAGPS   gps;
static volatile bool newData = false;
static volatile bool overrun = false; // set by the ISR when doSomeWork takes too long

//--------------------------

static void GPSisr( uint8_t c )
{
  if (gps.decode( c ) == NMEAGPS::DECODE_COMPLETED) {
    if (gps.nmeaMessage == NMEAGPS::NMEA_RMC) { // or your favorite
      if (newData)
        // took too long to use the previous data!
        overrun = true;
      else
        newData = true;
    }
  }

} // GPSisr

//--------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);

  DEBUG_PORT.print( F("NMEA_isr.INO: started\n") );
  DEBUG_PORT.print( F("fix object size = ") );
  DEBUG_PORT.println( sizeof(gps.fix()) );
  DEBUG_PORT.print( F("NMEAGPS object size = ") );
  DEBUG_PORT.println( sizeof(gps) );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );
  DEBUG_PORT.println( F("Only parsing xxRMC sentences.  All other sentences will be ignored.") );
  trace_header( DEBUG_PORT );
  DEBUG_PORT.flush();

  // Start the UART for the GPS device
  gps_port.attachInterrupt( GPSisr );
  gps_port.begin( 9600 );
}

//--------------------------

void loop()
{
  if (newData) {
    // Print all the things!
    trace_all( DEBUG_PORT, gps, gps.fix() );
    newData = false;
  }

  if (overrun) {
    overrun = false;
    DEBUG_PORT.println( F("DATA OVERRUN: took too long to use gps.fix()!") );
  }
}
