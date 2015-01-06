/*
  Serial is for trace output.
  Serial1 should be connected to the GPS device.
*/

#include <Arduino.h>

#include "NMEAGPS.h"
#include "Streamers.h"

// Set this to your debug output device.
Stream & trace = Serial;

static NMEAGPS gps;

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

  trace_header();

  trace.flush();
  
  // Start the UART for the GPS device
  Serial1.begin(9600);
}

//--------------------------

void loop()
{
  while (Serial1.available()) {
    if (gps.decode( Serial1.read() ) == NMEAGPS::DECODE_COMPLETED) {
//      trace << (uint8_t) gps.nmeaMessage << ' ';

// Make sure that the only sentence we care about is enabled
#ifndef NMEAGPS_PARSE_RMC
#error NMEAGPS_PARSE_RMC must be defined in NMEAGPS.h!
#endif

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC) {
        trace_all( gps, gps.fix() );

        //  Use received GPRMC sentence as a pulse
        seconds++;
      }
    }
  }
}
