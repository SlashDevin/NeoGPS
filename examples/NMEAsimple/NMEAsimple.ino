#include <NMEAGPS.h>

NMEAGPS  gps; // This parses the GPS characters
gps_fix  fix; // This holds on to the latest values

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

//======================================================================

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;
  Serial.print( F("NMEAsimple.INO: started\n") );

  gpsPort.begin(9600);
}

//--------------------------

void loop()
{
  while (gps.available( gpsPort )) {
    fix = gps.read();

    Serial.print( F("Location: ") );
    if (fix.valid.location) {
      Serial.print( fix.latitude(), 6 );
      Serial.print( ',' );
      Serial.print( fix.longitude(), 6 );
    }

    Serial.print( F(", Altitude: ") );
    if (fix.valid.altitude)
      Serial.print( fix.altitude() );

    Serial.println();
  }
}
