#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAtest.ino
//
//  Prerequisites:
//     1) All NMEA standard messages and Satellite Information
//             are enabled.
//     2) All 'gps_fix' members are enabled.
//
//  Description:  This test program uses one GPGGA sentence 
//  to test the parser's:
//     1) robustness WRT dropped, inserted, and mangled characters
//     2) correctness WRT values extracted from the input stream
//     
//  Some care in testing must be taken because
//     1) The XOR-style checksum is not very good at catching errors.  
//     2) The '*' is a special character for delimiting the CRC.  If
//        it is changed, a CR/LF will allow the sentence to pass.
//
//  'Serial' is for trace output to the Serial Monitor window.
//
//======================================================================

#include "Streamers.h"
Stream & trace = Serial;

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined(NMEAGPS_PARSE_GGA) | \
    !defined(NMEAGPS_PARSE_GLL) | \
    !defined(NMEAGPS_PARSE_GSA) | \
    !defined(NMEAGPS_PARSE_GST) | \
    !defined(NMEAGPS_PARSE_GSV) | \
    !defined(NMEAGPS_PARSE_RMC) | \
    !defined(NMEAGPS_PARSE_VTG) | \
    !defined(NMEAGPS_PARSE_ZDA)

  #error NMEAGPS_PARSE_GGA, GLL, GSA, GSV, RMC, VTG and ZDA must be defined in NMEAGPS_cfg.h!
#endif

#ifndef GPS_FIX_DATE
  #error GPS_FIX_DATE must be defined in GPSfix_cfg.h!
#endif

#ifndef GPS_FIX_TIME
  #error GPS_FIX_TIME must be defined in GPSfix_cfg.h!
#endif

#ifndef GPS_FIX_LOCATION
  #error GPS_FIX_LOCATION must be defined in GPSfix_cfg.h!
#endif

#ifndef GPS_FIX_ALTITUDE
  #error GPS_FIX_ALTITUDE must be defined in GPSfix_cfg.h!
#endif

#ifndef GPS_FIX_SPEED
  #error GPS_FIX_SPEED must be defined in GPSfix_cfg.h!
#endif

#ifndef GPS_FIX_HEADING
  #error GPS_FIX_HEADING must be defined in GPSfix_cfg.h!
#endif

#ifndef GPS_FIX_SATELLITES
  #error GPS_FIX_SATELLITES must be defined in GPSfix_cfg.h!
#endif

#ifndef GPS_FIX_HDOP
  #error GPS_FIX_HDOP must be defined in GPSfix_cfg.h!
#endif

static NMEAGPS gps;

//--------------------------
// Example sentences

const char validGGA[] __PROGMEM =
  "$GPGGA,092725.00,4717.11399,N,00833.91590,E,"
    "1,8,1.01,499.6,M,48.0,M,,0*5B\r\n";

// Ayers Rock
//  -25.3448688,131.0324914
//  2520.692128,S,13101.949484,E
const char validRMC[] __PROGMEM =
  "$GPRMC,092725.00,A,2520.69213,S,13101.94948,E,"
    "0.004,77.52,091202,,,A*43\r\n";

// Macchu Picchu
//  -13.162805, -72.545508
//  13.162805,S,72.545508,W
//  1309.7683,S,7232.7305,W

const char validGGA2[] __PROGMEM =
  "$GPGGA,162254.00,1309.7683,S,7232.7305,W,"
    "1,03,2.36,2430.2,M,-25.6,M,,*7E\r\n";

// Dexter MO
//  36.794405, -89.958655
//  36.794405,N,89.958655,W
//  3647.6643,N,8957.5193,W

// Ni'ihau, HI
//   21.827621, -160.244876
//   21.827621,N,160.244876,W
//   2149.65726,N,16014.69256,W

const char validRMC2[] __PROGMEM =
  "$GPRMC,162254.00,A,3647.6643,N,8957.5193,W,0.820,188.36,110706,,,A*49\r\n";

const char validRMC3[] __PROGMEM =
"$GPRMC,235959.99,A,2149.65726,N,16014.69256,W,8.690,359.99,051015,9.47,E,A*26\r\n";

// Some place in Kenya
const char validGLL[] __PROGMEM =
"$GNGLL,0105.60764,S,03701.70233,E,225627.00,A,A*6B\r\n";

const char mtk1[] __PROGMEM =
"$GPGGA,064951.000,2307.1256,N,12016.4438,E,1,8,0.95,39.9,M,17.8,M,,*63\r\n";
const char mtk2[] __PROGMEM =
"$GPRMC,064951.000,A,2307.1256,N,12016.4438,E,0.03,165.48,260406,3.05,W,A*2C\r\n";
const char mtk3[] __PROGMEM =
"$GPVTG,165.48,T,,M,0.03,N,0.06,K,A*36\r\n";
const char mtk4[] __PROGMEM =
"$GPGSA,A,3,29,21,26,15,18,09,06,10,,,,,2.32,0.95,2.11*00\r\n";
const char mtk5[] __PROGMEM =
"$GPGSV,3,1,09,29,36,029,42,21,46,314,43,26,44,020,43,15,21,321,39*7D\r\n";
const char mtk6[] __PROGMEM =
"$GPGSV,3,2,09,18,26,314,40,09,57,170,44,06,20,229,37,10,26,084,37*77\r\n";
const char mtk7[] __PROGMEM =
"$GPGSV,3,3,09,07,,,26*73\r\n";
const char mtk8[] __PROGMEM =
"$GNGST,082356.00,1.8,,,,1.7,1.3,2.2*60\r\n";
const char mtk9[] __PROGMEM =
"$GNRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A,V*33\r\n";
const char mtk10[] __PROGMEM =
"$GNGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*45\r\n";
const char mtk11[] __PROGMEM =
  "$GLZDA,225627.00,21,09,2015,00,00*70\r\n";

const char fpGGA1[] __PROGMEM = "$GPGGA,092725.00,3242.9000,N,11705.8169,W,"
  "1,8,1.01,499.6,M,48.0,M,,0*49\r\n";
const char fpGGA2[] __PROGMEM = "$GPGGA,092725.00,3242.9000,N,11705.8170,W,"
  "1,8,1.01,499.6,M,48.0,M,,0*41\r\n";
const char fpGGA3[] __PROGMEM = "$GPGGA,092725.00,3242.9000,N,11705.8171,W,"
  "1,8,1.01,499.6,M,48.0,M,,0*40\r\n";
const char fpGGA4[] __PROGMEM = "$GPGGA,092725.00,3242.9000,N,11705.8172,W,"
  "1,8,1.01,499.6,M,48.0,M,,0*43\r\n";
const char fpGGA5[] __PROGMEM = "$GPGGA,092725.00,3242.9000,N,11705.8173,W,"
  "1,8,1.01,499.6,M,48.0,M,,0*42\r\n";
const char fpGGA6[] __PROGMEM = "$GPGGA,092725.00,3242.9000,N,11705.8174,W,"
  "1,8,1.01,499.6,M,48.0,M,,0*45\r\n";
const char fpGGA7[] __PROGMEM = "$GPGGA,092725.00,3242.9000,N,11705.8175,W,"
  "1,8,1.01,499.6,M,48.0,M,,0*44\r\n";
const char fpGGA8[] __PROGMEM = "$GPGGA,092725.00,3242.9000,N,11705.8176,W,"
  "1,8,1.01,499.6,M,48.0,M,,0*47\r\n";

//--------------------------

static bool parse_P( const char *ptr )
{
    bool decoded = false;
    char c;

    gps.fix().init();
    while ( (c = pgm_read_byte( ptr++ )) != '\0' ) {
      if (NMEAGPS::DECODE_COMPLETED == gps.decode( c )) {
        decoded = true;
      }
    }

    return decoded;
}

//--------------------------

static void traceSample( const char *ptr )
{
    trace << F("Input:  ") << (const __FlashStringHelper *) ptr;

    bool decoded = parse_P( ptr );

    if (decoded)
      trace << F("Results:  ");
    else
      trace << F("Failed to decode!  ");

    trace_all( gps, gps.fix() );
    trace << '\n';
}

//--------------------------

static uint8_t passed = 0;
static uint8_t failed = 0;

static void checkLatLon
  ( const char *msg, NMEAGPS::nmea_msg_t msg_type, int32_t lat, int32_t lon )
{
  const char *ptr = msg;
  for (;;) {
    char c = pgm_read_byte( ptr++ );
    if (!c) {
      trace.print( F("FAILED to parse \"") );
      trace.print( (const __FlashStringHelper *) msg );
      trace.println( F("\"\n") );
      failed++;
      break;
    }
    if (NMEAGPS::DECODE_COMPLETED == gps.decode( c )) {
      bool ok = true;

      if (gps.nmeaMessage != msg_type) {
        trace.print( F("FAILED wrong message type ") );
        trace.println( gps.nmeaMessage );
        failed++;
        ok = false;
      }
      if (gps.fix().latitudeL() != lat) {
        trace.print( F("FAILED wrong latitude ") );
        trace.println( gps.fix().latitudeL() );
        failed++;
        ok = false;
      }
      if (gps.fix().longitudeL() != lon) {
        trace.print( F("FAILED wrong longitude ") );
        trace.println( gps.fix().longitudeL() );
        failed++;
        ok = false;
      }

      if (ok)
        passed++;
      break;
    }
  }
}

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);

  trace.print( F("NMEA test: started\n") );
  trace.print( F("fix object size = ") );
  trace.println( sizeof(gps.fix()) );
  trace.print( F("NMEAGPS object size = ") );
  trace.println( sizeof(gps) );
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

  uint16_t validGGA_len = 0;

  // Insert a ' ' at each position of the test sentence
  uint16_t insert_at = 1;
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
        gps_fix expected;
        expected.dateTime.parse( PSTR("2002-12-09 09:27:25") );
        expected.dateTime_cs = 0;

        if (gps.nmeaMessage != NMEAGPS::NMEA_GGA) {
          trace.print( F("FAILED wrong message type ") );
          trace.println( gps.nmeaMessage );
          failed++;
          break;
        }
        if ((gps.fix().dateTime.hours   != expected.dateTime.hours  ) ||
            (gps.fix().dateTime.minutes != expected.dateTime.minutes) ||
            (gps.fix().dateTime.seconds != expected.dateTime.seconds) ||
            (gps.fix().dateTime_cs      != expected.dateTime_cs)) {
          trace << F("FAILED wrong time ") << gps.fix().dateTime << '.' << gps.fix().dateTime_cs << F(" != ") << expected.dateTime << '.' << expected.dateTime_cs << '\n';
          failed++;
          break;
        }
        if (gps.fix().latitudeL() != 472852332L) {
          trace.print( F("FAILED wrong latitude ") );
          trace.println( gps.fix().latitudeL() );
          failed++;
          break;
        }
        if (gps.fix().longitudeL() != 85652650L) {
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

  checkLatLon( validRMC , NMEAGPS::NMEA_RMC, -253448688L,  1310324913L );
  checkLatLon( validGGA2, NMEAGPS::NMEA_GGA, -131628050L,  -725455083L );
  checkLatLon( validRMC2, NMEAGPS::NMEA_RMC,  367944050L,  -899586550L );
  checkLatLon( validRMC3, NMEAGPS::NMEA_RMC,  218276210L, -1602448760L );
  checkLatLon( validGLL , NMEAGPS::NMEA_GLL,  -10934607L,   370283722L );
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
  } else {
    trace << F("------ Samples ------\nResults format:\n  ");
    trace_header();
    trace << '\n';

#ifdef NMEAGPS_STATS
    gps.statistics.init();
#endif

    traceSample( validGGA );
    traceSample( validRMC );
    traceSample( validRMC2 );
    traceSample( validRMC3 );
    traceSample( validGLL );
    traceSample( mtk1 );
    traceSample( mtk2 );
    traceSample( mtk3 );
    traceSample( mtk4 );
    traceSample( mtk5 );
    traceSample( mtk6 );
    traceSample( mtk7 );
    traceSample( mtk8 );
    traceSample( mtk9 );
    traceSample( mtk10 );
    traceSample( mtk11 );
    if (!gps.fix().valid.date              ||
        (gps.fix().dateTime.date    != 21) ||
        (gps.fix().dateTime.month   != 9)  ||
        (gps.fix().dateTime.year    != 15) ||
        !gps.fix().valid.time              ||
        (gps.fix().dateTime.hours   != 22) ||
        (gps.fix().dateTime.minutes != 56) ||
        (gps.fix().dateTime.seconds != 27))
      trace << F("********  ZDA not parsed correctly **********\n");

    /**
     * This next section displays incremental longitudes.
     * If you have defined USE_FLOAT in Streamers.cpp, this will show
     * how the conversion to /float/ causes loss of accuracy compared 
     * to the /uint32_t/ values.
     */
    trace << F("--- floating point conversion tests ---\n\n");

    traceSample( fpGGA1 );
    traceSample( fpGGA2 );
    traceSample( fpGGA3 );
    traceSample( fpGGA4 );
    traceSample( fpGGA5 );
    traceSample( fpGGA6 );
    traceSample( fpGGA7 );
    traceSample( fpGGA8 );
  }

  for (;;);
}
