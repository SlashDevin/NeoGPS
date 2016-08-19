#include <Arduino.h>
#include "NMEAGPS.h"

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

//------------------------------------------------------------

static NMEAGPS  gps;  // This parses the GPS characters
static gps_fix  fix;  // This holds the latest GPS fix

//------------------------------------------------------------

using namespace NeoGPS;

static gps_fix    first;            // good GPS data
static clock_t    firstSecs;        // cached dateTime in seconds since EPOCH
static Location_t avgLoc;           // gradually-calculated average location
static uint16_t   count;            // number of samples
static int32_t    sumDLat, sumDLon; // accumulated deltas
static bool       doneAccumulating; // accumulation completed

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
      dLat = fix.location.lat() - avgLoc.lat();
      DEBUG_PORT.print( dLat );
      DEBUG_PORT.print( ',' );
      dLon = fix.location.lon() - avgLoc.lon();
      DEBUG_PORT.print( dLon );
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( Location_t::DistanceKm( fix.location, avgLoc ) * 100000.0 );
      DEBUG_PORT.println();
    }

  } else {
    if (!warned) {
      warned = true;
      DEBUG_PORT.print( F("Waiting for fix...") );
    } else
      DEBUG_PORT.print( '.' );
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
  DEBUG_PORT.println( F("count,avg lat,avg lon,dlat,dlon,distance(cm)") );

  DEBUG_PORT.flush();

  // Start the UART for the GPS device
  gps_port.begin( 9600 );
}

//------------------------------------------------------------

void loop()
{
  GPSloop();
}
