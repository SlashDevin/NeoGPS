#include <NMEAGPS.h>

//======================================================================
//  Program: NMEAsimple.ino
//
//  Description:  This program shows simple usage of NeoGPS
//
//  Prerequisites:
//     1) NMEA.ino works with your device (correct TX/RX pins and baud rate)
//     2) At least one of the RMC, GGA or GLL sentences have been enabled in NMEAGPS_cfg.h.
//     3) Your device at least one of those sentences (use NMEAorder.ino to confirm).
//     4) LAST_SENTENCE_IN_INTERVAL has been set to one of those sentences in NMEAGPS_cfg.h (use NMEAorder.ino).
//     5) LOCATION and ALTITUDE have been enabled in GPSfix_cfg.h
//
//  'Serial' is for debug output to the Serial Monitor window.
//
//======================================================================

#include <GPSport.h>

NMEAGPS  gps; // This parses the GPS characters
gps_fix  fix; // This holds on to the latest values

void setup()
{
  DEBUG_PORT.begin(9600);
  while (!Serial)
    ;
  DEBUG_PORT.print( F("NMEAsimple.INO: started\n") );

  gpsPort.begin(9600);
}

//--------------------------

void loop()
{
  while (gps.available( gpsPort )) {
    fix = gps.read();

    DEBUG_PORT.print( F("Location: ") );
    if (fix.valid.location) {
      DEBUG_PORT.print( fix.latitude(), 6 );
      DEBUG_PORT.print( ',' );
      DEBUG_PORT.print( fix.longitude(), 6 );
    }

    DEBUG_PORT.print( F(", Altitude: ") );
    if (fix.valid.altitude)
      DEBUG_PORT.print( fix.altitude() );

    DEBUG_PORT.println();
  }
}
