#ifndef USING_GPS_PORT

//-----------------------------------------------------------
// Pick the serial port to which the GPS device is connected.
//
// If you include "SoftwareSerial.h", "gSoftSerial.h",
//   "AltSoftSerial.h"  *OR*  "NeoHWSerial.h" before 
//   this file, one of those ports will be used for 
//   the GPS.  Otherwise, just declare your own gps_port 
//   in your INO and don't include this file.
//
// It also defines USING_GPS_PORT, which can be used for printing
//   the selected device:
//
//   Serial.println( "Looking for GPS device on " USING_GPS_PORT );

#if defined( SoftwareSerial_h )
  #define SS_TYPE SoftwareSerial
#elif defined( gSoftSerial_h )
  #define SS_TYPE gSoftSerial
#elif defined( AltSoftSerial_h )
  #define SS_TYPE AltSoftSerial
#endif

#ifdef SS_TYPE

  // The current Board (an Uno?) does not have an extra serial port.
  // Use SoftwareSerial to listen to the GPS device.
  //   You should expect to get some RX errors, which may
  //   prevent getting fix data every second.  YMMV.

  // Arduino RX pin number that is connected to the GPS TX pin
  #define RX_PIN 4

  // Arduino TX pin number that is connected to the GPS RX pin
  #define TX_PIN 3

  SS_TYPE gps_port( RX_PIN, TX_PIN );

  //  Here's a little preprocessor magic to get a nice string
  #define xstr(x) str(x)
  #define str(x) #x
  #define USING_GPS_PORT \
    xstr(SS_TYPE) "( RX pin " xstr(RX_PIN) \
                  ", TX pin " xstr(TX_PIN) " )"

  #if defined(UBRR1H)
    //  If you *really* want to do this, or you just happened
    //  to include SoftwareSerial.h for something else, you're
    //  better off *not* including this file.  Just declare
    //  your own gps_port in you INO file.

    #error You should be using Serial1 for the GPS device.  \
        SoftwareSerial is very inefficient and unreliable when \
        used for GPS communications!
  #endif
        
#else

  // The current Board (a Mega?) has an extra hardware serial port
  //   on pins 18 (TX1) and 19 (RX1) (probably).  You can change
  //   this to Serial2 or Serial3, if you like.

  #if defined( NeoHWSerial_h )
    // App is using replacement NeoHWSerial class
    NeoHWSerial & gps_port = NeoSerial1;
    #define USING_GPS_PORT "NeoSerial1"
  #else
    // App is using standard IDE HardwareSerial class
    HardwareSerial & gps_port = Serial1;
    #define USING_GPS_PORT "Serial1"
  #endif


#endif

#endif