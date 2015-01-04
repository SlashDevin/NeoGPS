/*
  Use GPGGA and GPRMC sentences to test the parser's performance.
  Serial is for trace output.
*/

#include <Arduino.h>

#include "NMEAGPS.h"
#include "Streamers.h"

// Set this to your debug output device.
Stream & trace = Serial;

/*
 * Make sure gpsfix.h and NMEAGPS.h are configured properly.
 */

#ifndef NMEAGPS_PARSE_GGA
#error NMEAGPS_PARSE_GGA must be defined in NMEAGPS.h!
#endif

#ifndef NMEAGPS_PARSE_RMC
#error NMEAGPS_PARSE_RMC must be defined in NMEAGPS.h!
#endif

static NMEAGPS gps;

//--------------------------

static uint32_t time_it( const char *data )
{
  const uint16_t ITERATIONS = 1024;
  uint32_t start, end;
  
  Serial.flush();
  start = micros();
  for (uint16_t i=ITERATIONS; i > 0; i--) {
    char *ptr = (char *) data;
    while (*ptr)
      gps.decode( *ptr++ );
  }
  end = micros();

  return (end-start)/ITERATIONS;
}

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);
  trace.println( F("NMEAbenchmark: started") );
  trace << F("fix object size = ") << sizeof(gps.fix()) << '\n';
  trace << F("NMEAGPS object size = ") << sizeof(NMEAGPS) << '\n';
  Serial.flush();
}

//--------------------------

void loop()
{
  const char *gga =
    "$GPGGA,092725.00,4717.11399,N,00833.91590,E,"
    "1,8,1.01,499.6,M,48.0,M,,0*5B\r\n";
  const char *rmc =
    "$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,"
    "0.004,77.52,091202,,,A*57\r\n";

  trace << F("GGA time = ") << time_it( gga ) << '\n';
  trace << F("RMC time = ") << time_it( rmc ) << '\n';

  for (;;);
}
