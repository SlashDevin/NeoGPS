#include <Arduino.h>
#include <NMEAGPS.h>

//======================================================================
//  Program: NMEAlocDMS.ino
//
//  Description:  This program only parses an RMC sentence for the lat/lon.
//
//  Prerequisites:
//     1) NMEA.ino works with your device (correct TX/RX pins and baud rate)
//     2) The RMC sentence has been enabled.
//     3) Your device sends an RMC sentence (e.g., $GPRMC).
//
//  Serial is for trace output to the Serial Monitor window.
//
//======================================================================

#include <GPSport.h>

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_RMC )
  #error You must uncomment NMEAGPS_PARSE_RMC in NMEAGPS_cfg.h!
#endif

#if !defined( GPS_FIX_LOCATION_DMS )
  #error You must uncomment GPS_FIX_LOCATION_DMS in GPSfix_cfg.h!
#endif

#ifdef NMEAGPS_INTERRUPT_PROCESSING
  #error You must *NOT* define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
#endif

//------------------------------------------------------------

static NMEAGPS  gps; // This parses the GPS characters

static void doSomeWork( const gps_fix & fix );
static void doSomeWork( const gps_fix & fix )
{
  //  This is the best place to do your time-consuming work, right after
  //     the RMC sentence was received.  If you do anything in "loop()",
  //     you could cause GPS characters to be lost, and you will not
  //     get a good lat/lon.
  //  For this example, we just print the lat/lon.  If you print too much,
  //     this routine will not get back to "loop()" in time to process
  //     the next set of GPS data.

  if (fix.valid.location) {

    DEBUG_PORT << fix.latitudeDMS;
    DEBUG_PORT.print( fix.latitudeDMS.NS() );
    DEBUG_PORT.write( ' ' );
    if (fix.longitudeDMS.degrees < 100)
      DEBUG_PORT.write( '0' );
    DEBUG_PORT << fix.longitudeDMS;
    DEBUG_PORT.print( fix.longitudeDMS.EW() );

  } else {
    // No valid location data yet!
    DEBUG_PORT.print( '?' );
  }

  DEBUG_PORT.println();

} // doSomeWork

//------------------------------------

static void GPSloop();
static void GPSloop()
{  
  while (gps.available( gpsPort ))
    doSomeWork( gps.read() );

} // GPSloop
  
//--------------------------

void setup()
{
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("NMEAlocDMS.INO: started\n") );
  DEBUG_PORT.println( F("Looking for GPS device on " GPS_PORT_NAME) );
  DEBUG_PORT.flush();

  gpsPort.begin(9600);
}

//--------------------------

void loop()
{
  GPSloop();
}
