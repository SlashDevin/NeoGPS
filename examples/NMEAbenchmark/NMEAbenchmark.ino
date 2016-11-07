#include <Arduino.h>
#include <NMEAGPS.h>

//======================================================================
//  Program: NMEAbenchmark.ino
//
//  Prerequisites:
//
//  Description:  Use GPGGA and GPRMC sentences to test 
//     the parser's performance.
//
//     GSV sentences are tested if enabled.
//     
//  'Serial' is for debug output to the Serial Monitor window.
//
//======================================================================

#include "Streamers.h"

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
  Serial.println( F("NMEAbenchmark: started") );
  Serial.print( F("fix object size = ") );
  Serial.println( sizeof(gps.fix()) );
  Serial.print( F("  gps object size = ") );
  Serial.println( sizeof(gps) );

  trace_header( Serial );

  Serial.flush();
}

//--------------------------

void loop()
{
  const char *gga =
    "$GPGGA,092725.00,4717.11399,N,00833.91590,E,"
    "1,8,1.01,499.6,M,48.0,M,,0*5B\r\n";
  const char *gga_no_lat =
    "$GPGGA,092725.00,,,00833.91590,E,"
    "1,8,1.01,499.6,M,48.0,M,,0*0D\r\n";

  Serial << F("GGA time = ")        << time_it( gga )        << '\n';
  trace_all( Serial, gps, gps.fix() );

  Serial << F("GGA no lat time = ") << time_it( gga_no_lat ) << '\n';
  trace_all( Serial, gps, gps.fix() );

  const char *rmc =
    "$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,"
    "0.004,77.52,091202,,,A*57\r\n";

  Serial << F("RMC time = ")        << time_it( rmc )        << '\n';
  trace_all( Serial, gps, gps.fix() );

  #ifdef NMEAGPS_PARSE_GSV
    const char *gsv = 
      "$GPGSV,3,1,10,23,38,230,44,29,71,156,47,07,29,116,41,08,09,081,36*7F\r\n"
      "$GPGSV,3,2,10,10,07,189,,05,05,220,,09,34,274,42,18,25,309,44*72\r\n"
      "$GPGSV,3,3,10,26,82,187,47,28,43,056,46*77\r\n";
    Serial << F("GSV time = ") << time_it( gsv ) << '\n';
    trace_all( Serial, gps, gps.fix() );
  #endif

  for (;;);
}
