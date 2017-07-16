#include <NMEAGPS.h>

NMEAGPS  gps; // This parses the GPS characters
gps_fix  fix; // This holds on to the latest values

//-----------------
//  Prerequisites:
//     1) NMEA.ino works with your device (correct TX/RX pins and baud rate)
//     2) GPS_FIX_SATELLITES is enabled in GPSfix_cfg.h
//     3) NMEAGPS_PARSE_SATELLITES and NMEAGPS_PARSE_SATELLITE_INFO are
//              enabled in NMEAGPS_cfg.h
//     4) The GSV sentence has been enabled in NMEAGPS_cfg.h.
//     5) Your device emits the GSV sentence (use NMEAorder.ino to confirm).
//     6) LAST_SENTENCE_IN_INTERVAL has been set to GSV (or any other enabled sentence)
//              in NMEAGPS_cfg.h (use NMEAorder.ino).
//
//  'Serial' is for debug output to the Serial Monitor window.
//

//-----------------
//   Choose a serial port for the GPS device:
//
//   BEST: For a Mega, Leonardo or Due, use the extra hardware serial port
#define gpsPort Serial1

//   2nd BEST:  For other Arduinos, use AltSoftSerial on the required pins
//                 (8&9 for an UNO)
// #include <AltSoftSerial.h>
// AltSoftSerial gpsPort;  // pin 8 to GPS TX, pin 9 to GPS RX

//   3rd BEST:  If you can't use those specific pins (are you sure?),
//                 use NeoSWSerial on any two pins @ 9600, 19200 or 38400
// #include <NeoSWSerial.h>
// NeoSWSerial gpsPort( 2, 3 ); // pin 2 to GPS TX, pin 3 to GPS RX

//   WORST:  SoftwareSerial is NOT RECOMMENDED

//-----------------

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;
  Serial.print( F("NeoGPS GSV example started\n") );

  gpsPort.begin(9600);

} // setup

//-----------------

void loop()
{
  while (gps.available( gpsPort )) {
    fix = gps.read();

    displaySatellitesInView();
  }

} // loop

//-----------------

void displaySatellitesInView()
{
  Serial.print( gps.sat_count );
  Serial.print( ',' );

  for (uint8_t i=0; i < gps.sat_count; i++) {
    Serial.print( gps.satellites[i].id );
    Serial.print( ' ' );
    Serial.print( gps.satellites[i].elevation );
    Serial.print( '/' );
    Serial.print( gps.satellites[i].azimuth );
    Serial.print( '@' );
    if (gps.satellites[i].tracked)
      Serial.print( gps.satellites[i].snr );
    else
      Serial.print( '-' );
    Serial.print( F(", ") );
  }

  Serial.println();

} // displaySatellitesInView
