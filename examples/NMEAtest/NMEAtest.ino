/*
  This test program uses one GPGGA sentence to test the parser's:
  1) robustness WRT dropped, inserted, and mangled characters
  2) correctness WRT values extracted from the input stream
  Some care in testing must be taken because
  1) The XOR-style checksum is not very good at catching errors.  
  2) The '*' is a special character for delimiting the CRC.  If
     it is changed, a CR/LF will allow the sentence to pass.

  Serial is for trace output.
*/

#include <Arduino.h>

#include "Streamers.h"

#include "NMEAGPS.h"

/*
 * Make sure gpsfix.h and NMEAGPS.h are configured properly.
 */

#ifndef NMEAGPS_PARSE_GGA
#error NMEAGPS_PARSE_GGA must be defined in NMEAGPS.h!
#endif

#ifndef GPS_FIX_DATE
#error GPS_FIX_DATE must be defined in gpsfix.h!
#endif

#ifndef GPS_FIX_TIME
#error GPS_FIX_TIME must be defined in gpsfix.h!
#endif

#ifndef GPS_FIX_LOCATION
#error GPS_FIX_LOCATION must be defined in gpsfix.h!
#endif

#ifndef GPS_FIX_ALTITUDE
#error GPS_FIX_ALTITUDE must be defined in gpsfix.h!
#endif

#ifndef GPS_FIX_SPEED
#error GPS_FIX_SPEED must be defined in gpsfix.h!
#endif

#ifndef GPS_FIX_HEADING
#error GPS_FIX_HEADING must be defined in gpsfix.h!
#endif

#ifndef GPS_FIX_SATELLITES
#error GPS_FIX_SATELLITES must be defined in gpsfix.h!
#endif

#ifndef GPS_FIX_HDOP
#error GPS_FIX_HDOP must be defined in gpsfix.h!
#endif

static NMEAGPS gps;

//--------------------------

static uint8_t passed = 0;
static uint8_t failed = 0;

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


  //  Some basic rejection tests
  for (uint16_t c=0; c < 256; c++) {
    if (c != '$') {
      if (NMEAGPS::DECODE_CHR_INVALID != gps.decode( (char)c )) {
        trace.print( F("FAILED to reject single character ") );
        trace.println( c );
        failed++;
        return;
      }
    }
  }
  passed++;

  for (uint16_t i=0; i < 256; i++) {
    if (NMEAGPS::DECODE_COMPLETED == gps.decode( '$' )) {
      trace.print( F("FAILED to reject multiple '$' characters\n") );
      failed++;
      return;
    }
  }
  passed++;

  // An example sentence
  const char *validGGA = (const char *) F("$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,8,1.01,499.6,M,48.0,M,,0*5B\r\n");
  uint8_t validGGA_len = 0;

  // Insert a ' ' at each position of the test sentence
  uint16_t insert_at=1;
  do {
    const char *ptr = validGGA;
    uint8_t j = 0;
    for (;;) {
      if (j++ == insert_at) {
        if (NMEAGPS::DECODE_COMPLETED == gps.decode( ' ' )) {
          trace.print( F("FAILED inserting ' ' @ pos ") );
          trace.println( insert_at );
          failed++;
          return;
        }
      }
      char c = pgm_read_byte( ptr++ );
      if (!c) {
        if (validGGA_len == 0) {
          validGGA_len = j-1;
          trace.print( F("Test string length = ") );
          trace.println( validGGA_len );
        }
        break;
      }
      if (NMEAGPS::DECODE_COMPLETED == gps.decode( c )) {
        trace.print( F("FAILED inserting @ pos ") );
        trace.println( insert_at );
        failed++;
        return;
      }
    }
  } while (++insert_at < validGGA_len-2);
  passed++;

  // Drop one character from each position in example sentence
  for (uint16_t i=0; i < validGGA_len-3; i++) {
    const char *ptr = validGGA;
    uint8_t j = 0;
    char dropped = 0;
    for (;;) {
      char c = pgm_read_byte( ptr++ );
      if (!c || (c == '*')) break;
      if (j == i) dropped = c;
      if ((j++ != i) && (gps.decode( c ) == NMEAGPS::DECODE_COMPLETED)) {
        trace.print( F("FAILED dropping '") );
        trace << dropped;
        trace.print( F("' at pos ") );
        trace.println( i );
        failed++;
        break;
        //return;
      }
    }
  }
  passed++;

  // Mangle one character from each position in example sentence
  for (uint16_t i=0; i < validGGA_len-3; i++) {
    const char *ptr = validGGA;
    uint8_t j = 0;
    char replaced = 0;
    for (;;) {
      char c = pgm_read_byte( ptr++ );
      if (!c || (c == '*')) break;
      if (j++ == i)
        replaced = c++; // mangle means increment
      if (NMEAGPS::DECODE_COMPLETED == gps.decode( c )) {
        trace.print( F("FAILED replacing '") );
        trace << (uint8_t) replaced;
        trace.print( F("' with '") );
        trace << (uint8_t) (replaced+1);
        trace.print( F("' at pos ") );
        trace.println( i );
        failed++;
        break;
        //return;
      }
    }
  }
  passed++;

  //  Verify that exact values are extracted
  {
    const char *ptr = validGGA;
    for (;;) {
      char c = pgm_read_byte( ptr++ );
      if (!c) {
        trace.print( F("FAILED to parse \"") );
        trace.print( (str_P) validGGA );
        trace.println( F("\"\n") );
        failed++;
        break;
      }
      if (NMEAGPS::DECODE_COMPLETED == gps.decode( c )) {
        gps_fix expected; // "2002-12-09 09:27:25"
        expected.dateTime.Year = 32;
        expected.dateTime.Month = 12;
        expected.dateTime.Day = 9;
        expected.dateTime.Hour = 9;
        expected.dateTime.Minute = 27;
        expected.dateTime.Second = 25;
        expected.dateTime_cs = 0;
// $GPRMC,092725.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A
// $GPGGA,092725.00,4717.11399,N,00833.91590,E,1,8,1.01,499.6,M,48.0,M,,0*5B
        if (gps.nmeaMessage != NMEAGPS::NMEA_GGA) {
          trace.print( F("FAILED wrong message type ") );
          trace.println( gps.nmeaMessage );
          failed++;
          break;
        }
        if ((gps.fix().dateTime.Hour   != expected.dateTime.Hour) ||
            (gps.fix().dateTime.Minute != expected.dateTime.Minute) ||
            (gps.fix().dateTime.Second != expected.dateTime.Second) ||
            (gps.fix().dateTime_cs     != expected.dateTime_cs)) {
          trace.print( F("FAILED wrong time ") );
          trace << gps.fix().dateTime << '.' << gps.fix().dateTime_cs;
          trace.print( F(" != ") );
          trace << expected.dateTime << '.';
          trace  << expected.dateTime_cs << '\n';
          failed++;
          break;
        }
        if (gps.fix().latitudeL() != 472852332) {
          trace.print( F("FAILED wrong latitude ") );
          trace.println( gps.fix().latitudeL() );
          failed++;
          break;
        }
        if (gps.fix().longitudeL() != 85652650) {
          trace.print( F("FAILED wrong longitude ") );
          trace.println( gps.fix().longitudeL() );
          failed++;
          break;
        }
        if (gps.fix().status != gps_fix::STATUS_STD) {
          trace.print( F("FAILED wrong status ") );
          trace.println( gps.fix().status );
          failed++;
          break;
        }
        if (gps.fix().satellites != 8) {
          trace.print( F("FAILED wrong satellites ") );
          trace.println( gps.fix().satellites );
          failed++;
          break;
        }
        if (gps.fix().hdop != 1010) {
          trace.print( F("FAILED wrong HDOP ") );
          trace.println( gps.fix().hdop );
          failed++;
          break;
        }
        if (gps.fix().altitude_cm() != 49960) {
          trace.print( F("FAILED wrong altitude ") );
          trace.println( gps.fix().longitudeL() );
          failed++;
          break;
        }
        break;
      }
    }
  }
  passed++;
}

//--------------------------

void loop()
{
  trace.print( F("PASSED ") );
  trace << passed;
  trace.println( F(" tests.") );
  if (failed) {
    trace.print( F("FAILED ") );
    trace << failed;
    trace.println( F(" tests.") );
  }

  for (;;);
}
