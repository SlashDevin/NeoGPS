#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEA.ino
//
//  Description:  This program saves RMC information in a fix structure 
//     that can be used anywhere, at any time.  If the RMC message is
//     disabled, or all 'gps_fix' members are disabled, no information 
//     will be copied or printed.
//
//     It also does a little diagnosis if the baud rate is wrong or 
//     the wrong LAST_SENTENCE_IN_INTERVAL is used.
//
//  Prerequisites:
//     1) Your GPS device has been correctly powered.
//          Be careful when connecting 3.3V devices.
//     2) Your GPS device is correctly connected to an Arduino serial port.
//          See GPSport.h for the default connections.
//     3) You know the default baud rate of your GPS device.
//          If 9600 does not work, use NMEAdiagnostic.ino to 
//          scan for the correct baud rate.
//     4) You know the last sentence sent in each 1-second interval.
//          Modify the '#define LAST_SENTENCE_IN_INTERVAL' in
//          NMEAGPS_cfg.h to be the sentence identified by NMEAorder.ino.
//          The default is RMC.
//
//  'Serial' is for debug output to the Serial Monitor window.
//
//======================================================================

//-------------------------------------------------------------------------
//  The GPSport.h include file tries to choose a default serial port 
//  for the GPS device.  If you know which serial port you want to use,
//  declare it here:
//
//    SoftwareSerial gps_port( rxpin, txpin ); // to GPS TX, RX
//          or
//    HardwareSerial & gps_port = Serial2; // an alias
//          or
//    Search and replace all occurrences of "gps_port" with your port's name.
//
//  See Installation instructions for additional information.

#if defined( UBRR1H )
  // Default is to use Serial1 when available.  You could also
  // use NeoHWSerial, especially if you want to handle GPS characters
  // in an Interrupt Service Routine.
  //#include <NeoHWSerial.h>
#else  
  // Only one serial port is available, uncomment one of the following:
  //#include <NeoICSerial.h>
  #include <NeoSWSerial.h>
  //#include <SoftwareSerial.h> /* NOT RECOMMENDED */
#endif

#include "GPSport.h"

//------------------------------------------------------------
// For the NeoGPS example programs, "Streamers" is common set 
//   of printing and formatting routines for GPS data, in a
//   Comma-Separated Values text format (aka CSV).  The CSV
//   data will be printed to the "debug output device".
// If you don't need these formatters, simply delete this section.

#include "Streamers.h"

//------------------------------------------------------------
// When NeoHWSerial is used, none of the built-in HardwareSerial
//   variables can be used: Serial, Serial1, Serial2 and Serial3
//   *cannot* be used.  Instead, you must use the corresponding
//   NeoSerial, NeoSerial1, NeoSerial2 or NeoSerial3.  This define
//   is used to substitute the appropriate Serial variable in
//   all debug prints below.

#ifdef NeoHWSerial_h
  #define DEBUG_PORT NeoSerial
#else
  #define DEBUG_PORT Serial
#endif

//------------------------------------------------------------
// This object parses received characters 
//   into the gps.fix() data structure

static NMEAGPS  gps; 

//------------------------------------------------------------
//  Define an extra set of GPS fix information.  It will
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

static void doSomeWork()
{
  // Print all the things!

  trace_all( DEBUG_PORT, gps, fix_data );

  // Clear out what we just printed.  If you need this data elsewhere,
  //   don't do this.

  gps.data_init();
  fix_data.init();

} // doSomeWork

//------------------------------------
//  This is the main GPS parsing loop.

static void GPSloop()
{  
  while (gps_port.available()) {

    //  Are we just getting garbage?
    static uint32_t bad_chars_received = 0;
    bad_chars_received++;
    if (bad_chars_received > 1000) {
      bad_chars_received = 0;
      DEBUG_PORT.println( "Invalid data received.  Use NMEAdiagnostic.INO to verify baud rate." );
    }

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {
      bad_chars_received = 0;

      // If you have something quick to do, you can safely use gps.fix()
      //   members now.  For example, comparing the current speed
      //   against some limits and setting a flag would be ok.  Declare
      //   a global flag at the top of the file somewhere...
      //
      //     bool tooFast = false;
      //
      //   ...and set it right here, if a valid speed has been received:
      //
      //     if (gps.fix().valid.speed)
      //       tooFast = (gps.fix().speed() > 15.0); // nautical mph
      //
      // DO NOT do any printing or writing to an SD card *here*.
      //   Those operations can take a long time and may cause data loss.  
      //   Instead, do those things in 'doSomeWork'.
      //
      // If you just need one piece of fix data, like the current second,
      //    you could copy one value like this:
      //
      //      if (gps.fix().valid.time)
      //        seconds = gps.fix().dateTime.seconds;
      //
      // If you need to use several pieces of the latest GPS data anywhere
      //   in your program, at any time, you can save a a complete copy of
      //   the entire GPS fix data collection, but you must do it *now*.  

      // This example program only copies the data from an RMC.
      //   You could use any criteria to filter or sort the
      //   data: talker ID, speed, satellites, etc.
      //
      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC) {
        fix_data = gps.fix();

        // NOTE: you may not need a copy of 'gps.fix()' if 
        //   'doSomeWork' is the only routine that uses fix data,
        //   *and* the RMC sentence is the only sentence you need.
      }

      // If this happens to be the last sentence in a 1-second interval,
      //   the GPS quiet time is beginning, and it is safe to do some
      //   time-consuming work.

      static bool last_sentence_received = false;
      if (gps.nmeaMessage == LAST_SENTENCE_IN_INTERVAL) {

        last_sentence_received = true;
        doSomeWork();

      } else if (!last_sentence_received) {

        // Print out some diagnostics about LAST_SENTENCE_IN_INTERVAL and
        // why it may not have been received yet:
        //   1) The sketch just started, and we're getting the first
        //      few sentences; or
        //   2) We're waiting for the *wrong* last sentence.
        // This will also provide a little positive feedback
        // at the beginning, even if the last sentence never arrives.

        static uint8_t sentences_printed = 0;
        if (sentences_printed < 20) {

          sentences_printed++;
          DEBUG_PORT.print( F("Received ") );
          DEBUG_PORT.print( gps.string_for( gps.nmeaMessage ) );
          DEBUG_PORT.println( F("...") );

        } else if (sentences_printed == 20) {

          sentences_printed++;
          DEBUG_PORT.print( F("Warning: ") );
          DEBUG_PORT.print( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
          DEBUG_PORT.println( F(" sentence was never received and is not the LAST_SENTENCE_IN_INTERVAL.\n"
                            "  Please use NMEAorder.ino to determine which sentences your GPS device sends, and then\n"
                            "  use the last one for the definition above.") );
        }
      }
    }
  }
} // GPSloop

//--------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);

  DEBUG_PORT.print( F("NMEA.INO: started\n") );
  DEBUG_PORT.print( F("fix object size = ") );
  DEBUG_PORT.println( sizeof(gps.fix()) );
  DEBUG_PORT.print( F("NMEAGPS object size = ") );
  DEBUG_PORT.println( sizeof(gps) );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );
  DEBUG_PORT.println( F("Only displaying data from xxRMC sentences.  Other sentences may be parsed, but their data will not be displayed.") );
  DEBUG_PORT.print( F("GPS quiet time begins after a ") );
  DEBUG_PORT.print( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
  DEBUG_PORT.println( F(" sentence is received.\n"
                   "You should confirm this with NMEAorder.ino") );
  trace_header( DEBUG_PORT );
  DEBUG_PORT.flush();
  
  // Start the UART for the GPS device
  gps_port.begin( 9600 );
}

//--------------------------

void loop()
{
  GPSloop();
}
