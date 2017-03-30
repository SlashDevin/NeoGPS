#include <NMEAGPS.h>

NMEAGPS  gps; // This parses the GPS characters

//======================================================================
//  Program: NMEAsimple.ino
//
//  Description:  This program shows simple usage of NeoGPS
//
//  Prerequisites:
//     1) NMEA.ino works with your device (correct TX/RX pins and baud rate)
//     2) The RMC and GGA sentences have been enabled in NMEAGPS_cfg.h.
//     3) Your device sends RMC and GGA sentences (use NMEAorder.ino to confirm).
//     4) LAST_SENTENCE_IN_INTERVAL has been selected in NMEAGPS_cfg.h (use NMEAorder.ino).
//     5) LOCATION and ALTITUDE have been enabled in GPSfix_cfg.h
//
//  'Serial' is for debug output to the Serial Monitor window.
//
//======================================================================

// PICK A SERIAL PORT:
//   BEST: For a Mega, Leonardo or Due, use the extra hardware serial port
#define gpsPort Serial1

//   2nd BEST:  For other devices, use AltSoftSerial on the required pins 
//                 (8&9 for an UNO)
// #include <AltSoftSerial.h>
// AltSoftSerial gpsPort;

//   3rd BEST:  If you can't use those specific pins (are you sure?), 
//                 use NeoSWSerial on any two pins @ 9600, 19200 or 38400
// #include <NeoSWSerial.h>
// NeoSWSerial gpsPort( 2, 3 ); // Arduino RX pin (to GPS TX pin), Arduino TX pin (to GPS RX pin)

//   WORST:  SoftwareSerial is NOT RECOMMENDED

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_RMC )
  #error You must uncomment NMEAGPS_PARSE_RMC in NMEAGPS_cfg.h!
#endif

#if !defined( NMEAGPS_PARSE_GGA )
  #error You must uncomment NMEAGPS_PARSE_GGA in NMEAGPS_cfg.h!
#endif

#if !defined( GPS_FIX_LOCATION )
  #error You must uncomment GPS_FIX_LOCATION in GPSfix_cfg.h!
#endif

#if !defined( GPS_FIX_ALTITUDE )
  #error You must uncomment GPS_FIX_ALTITUDE in GPSfix_cfg.h!
#endif

//--------------------------

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
    gps_fix fix = gps.read();

    Serial.print( F("Location: ") );
    if (fix.valid.location)
      Serial.print( fix.latitude(), 6 ); // floating-point display
    Serial.print( ',' );
    if (fix.valid.location)
      Serial.print( fix.longitude(), 6 ); // floating-point display

    Serial.print( F(", Altitude: ") );
    if (fix.valid.altitude)
      Serial.print( fix.altitude() );

    Serial.println();
  }
}
