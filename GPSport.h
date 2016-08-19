#ifndef GPSport_h
#define GPSport_h

//-----------------------------------------------------------
// Pick the serial port to which the GPS device is connected.
//
// This file tries to guess which port a beginner should use.
//   If you already know which port and which library you want
//   to use for that port, DON'T INCLUDE THIS FILE.
//   Just declare it yourself, and remove this include from your INO!
//
// If you include <SoftwareSerial.h>, <gSoftSerial.h>,
//   <AltSoftSerial.h>, <NeoICSerial>  *OR*  <NeoHWSerial.h> before 
//   this file, one of those ports will be used for 
//   the GPS.
//
// It also defines USING_GPS_PORT, which can be used for printing
//   the selected device:
//
//   Serial.println( "Looking for GPS device on " USING_GPS_PORT );

#if defined( SoftwareSerial_h )
  #define SS_TYPE SoftwareSerial

#elif defined( NeoSWSerial_h )
  #define SS_TYPE NeoSWSerial

#elif defined( AltSoftSerial_h )
  AltSoftSerial gps_port;
  #define USING_GPS_PORT "AltSoftSerial"

#elif defined( NeoICSerial_h )
  NeoICSerial gps_port;
  #define USING_GPS_PORT "NeoICSerial"

#elif defined( NeoHWSerial_h )
  NeoHWSerial & gps_port = NeoSerial1;
  #define USING_GPS_PORT "NeoSerial1"

#elif defined( UBRR1H ) | defined( ID_USART0 )
  HardwareSerial & gps_port = Serial1;
  #define USING_GPS_PORT "Serial1"

#else
  #error Unable to choose serial port for GPS device.  \
  You must include a serial header before "#include GPSport.h" in the INO!  \
  NeoSWSerial, NeoICSerial, NeoHWSerial, AltSoftSerial and SoftwareSerial are supported.
#endif

#ifdef SS_TYPE

  //---------------------------------------------------------------
  // The current Board (an Uno?) does not have an extra serial port.
  // Use a software serial library to listen to the GPS device.
  //   You should expect to get some RX errors, which may
  //   prevent getting fix data every second.  YMMV.

  // Arduino RX pin number that is connected to the GPS TX pin
  #ifndef RX_PIN
    #define RX_PIN 4
  #endif

  // Arduino TX pin number that is connected to the GPS RX pin
  #ifndef TX_PIN
    #define TX_PIN 3
  #endif

  SS_TYPE gps_port( RX_PIN, TX_PIN );

  //  A little preprocessor magic to get a nice string
  #define xstr(x) str(x)
  #define str(x) #x
  #define USING_GPS_PORT \
    xstr(SS_TYPE) "( RX pin " xstr(RX_PIN) \
                  ", TX pin " xstr(TX_PIN) " )"

  #if defined(UBRR1H)
    //  If you *really* want to do this, or you just happened
    //  to include SoftwareSerial.h for something else, you're
    //  better off *not* including this file.  Just declare
    //  your own gps_port in your INO file.

    #error You should be using Serial1 for the GPS device.  \
        Software serial libraries are very inefficient and unreliable when \
        used for GPS communications!
  #endif

#endif

#endif