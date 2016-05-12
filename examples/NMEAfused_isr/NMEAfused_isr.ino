#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAfused_isr.ino
//
//  Description:  This program is an interrupt-driven version 
//    of NMEAfused.ino, and uses the special replacements for the 
//    default Arduino HardwareSerial.
//    NeoHWSerial, NeoSWSerial, or NeoICSerial can also be used.
//
//  Prerequisites:
//     1) You have completed the requirements for NMEAfused.ino
//     2) You have installed NeoHWSerial.
//     3) For non-Mega boards, you have installed NeoICSerial or NeoSWSerial.
//
//  'Serial' is for debug output to the Serial Monitor window.
//
//======================================================================

#if defined( UBRR1H )
  // Default is to use NeoSerial1 when available.
  #include <NeoHWSerial.h>
  // NOTE: There is an issue with IDEs before 1.6.6.  The above include 
  // must be commented out for non-Mega boards, even though it is
  // conditionally included.  If you have a Mega board, uncomment 
  // the above include.
#else  
  // Only one serial port is available, uncomment one of the following:
  //#include <NeoICSerial.h>
  #include <NeoSWSerial.h>
#endif
#include "GPSport.h"

#include "Streamers.h"
#ifdef NeoHWSerial_h
  #define DEBUG_PORT NeoSerial
#else
  #define DEBUG_PORT Serial
#endif

static NMEAGPS  gps; 
static gps_fix  fused;

//----------------------------------------------------------------

static void doSomeWork()
{
  // Print all the things!
  trace_all( DEBUG_PORT, gps, fused );

  // Clear out what we just printed.  If you need this data elsewhere,
  //   don't do this.
  gps.data_init();
  fused.init();

  // NOTE: Because 'gps' is modified by the ISR, using it here is
  // a little risky.  Some values, like the satellite array, could 
  // be modified while the print is occurring.  Depending on which 
  // sentences your GPS device emits and the baudrate, you may need 
  // to make a safe copy of those 'gps' members during the GPSisr, 
  // just like the 'fused' fix data.  Most apps only use fix data.

} // doSomeWork

//------------------------------------
//  GPSisr gets called during the interrupt for a received character.
//    You must not perform anything that takes too long.  Just set
//    a flag or copy some values to global variables.
//    You *cannot* do anything the uses interrupts inside an ISR:
//    no printing, no writing, no sending.
//    Even calling millis() is discouraged, because it depends on
//    interrupts to set the current time, and it takes a fair amount 
//    of time.  Instead, set a flag here and watch for that flag to 
//    change in loop() and call millis() there.

volatile bool fix_complete = false; // set by the ISR and cleared by GPSloop
volatile bool overrun      = false; // set by the ISR when doSomeWork takes too long

static void GPSisr( uint8_t c )
{
  if (gps.decode( c ) == NMEAGPS::DECODE_COMPLETED) {

    if (fix_complete)
      // doSomeWork took too long!
      overrun = true;

    fused |= gps.fix();

    if (gps.nmeaMessage == LAST_SENTENCE_IN_INTERVAL)
      fix_complete = true;
  }

} // GPSisr

//--------------------------

static void GPSloop()
{
  // Wait until all the fix data for this interval arrives.
  //   This is also the start of the GPS quiet time.
  
  if (fix_complete) {
    doSomeWork();
    // delay(900); // simulates taking too long in doSomeWork, causing overrun.
    fix_complete = false;

    // NOTE: you can use the fused fix data in doSomeWork until the 
    // next quiet time interval starts. At that time, fix_data will 
    // be modified by the ISR, perhaps causing corrupted values.  
    // You have almost exactly one second.
    //
    // If you need to access the fused fix data at any part of your 
    // program, independently of the quiet time, you must save 
    // a private copy *now*:
    //     safe_fix = fused;
  }
} // GPSloop

//--------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);  // change this to match 'trace'.  Can't do 'trace.begin'

  DEBUG_PORT.print( F("NMEAfused_isr.INO: started\n") );
  DEBUG_PORT.print( F("fix object size = ") );
  DEBUG_PORT.println( sizeof(gps.fix()) );
  DEBUG_PORT.print( F("NMEAGPS object size = ") );
  DEBUG_PORT.println( sizeof(gps) );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );
  DEBUG_PORT.print( F("GPS quiet time begins after a ") );
  DEBUG_PORT.print( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
  DEBUG_PORT.println( F(" sentence is received.\n"
                   "You should confirm this with NMEAorder.ino") );
  trace_header( DEBUG_PORT );
  DEBUG_PORT.flush();
  
  // Start the UART for the GPS device
  gps_port.attachInterrupt( GPSisr );
  gps_port.begin( 9600 );
}

//--------------------------

void loop()
{
  GPSloop();

  if (overrun) {
    overrun = false;
    DEBUG_PORT.println( F("DATA OVERRUN: doSomeWork took too long!") );
  }
}
