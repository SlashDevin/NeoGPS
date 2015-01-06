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

static NMEAGPS gps;

static gps_fix fused;

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);
  trace.print( F("NMEAfused: started\n") );
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
      fused |= gps.fix();

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
