#include <Arduino.h>

//  This simple example will toggle the LED once per second,
//     when a GPRMC message is received.
//  Because no actual GPS data is used, you could disable all
//    messages and all gps_fix members.  It would still recognize
//    the RMC message, without using any RAM or CPU time to parse or save
//    the (unused) values.  Essentially, this app uses the RMC message as
//    a 1PPS signal.

//  Serial is for trace output to the Serial Monitor window.

//  Note: Because this example does not use Serial (except in 'setup'), you
//    could use 'Serial' for the gps_port, like this:
//
//       HardwareSerial & gps_port = Serial;
//
//  If you don't need debug output on Serial, this would be a lot more reliable
//    than using SoftwareSerial for the GPS.
//  This is a better approach for stand-alone systems.  That is, systems that
//    are not attached to a PC should dedicate Serial to the GPS device.

//-------------------------------------------------------------------------
//  This include file will choose a default serial port for the GPS device.
#include "GPSport.h"

/*
  For Mega Boards, "GPSport.h" will choose Serial1.
    pin 18 should be connected to the GPS RX pin, and
    pin 19 should be connected to the GPS TX pin.

  For all other Boards, "GPSport.h" will choose SoftwareSerial:
    pin 3 should be connected to the GPS TX pin, and
    pin 4 should be connected to the GPS RX pin.

  If you know which serial port you want to use, delete the above
    include and  simply declare

    SomeKindOfSerial gps_port( args );
          or
    HardwareSerial & gps_port = Serialx; // an alias
          or
    Search and replace all occurrences of "gps_port" with your port's name.
*/

#include "NMEAGPS.h"

//------------------------------------------------------------

static NMEAGPS  gps         ; // This parses received characters

static const int led = 13;

//--------------------------

void setup()
{
  // Start the Serial Monitor output
  Serial.begin(9600);

  Serial.print( F("NMEAblink.INO: started\n") );
  Serial.print( F("fix object size = ") );
  Serial.println( sizeof(gps.fix()) );
  Serial.print( F("NMEAGPS object size = ") );
  Serial.println( sizeof(NMEAGPS) );
  Serial.println( F("Looking for GPS device on " USING_GPS_PORT) );
  Serial.flush();

  // Start the UART for the GPS device
  gps_port.begin(9600);

  pinMode(led, OUTPUT);
}

//--------------------------

void loop()
{
  while (gps_port.available()) {

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC) {

        // An RMC was received.
        digitalWrite( led, !digitalRead(led) ); // toggle the LED
      }
    }
  }
}
