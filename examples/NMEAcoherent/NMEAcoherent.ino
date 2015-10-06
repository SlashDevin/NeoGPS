#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAcoherent.ino
//
//  Prerequisites:
//     1) NMEAfused.ino works with your device
//     3) At least one NMEA sentence has been enabled.
//     4) Implicit Merging is disabled.
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
//  Serial is for trace output to the Serial Monitor window.
//
//======================================================================

#include "GPSport.h"
#include "Streamers.h"
Stream & trace = Serial;

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
    !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
    !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
    !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST )

  #error No NMEA sentences enabled: no fix data available for fusing.

#endif

#ifdef NMEAGPS_ACCUMULATE_FIX
  #error NMEAGPS_ACCUMULATE_FIX should not be enabled when explicit merging is used.
  // This is an Explicit Merge: "coherent |= gps.fix()"
  //
  // If you don't want or need the safe copy 'coherent', you could
  //   use NMEA.ino with NMEAGPS_ACCUMULATE_FIX enabled instead.
#endif

//------------------------------------------------------------

static NMEAGPS  gps         ; // This parses received characters
static uint32_t last_rx = 0L; // The last millis() time a character was
                              // received from GPS.  This is used to
                              // determine when the GPS quiet time begins.
static gps_fix coherent;

static const NMEAGPS::nmea_msg_t LAST_SENTENCE_IN_INTERVAL = NMEAGPS::NMEA_GLL;

//----------------------------------------------------------------

static void doSomeWork()
{
  // Print all the things!
  trace_all( gps, coherent );

  #ifdef NMEAGPS_PARSE_GST
    // Now is a good time to ask for a GST.  Most GPS devices
    //   do not send GST, and some GPS devices may not even
    //   respond to this poll.  Other may let you request
    //   these messages once per second by sending a 
    //   configuration command.
    gps.poll( &Serial1, NMEAGPS::NMEA_GST );

  #endif

} // doSomeWork

//----------------------------------------------------------------

#if defined(GPS_FIX_DATE) & !defined(GPS_FIX_TIME)
  // uncomment this to display just one pulse-per-day.
  //#define PULSE_PER_DAY
#endif

static bool isNewInterval()
{
  // See if we stepped into a different time interval,
  //   or if it has finally become valid after a cold start.

  bool newInterval;

  #if defined(GPS_FIX_TIME)
    newInterval = (gps.fix().valid.time &&
                  (!coherent.valid.time ||
                   (coherent.dateTime.seconds != gps.fix().dateTime.seconds) ||
                   (coherent.dateTime.minutes != gps.fix().dateTime.minutes) ||
                   (coherent.dateTime.hours   != gps.fix().dateTime.hours)));

  #elif defined(GPS_FIX_DATE) && defined(PULSE_PER_DAY)
    newInterval = (gps.fix().valid.date &&
                  (!coherent.valid.date ||
                   (coherent.dateTime.date  != gps.fix().dateTime.date) ||
                   (coherent.dateTime.month != gps.fix().dateTime.month) ||
                   (coherent.dateTime.year  != gps.fix().dateTime.year)));
  
  #else
    //  Time is not configured, so let's assume it's a new interval
    //    if we just received a particular sentence.
    //  Different GPS devices will send sentences in different orders.
    //  This should be the *first* sentence sent by your device in
    //    each 1-second interval, not the last.

    newInterval = (gps.nmeaMessage == NMEAGPS::NMEA_RMC);
  #endif

} // isNewInterval

//------------------------------------

static void GPSloop()
{  
  while (gps_port.available()) {
    last_rx = millis();

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

      if (isNewInterval())
        // Start with fresh data ONLY
        coherent = gps.fix();
      else
        // Explicitly Merge all subsequent data
        coherent |= gps.fix();

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

  trace.print( F("NMEAcoherent: started\n") );
  trace.print( F("fix object size = ") );
  trace.println( sizeof(gps.fix()) );
  trace.print( F("NMEAGPS object size = ") );
  trace.println( sizeof(gps) );
  trace.println( F("Looking for GPS device on " USING_GPS_PORT) );

  trace_header();

  trace.flush();
  
  // Start the UART for the GPS device
  gps_port.begin(9600);
}

//--------------------------

void loop()
{
  GPSloop();
}
