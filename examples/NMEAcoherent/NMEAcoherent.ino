/*
  Serial is for trace output.
  Serial1 should be connected to the GPS device.
*/

#include <Arduino.h>

#include "Streamers.h"

// Set this to your debug output device.
Stream & trace = Serial;

#include "NMEAGPS.h"

#if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
    !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
    !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
    !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST )

#if defined(GPS_FIX_DATE)| defined(GPS_FIX_TIME)
#error No NMEA sentences enabled: no fix data available for fusing.
#else
#warning No NMEA sentences enabled: no fix data available for fusing,\n\
 only pulse-per-second is available.
#endif

#endif

#if defined(GPS_FIX_DATE) & !defined(GPS_FIX_TIME)
// uncomment this to display just one pulse-per-day.
//#define PULSE_PER_DAY
#endif

static NMEAGPS gps;

static gps_fix fused;

//--------------------------

static void sentenceReceived()
{
  // See if we stepped into a different time interval,
  //   or if it has finally become valid after a cold start.

  bool newInterval;
#if defined(GPS_FIX_TIME)
  newInterval = (gps.fix().valid.time &&
                (!fused.valid.time ||
                 (fused.dateTime.Second != gps.fix().dateTime.Second) ||
                 (fused.dateTime.Minute != gps.fix().dateTime.Minute) ||
                 (fused.dateTime.Hour   != gps.fix().dateTime.Hour)));
#elif defined(PULSE_PER_DAY)
  newInterval = (gps.fix().valid.date &&
                (!fused.valid.date ||
                 (fused.dateTime.Day   != gps.fix().dateTime.Day) ||
                 (fused.dateTime.Month != gps.fix().dateTime.Month) ||
                 (fused.dateTime.Year  != gps.fix().dateTime.Year)));
#else
  //  No date/time configured, so let's assume it's a new interval
  //  if the seconds have changed.
  static uint32_t last_sentence = 0L;
  
  newInterval = (seconds != last_sentence);
  last_sentence = seconds;
#endif

  if (newInterval) {

    //  Since we're into the next time interval, we throw away
    //     all of the previous fix and start with what we
    //     just received.
    fused = gps.fix();

  } else {
    // Accumulate all the reports in this time interval into a /coherent/ fix
    fused |= gps.fix();
  }

} // sentenceReceived

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);
  trace.print( F("NMEAcoherent: started\n") );
  trace.print( F("fix object size = ") );
  trace.println( sizeof(gps.fix()) );
  trace.print( F("NMEAGPS object size = ") );
  trace.println( sizeof(NMEAGPS) );

  trace_header();

  trace.flush();
  
  // Start the UART for the GPS device
  Serial1.begin(9600);
}

//--------------------------

void loop()
{
  static uint32_t last_rx = 0L;

  while (Serial1.available()) {
    last_rx = millis();

    if (gps.decode( Serial1.read() ) == NMEAGPS::DECODE_COMPLETED) {

      // All enabled sentence types will be merged into one fix
      sentenceReceived();

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC)
        //  Use received GPRMC sentence as a pulse
        seconds++;
    }
  }

  // Print things out once per second, after the serial input has died down.
  // This prevents input buffer overflow during printing.

  static uint32_t last_trace = 0L;

  if ((last_trace != seconds) && (millis() - last_rx > 5)) {
    last_trace = seconds;

    // It's been 5ms since we received anything, log what we have so far...
    trace_all( gps, fused );
  }
}
