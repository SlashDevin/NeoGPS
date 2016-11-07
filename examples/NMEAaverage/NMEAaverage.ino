#include <Arduino.h>
#include <NMEAGPS.h>
using namespace NeoGPS;

//======================================================================
//  Program: NMEAaverage.ino
//
//  Description:  This program averages locations over time to compute
//                a higher-accuracy *static* location.  It also shows
//                how to use the distance functions in the Location class.
//
//  Prerequisites:
//     1) NMEA.ino works with your device (correct TX/RX pins and baud rate)
//     2) At least once sentence with a location field has been enabled
//
//  'Serial' is for debug output to the Serial Monitor window.
//
//======================================================================

#if defined( UBRR1H ) | defined( ID_USART0 )
  // Default is to use Serial1 when available.  You could also
  // use NeoHWSerial, especially if you want to handle GPS characters
  // in an Interrupt Service Routine.
  //#include <NeoHWSerial.h>
#else
  // Only one serial port is available, uncomment one of the following:
  //#include <NeoICSerial.h>
  //#include <AltSoftSerial.h>
  #include <NeoSWSerial.h>
  //#include <SoftwareSerial.h> /* NOT RECOMMENDED */
#endif

#include "GPSport.h"

#ifdef NeoHWSerial_h
  #define DEBUG_PORT NeoSerial
#else
  #define DEBUG_PORT Serial
#endif

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_GGA ) && \
    !defined( NMEAGPS_PARSE_GLL ) && \
    !defined( NMEAGPS_PARSE_RMC )
  #error You must uncomment at least one of NMEAGPS_PARSE_GGA, GGL or RMC in NMEAGPS_cfg.h!
#endif

#if !defined( GPS_FIX_LOCATION )
  #error You must uncomment GPS_FIX_LOCATION in GPSfix_cfg.h!
#endif

#if !defined( GPS_FIX_TIME )
  #error You must uncomment GPS_FIX_TIME in GPSfix_cfg.h!
#endif

#if !defined( GPS_FIX_DATE )
  #error You must uncomment GPS_FIX_DATE in GPSfix_cfg.h!
#endif

//------------------------------------------------------------

static NMEAGPS  gps;  // This parses the GPS characters
static gps_fix  fix;  // This holds the latest GPS fix

//------------------------------------------------------------

static gps_fix    first;            // good GPS data
static clock_t    firstSecs;        // cached dateTime in seconds since EPOCH
static Location_t avgLoc;           // gradually-calculated average location
static uint16_t   count;            // number of samples
static int32_t    sumDLat, sumDLon; // accumulated deltas
static bool       doneAccumulating; // accumulation completed

//------------------------------------------------------------

const char nCD  [] PROGMEM = "N";
const char nneCD[] PROGMEM = "NNE";
const char neCD [] PROGMEM = "NE";
const char eneCD[] PROGMEM = "ENE";
const char eCD  [] PROGMEM = "E";
const char eseCD[] PROGMEM = "ESE";
const char seCD [] PROGMEM = "SE";
const char sseCD[] PROGMEM = "SSE";
const char sCD  [] PROGMEM = "S";
const char sswCD[] PROGMEM = "SSW";
const char swCD [] PROGMEM = "SW";
const char wswCD[] PROGMEM = "WSW";
const char wCD  [] PROGMEM = "W";
const char wnwCD[] PROGMEM = "WNW";
const char nwCD [] PROGMEM = "NW";
const char nnwCD[] PROGMEM = "NNW";

const char * const dirStrings[] =
  { nCD, nneCD, neCD, eneCD, eCD, eseCD, seCD, sseCD, 
    sCD, sswCD, swCD, wswCD, wCD, wnwCD, nwCD, nnwCD };

const __FlashStringHelper *compassDir( uint16_t bearing ) // degrees CW from N
{
  const int16_t directions    = sizeof(dirStrings)/sizeof(dirStrings[0]);
  const int16_t degreesPerDir = 360 / directions;
        int8_t  dir           = (bearing + degreesPerDir/2) / degreesPerDir;

  while (dir < 0)
    dir += directions;
  while (dir >= directions)
    dir -= directions;

  return (const __FlashStringHelper *) dirStrings[ dir ];

} // compassDir

//------------------------------------------------------------

static void doSomeWork()
{
  static bool warned = false; // that we're waiting for a valid location

  if (fix.valid.location && fix.valid.date && fix.valid.time) {

    if (count == 0) {
    
      // Just save the first good fix
      first = fix;
      firstSecs = (clock_t) first.dateTime;
      count = 1;

    } else {

      // After the first fix, accumulate locations until we have
      //   a good average.  Then display the offset from the average.

      if (warned) {
        // We were waiting for the fix to be re-acquired.
        warned = false;
        DEBUG_PORT.println();
      }

      DEBUG_PORT.print( count );

      if (!doneAccumulating) {

        // Enough time?
        if (((clock_t)fix.dateTime - firstSecs) > 2 * SECONDS_PER_HOUR)
          doneAccumulating = true;
      }

      int32_t dLat, dLon;

      if (!doneAccumulating) {

        // Use deltas from the first location
        dLat = fix.location.lat() - first.location.lat();
        sumDLat += dLat;
        int32_t avgDLat = sumDLat / count;

        dLon = fix.location.lon() - first.location.lon();
        sumDLon += dLon;
        int32_t avgDLon = sumDLon / count;

        //  Then calculated the average location as the first location
        //     plus the averaged deltas.
        avgLoc.lat( first.location.lat() + avgDLat );
        avgLoc.lon( first.location.lon() + avgDLon );

        count++;
      }

      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( avgLoc.lat() );
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( avgLoc.lon() );
      DEBUG_PORT.print( ',' );
      dLat = avgLoc.lat() - fix.location.lat();
      DEBUG_PORT.print( dLat );
      DEBUG_PORT.print( ',' );
      dLon = avgLoc.lon() - fix.location.lon();
      DEBUG_PORT.print( dLon );

      // Calculate the distance from the current fix to the average location
      float avgDistError = avgLoc.DistanceKm( fix.location );
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( avgDistError * 100000.0 ); // cm

      // Calculate the bearing from the current fix to the average location.
      //   NOTE: other libraries will have trouble with this calculation,
      //   because these coordinates are *VERY* close together.  Naive
      //   floating-point calculations will not have enough significant
      //   digits.
      float avgBearingErr = fix.location.BearingTo( avgLoc );
      float bearing       = avgBearingErr * Location_t::DEG_PER_RAD;
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( bearing, 6 );
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( compassDir( bearing ) );

      // Calculate a point that is 10km away from the average location,
      //   at the error bearing
      Location_t tenKmAway( avgLoc );
      tenKmAway.OffsetBy( 10.0 / Location_t::EARTH_RADIUS_KM, avgBearingErr );
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( tenKmAway.lat() );
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( tenKmAway.lon() );

      // Calculate the bearing from the average location to that point.
      //    This should be very close to the avgBearingErr, and will
      //    reflect the calculation error.  This is because the
      //    current fix is *VERY* close to the average location.
      float tb = avgLoc.BearingToDegrees( tenKmAway );
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( tb, 6 );

      DEBUG_PORT.println();
    }

  } else {
    if (!warned) {
      warned = true;
      DEBUG_PORT.print( F("Waiting for fix...") );
    } else {
      DEBUG_PORT.print( '.' );
    }
  }

} // doSomeWork

//------------------------------------------------------------

static void GPSloop()
{
  while (gps.available( gps_port )) {
    fix = gps.read();
    doSomeWork();
  }

} // GPSloop

//------------------------------------------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("NMEAaverage.INO: started\n") );
  DEBUG_PORT.print( F("  fix object size = ") );
  DEBUG_PORT.println( sizeof(gps.fix()) );
  DEBUG_PORT.print( F("  gps object size = ") );
  DEBUG_PORT.println( sizeof(gps) );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );
  DEBUG_PORT.println( F("Comparing current fix with averaged location.\n"
                        "count,avg lat,avg lon,dlat,dlon,distance(cm),"
                        "bearing deg,compass,lat/lon 10km away & recalc bearing") );
  DEBUG_PORT.flush();

  // Start the UART for the GPS device
  gps_port.begin( 9600 );
}

//------------------------------------------------------------

void loop()
{
  GPSloop();
}
