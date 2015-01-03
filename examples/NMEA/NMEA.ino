/*
  Serial is for trace output.
  Serial1 should be connected to the GPS device.
*/

#include <Arduino.h>

#include "NMEAGPS.h"
#include "Streamers.h"

#if !defined(GPS_FIX_DATE) & !defined(GPS_FIX_TIME)
static uint32_t seconds = 0L;
#endif

static NMEAGPS gps;

//--------------------------

static void sentenceReceived()
{
#if !defined(GPS_FIX_DATE) & !defined(GPS_FIX_TIME)
  //  Date/Time not enabled, just output the interval number
  trace << seconds << ',';
#endif

  trace << gps.fix();

#if defined(NMEAGPS_PARSE_SATELLITES)
  if (gps.fix().valid.satellites) {
    trace << ',' << '[';

    uint8_t i_max = gps.fix().satellites;
    if (i_max > NMEAGPS::MAX_SATELLITES)
      i_max = NMEAGPS::MAX_SATELLITES;

    for (uint8_t i=0; i < i_max; i++) {
      trace << gps.satellites[i].id;
#if defined(NMEAGPS_PARSE_SATELLITE_INFO)
      trace << ' ' << 
        gps.satellites[i].elevation << '/' << gps.satellites[i].azimuth;
      trace << '@';
      if (gps.satellites[i].tracked)
        trace << gps.satellites[i].snr;
      else
        trace << '-';
#endif
      trace << ',';
    }
    trace << ']';
  }

#else

#ifdef GPS_FIX_SATELLITES
  trace << gps.fix().satellites << ',';
#endif

#endif

  trace << '\n';

} // sentenceReceived

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);
  trace.print( F("NMEA test: started\n") );
  trace.print( F("fix object size = ") );
  trace.println( sizeof(gps.fix()) );
  trace.print( F("NMEAGPS object size = ") );
  trace.println( sizeof(NMEAGPS) );
  trace.flush();
  
  // Start the UART for the GPS device
  Serial1.begin(9600);
}

//--------------------------

void loop()
{
  while (Serial1.available())
    if (gps.decode( Serial1.read() ) == NMEAGPS::DECODE_COMPLETED) {
//      trace << (uint8_t) gps.nmeaMessage << ' ';

// Make sure that the only sentence we care about is enabled
#ifndef NMEAGPS_PARSE_RMC
#error NMEAGPS_PARSE_RMC must be defined in NMEAGPS.h!
#endif

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC) {
        sentenceReceived();
#if !defined(GPS_FIX_DATE) & !defined(GPS_FIX_TIME)
        //  No date/time fields enabled, use received GPRMC sentence as a pulse
        seconds++;
#endif
      }
    }
}
