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
//  Prerequisites:
//     1) Your GPS device has been correctly powered.
//          Be careful when connecting 3.3V devices.
//     2) Your GPS device is correctly connected to an Arduino serial port.
//          See GPSport.h for the default connections.
//     3) You know the default baud rate of your GPS device.
//          If 9600 does not work, use NMEAdiagnostic.ino to 
//          scan for the correct baud rate.
//     4) You know the last sentence sent in each 1-second interval.
//          Use NMEAorder.ino to list the sentence order.
//
//  'Serial' is for trace output to the Serial Monitor window.
//
//======================================================================

//-------------------------------------------------------------------------
//  This include file will choose a default serial port for the GPS device.
//    If you know which serial port you want to use, delete declare it here:
//
//    SomeKindOfSerial gps_port( args );
//          or
//    HardwareSerial & gps_port = Serialx; // an alias
//          or
//    Search and replace all occurrences of "gps_port" with your port's name.

#include "GPSport.h"

//------------------------------------------------------------
// For the NeoGPS example programs, "Streamers" is common set 
//   of printing and formatting routines for GPS data, in a
//   Comma-Separated Values text format (aka CSV).  The CSV
//   data will be printed to the "debug output device", called
//   "trace".  It's just an alias for the debug Stream.
//   Set "trace" to your debug output device, if it's not "Serial".
// If you don't need these formatters, simply delete this section.

#include "Streamers.h"
Stream & trace = Serial;

static NMEAGPS  gps         ; // This parses received characters

//------------------------------------------------------------
//  Define an extra set of GPS fix information.  It will
//  hold on to the various pieces as they are received from
//  an RMC sentence.  It can be used anywhere in your sketch.

static gps_fix  fix_data;

//------------------------------------------------------------
//  Identify the last sentence sent by the GPS device in each
//    1-second interval.  After this message is sent, the GPS
//    device will be quiet until the next 1-second interval.
//
//  If you're not sure what sentences are sent by your device,
//    you should use NMEAorder.ino to list them.

static const NMEAGPS::nmea_msg_t LAST_SENTENCE_IN_INTERVAL = NMEAGPS::NMEA_GLL;

//----------------------------------------------------------------
//  This function gets called about once per second, during the GPS
//  quiet time.  It's the best place to do anything that might take 
//  a while: print a bunch of things, write to SD, send an SMS, etc.
//
//  By doing the "hard" work during the quiet time, the CPU can get back to
//  reading the GPS chars as they come in, so that no chars are lost.

static void doSomeWork()
{
  // Display the header, just once.

  static bool header_printed = false;
  if (!header_printed) {

    trace.print( F("GPS quiet time begins after a ") );
    trace.print( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
    trace.println( F(" sentence is received.\n"
                     "You should confirm this with NMEAorder.ino") );

    #if defined(GPS_FIX_TIME)
      trace.print( F("Local time,") );
    #endif
    trace_header();

    header_printed = true;
  }

  #if defined(GPS_FIX_TIME)
    // Display the local time
    if (fix_data.valid.time) {
      static const int32_t         zone_hours   = -4L; // EST
      static const int32_t         zone_minutes =  0L;
      static const NeoGPS::clock_t zone_offset  =
                        zone_hours   * NeoGPS::SECONDS_PER_HOUR +
                        zone_minutes * NeoGPS::SECONDS_PER_MINUTE;

      trace << NeoGPS::time_t( fix_data.dateTime + zone_offset );
    }
    trace << ',';
  #endif

  // Print all the things!
  trace_all( gps, fix_data );

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

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

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
      // NOTE: you may not need a copy of 'gps.fix()' if 
      //   'doSomeWork' is the only routine that uses fix data,
      //   *and* the RMC sentence is the only sentence you need.

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC)
        fix_data = gps.fix();

      // If this happens to be the last sentence in a 1-second interval,
      //   the GPS quiet time is beginning, and it is safe to do some
      //   time-consuming work.

      if (gps.nmeaMessage == LAST_SENTENCE_IN_INTERVAL)
        doSomeWork();

    }
  }
} // GPSloop

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);  // change this to match 'trace'.  Can't do 'trace.begin'

  trace.print( F("NMEA.INO: started\n") );
  trace.print( F("fix object size = ") );
  trace.println( sizeof(gps.fix()) );
  trace.print( F("NMEAGPS object size = ") );
  trace.println( sizeof(gps) );
  trace.println( F("Looking for GPS device on " USING_GPS_PORT) );
  trace.flush();
  
  // Start the UART for the GPS device
  gps_port.begin( 9600 );
}

//--------------------------

void loop()
{
  GPSloop();
}
