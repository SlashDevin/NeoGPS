#ifndef USING_GPS_PORT

//-----------------------------------------------------------
// Pick the serial port to which the GPS device is connected.
//
// It also defines USING_GPS_PORT, which can be used for printing
//   the selected device:
//
//   Serial.println( "Looking for GPS device on " USING_GPS_PORT );

#if defined(UBRR1H)

  // The current Board (a Mega?) has an extra hardware serial port
  //   on pins 18 (TX1) and 19 (RX1) (probably).
  HardwareSerial & gps_port = Serial1;

  #define USING_GPS_PORT "Serial1"

#else

  // The current Board (an Uno?) does not have an extra serial port.
  // Use SoftwareSerial to listen to the GPS device.
  //   You should expect to get some RX errors, which may
  //   prevent getting fix data every second.  YMMV.
  #include "SoftwareSerial.h"

  // Arduino RX pin number that is connected to the GPS TX pin
  #define RX_PIN 3

  // Arduino TX pin number that is connected to the GPS RX pin
  #define TX_PIN 4

  SoftwareSerial gps_port( RX_PIN, TX_PIN );

  //  Here's a little preprocessor magic to get a nice string
  #define f(x) #x
  #define STRINGIZE(f,x) f(x)
  #define USING_GPS_PORT \
    "SoftwareSerial( RX pin " STRINGIZE(f,RX_PIN) \
                  ", TX pin " STRINGIZE(f,TX_PIN) " )"

#endif

#endif