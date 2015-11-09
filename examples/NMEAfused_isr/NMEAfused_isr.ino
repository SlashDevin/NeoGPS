#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAfused_isr.ino
//
//  Description:  This program is an interrupt-driven version 
//    of NMEAfused.ino, and uses the special NeoHWSerial replacement for the 
//    default Arduino HardwareSerial.  (NeoICSerial could also be used)
//
//  Prerequisites:
//     1) You have completed the requirements for NMEAfused.ino
//     2) You have installed NeoHWSerial or NeoICSerial.
//
//  'NeoSerial' is for trace output to the Serial Monitor window.
//
//======================================================================

#include <NeoHWSerial.h>
//#include <NeoICSerial.h>
#include "GPSport.h"
#include "Streamers.h"
Stream & trace = NeoSerial;

static NMEAGPS  gps; 
static gps_fix  fused;

static const NMEAGPS::nmea_msg_t LAST_SENTENCE_IN_INTERVAL = NMEAGPS::NMEA_GLL;

//----------------------------------------------------------------

static void doSomeWork()
{
  // Print all the things!
  trace_all( gps, fused );

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

volatile bool quiet_time = false; // a flag set by the ISR and cleared by GPSloop

static void GPSisr( uint8_t c )
{
  if (gps.decode( c ) == NMEAGPS::DECODE_COMPLETED) {

    fused |= gps.fix();

    if (gps.nmeaMessage == LAST_SENTENCE_IN_INTERVAL)
      quiet_time = true;
  }

} // GPSisr

//--------------------------

static void GPSloop()
{
  // Wait for the quiet time to begin...
  
  if (quiet_time) {
    quiet_time = false;
    doSomeWork();

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
  NeoSerial.begin(9600);  // change this to match 'trace'.  Can't do 'trace.begin'

  NeoSerial.print( F("NMEAfused_isr.INO: started\n") );
  NeoSerial.print( F("fix object size = ") );
  NeoSerial.println( sizeof(gps.fix()) );
  NeoSerial.print( F("NMEAGPS object size = ") );
  NeoSerial.println( sizeof(gps) );
  NeoSerial.println( F("Looking for GPS device on " USING_GPS_PORT) );
  NeoSerial.print( F("GPS quiet time begins after a ") );
  NeoSerial.print( (const __FlashStringHelper *) gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
  NeoSerial.println( F(" sentence is received.\n"
                   "You should confirm this with NMEAorder.ino") );
  trace_header();
  NeoSerial.flush();
  
  // Start the UART for the GPS device
  gps_port.attachInterrupt( GPSisr );
  gps_port.begin( 9600 );
}

//--------------------------

void loop()
{
  GPSloop();
}
