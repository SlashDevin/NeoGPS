#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAorder.ino
//
//  Description:  This example program records the order of sentences 
//    received in each 1-second interval.  The last sentence is 
//    important to know, as that will be used to determine when the 
//    GPS quiet time is starting (see NMEA.ino).  It is safe to perform
//    time-consuming operations at that point, and the risk of losing
//    characters will be minimized (see comments in 'GPSloop').
//
//  Prerequisites:
//     1) Your GPS device has been correctly powered.
//          Be careful when connecting 3.3V devices.
//     2) Your GPS device is correctly connected to an Arduino serial port.
//          See GPSport.h for the default connections.
//     3) You know the default baud rate of your GPS device
//          Use NMEAdiagnostic.ino to scan for the correct baud rate.
//
//  'Serial' is for trace output to the Serial Monitor window.
//
//======================================================================

#include "GPSport.h"
#include "Streamers.h"
Stream & trace = Serial;

static NMEAGPS  gps         ; // This parses received characters
static uint32_t last_rx = 0L; // The last millis() time a character was
                              // received from GPS.  This is used to
                              // determine when the GPS quiet time begins.

//------------------------------------------------------------

static NMEAGPS::nmea_msg_t last_sentence_in_interval = NMEAGPS::NMEA_UNKNOWN;
static       uint8_t       prev_sentence_count       = 0;
static       uint8_t       sentence_count            = 0;
static const uint8_t       MAX_SENTENCES             = 20; // per second
static NMEAGPS::nmea_msg_t sentences[ MAX_SENTENCES ];

static void recordSentenceTypes()
{
  // Always save the last sentence, even if we're full
  sentences[ sentence_count ] = gps.nmeaMessage;
  if (sentence_count < MAX_SENTENCES-1)
    sentence_count++;
  
} // recordSentenceTypes

//-----------------------------------------------------------  

static void printSentenceOrder()
{
  trace << F("\nSentence order in each 1-second interval:\n  ");

  for (uint8_t i=0; i<sentence_count; i++)
    trace <<
      (const __FlashStringHelper *)
        gps.string_for( sentences[i] ) << F("\n  ");

  trace << '\n';

} // printSentenceOrder

//------------------------------------

static void GPSloop()
{  
  while (gps_port.available()) {
    last_rx = millis();

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

      if (last_sentence_in_interval == NMEAGPS::NMEA_UNKNOWN) {
        // Still building the list
        recordSentenceTypes();
        trace << '.';

      }
    }
  }
} // GPSloop

//----------------------------------------------------------------
//  Determine whether the GPS quiet time has started.
//
//  This is only needed in the example programs, which must work 
//  for *any* GPS device.
//
//  It also "pretends" to have a quiet time once per 
//  second so that some debug messages are emitted.  This allows 
//  beginners to see whether the GPS device is correctly 
//  connected and functioning.

static bool quietTimeStarted()
{
  uint32_t current_ms       = millis();
  uint32_t ms_since_last_rx = current_ms - last_rx;

  if (ms_since_last_rx > 5) {

    // The GPS device has not sent any characters for at least 5ms.
    //   See if we've been getting chars sometime during the last second.
    //   If not, the GPS may not be working or connected properly.

           bool     getting_chars   = (ms_since_last_rx < 1000UL);
    static uint32_t last_quiet_time = 0UL;
           bool     just_went_quiet =
                            (((int32_t) (last_rx - last_quiet_time)) > 0L);
           bool     next_quiet_time =
                               ((current_ms - last_quiet_time) >= 1000UL);

    if ((getting_chars && just_went_quiet)
          ||
        (!getting_chars && next_quiet_time)) {

      last_quiet_time = current_ms;  // Remember for next loop

      //  If we're not getting good data, make some suggestions.
      
      bool allDone = false;

      if (!getting_chars) {
        
        trace.println( F("\nCheck GPS device and/or connections.  No characters received.\n") );
        
        allDone = true;

      } else if (sentence_count == 0) {
        
        trace.println( F("No valid sentences, but characters are being received.\n"
        "Check baud rate or device protocol configuration.\n" ) );

        allDone = true;
      }

      if (allDone) {
        trace << F("\nEND.\n");
        for (;;)
          ; // hang!
      }

      // No problem, just a real GPS quiet time.
      return true;
    }
  }

  return false;
  
} // quietTimeStarted

//----------------------------------------------------------------
//  Figure out what sentence the GPS device sends 
//  as the last sentence in each 1-second interval.

static void watchForLastSentence()
{
  if (quietTimeStarted()) {

    if (prev_sentence_count != sentence_count) {
    
      // We have NOT received two full intervals of sentences with
      //    the same number of sentences in each interval.  Start
      //    recording again.
      prev_sentence_count = sentence_count;
      sentence_count      = 0;
      
    } else if (sentence_count > 0) {

      // Got it!
      last_sentence_in_interval = sentences[ sentence_count-1 ];
    }
  }

} // watchForLastSentence

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);  // change this to match 'trace'.  Can't do 'trace.begin'

  trace.print( F("NMEAorder.INO: started\n") );
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

  if (last_sentence_in_interval == NMEAGPS::NMEA_UNKNOWN)
    watchForLastSentence();
  else {

    printSentenceOrder();
    for (;;)
      ; // All done!
  }
}
