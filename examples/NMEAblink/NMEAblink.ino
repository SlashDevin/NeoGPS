#include <Arduino.h>
#include <NMEAGPS.h>

//======================================================================
//  Program: NMEAblink.ino
//
//  Prerequisites:
//     1) NMEA.ino works with your device
//
//  Description:  This program will toggle the LED once per second,
//     when a GPRMC message is received.
//
//     Because no actual GPS data is used, you could disable all
//       messages and all gps_fix members.  It would still recognize
//       the RMC message, without using any RAM or CPU time to parse or save
//       the (unused) values.  Essentially, this app uses the RMC message as
//       a 1PPS signal.
//
//  Note: Because this example does not use 'Serial', you
//    could use 'Serial' for the gps_port, like this:
//
//       HardwareSerial & gps_port = Serial;
//
//    Of course, be sure to connect the GPS device appropriately.
//      You may need to research whether your Arduino allows 
//      using 'Serial' in this fashion.  For example, some Arduinos
//      dedicate 'Serial' to the USB interface, and it cannot be
//      "shared" with the GPS device.
//
//    If you don't need debug output on Serial, this would be a lot 
//      more reliable than using SoftwareSerial for the GPS.  Be 
//      sure to delete all 'Serial' debug statements.  If your system 
//      is not attached to a PC when installed, you should seriously 
//      consider using 'Serial' for the GPS device.
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

static NMEAGPS   gps;
static const int led = 13;

//--------------------------

void setup()
{
  // Start the UART for the GPS device
  gps_port.begin(9600);

  pinMode(led, OUTPUT);
}

//--------------------------

void loop()
{
  while (gps_port.available()) {

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC)
        digitalWrite( led, !digitalRead(led) ); // toggle the LED
    }
  }
}
