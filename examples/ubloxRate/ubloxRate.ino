#include <NMEAGPS.h>
#include <NeoTeeStream.h>

//======================================================================
//  Program: NMEA.ino
//
//  Description:  This program sends ublox commands to enable and disable
//    NMEA sentences, set the update rate to 1Hz, 5Hz or 10Hz, and set the 
//    baud rate to 9600 or 115200.
//
//    Enter the following commands through the Serial Monitor window:
//
//      '1'  - send NMEA PUBX text command to enable all sentences
//      '0'  - send NMEA PUBX text command to disable all sentences except GLL
//      'd'  - send UBX binary command to disable all sentences except GLL
//      'r1' - send UBX binary command to set update rate to  1Hz
//      'r5' - send UBX binary command to set update rate to  5Hz
//      'r0' - send UBX binary command to set update rate to 10Hz
//      'r6' - send UBX binary command to set update rate to 16Hz
//      '3'  - send NMEA PUBX text command to set baud rate to 38400
//      '9'  - send NMEA PUBX text command to set baud rate to 9600
//      'e'  - toggle echo of all characters received from GPS device.
//
//    CAUTION:   If your Serial Monitor window baud rate is less than the GPS baud
//       rate, turning echo ON will cause the sketch to lose some or all 
//       GPS data and/or fixes.
//
//    NOTE:  All NMEA PUBX text commands are also echoed to the debug port.
//
//  Prerequisites:
//     1) Your GPS device has been correctly powered.
//          Be careful when connecting 3.3V devices.
//     2) Your GPS device is correctly connected to an Arduino serial port.
//          See GPSport.h for the default connections.
//     3) You know the default baud rate of your GPS device.
//          If 9600 does not work, use NMEAdiagnostic.ino to
//          scan for the correct baud rate.
//     4) LAST_SENTENCE_IN_INTERVAL is defined to be the following in NMEAGPS_cfg.h:
//
//          #include <stdint.h>
//          extern uint8_t LastSentenceInInterval; // a variable!
//          #define LAST_SENTENCE_IN_INTERVAL \
//                      ((NMEAGPS::nmea_msg_t) LastSentenceInInterval)
//
//        This is a replacement for the typical
//
//          #define LAST_SENTENCE_IN_INTERVAL NMEAGPS::NMEA_GLL
//
//        This allows the sketch to choose the last sentence *at run time*, not
//        compile time.  This is necessary because this sketch can send 
//        configuration commands that change which sentences are enabled at run
//        time.  The storage for the "externed" variable is below.
//
//  'Serial' is for debug output to the Serial Monitor window.
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

#ifdef NeoHWSerial_h
  #define DEBUG_PORT NeoSerial
#else
  #define DEBUG_PORT Serial
#endif

#include "Streamers.h"

static NMEAGPS  gps;
static gps_fix  fix_data;
uint8_t LastSentenceInInterval = 0xFF; // storage for the run-time selection

static char lastChar; // last command char
static bool echoing = false;

//  Use NeoTee to echo the NMEA text commands to the Serial Monitor window
Stream *both[2] = { &Serial, &gps_port };
NeoTeeStream tee( both, sizeof(both)/sizeof(both[0]) );

//-------------------------------------------
// U-blox UBX binary commands

const unsigned char ubxRate1Hz[] PROGMEM = 
  { 0x06,0x08,0x06,0x00,0xE8,0x03,0x01,0x00,0x01,0x00 };
const unsigned char ubxRate5Hz[] PROGMEM =
  { 0x06,0x08,0x06,0x00,200,0x00,0x01,0x00,0x01,0x00 };
const unsigned char ubxRate10Hz[] PROGMEM =
  { 0x06,0x08,0x06,0x00,100,0x00,0x01,0x00,0x01,0x00 };
const unsigned char ubxRate16Hz[] PROGMEM =
  { 0x06,0x08,0x06,0x00,50,0x00,0x01,0x00,0x01,0x00 };

// Disable specific NMEA sentences
const unsigned char ubxDisableGGA[] PROGMEM =
  { 0x06,0x01,0x08,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x01 };
const unsigned char ubxDisableGLL[] PROGMEM =
  { 0x06,0x01,0x08,0x00,0xF0,0x01,0x00,0x00,0x00,0x00,0x00,0x01 };
const unsigned char ubxDisableGSA[] PROGMEM =
  { 0x06,0x01,0x08,0x00,0xF0,0x02,0x00,0x00,0x00,0x00,0x00,0x01 };
const unsigned char ubxDisableGSV[] PROGMEM =
  { 0x06,0x01,0x08,0x00,0xF0,0x03,0x00,0x00,0x00,0x00,0x00,0x01 };
const unsigned char ubxDisableRMC[] PROGMEM =
  { 0x06,0x01,0x08,0x00,0xF0,0x04,0x00,0x00,0x00,0x00,0x00,0x01 };
const unsigned char ubxDisableVTG[] PROGMEM =
  { 0x06,0x01,0x08,0x00,0xF0,0x05,0x00,0x00,0x00,0x00,0x00,0x01 };
const unsigned char ubxDisableZDA[] PROGMEM =
  { 0x06,0x01,0x08,0x00,0xF0,0x08,0x00,0x00,0x00,0x00,0x00,0x01 };

//--------------------------

void sendUBX( const unsigned char *progmemBytes, size_t len )
{
  gps_port.write( 0xB5 ); // SYNC1
  gps_port.write( 0x62 ); // SYNC2

  uint8_t a = 0, b = 0;
  while (len-- > 0) {
    uint8_t c = pgm_read_byte( progmemBytes++ );
    a += c;
    b += a;
    gps_port.write( c );
  }

  gps_port.write( a ); // CHECKSUM A
  gps_port.write( b ); // CHECKSUM B

} // sendUBX

//-------------------------------------------
// U-blox NMEA text commands

const char disableRMC[] PROGMEM = "PUBX,40,RMC,0,0,0,0,0,0";
const char disableGLL[] PROGMEM = "PUBX,40,GLL,0,0,0,0,0,0";
const char disableGSV[] PROGMEM = "PUBX,40,GSV,0,0,0,0,0,0";
const char disableGSA[] PROGMEM = "PUBX,40,GSA,0,0,0,0,0,0";
const char disableGGA[] PROGMEM = "PUBX,40,GGA,0,0,0,0,0,0";
const char disableVTG[] PROGMEM = "PUBX,40,VTG,0,0,0,0,0,0";
const char disableZDA[] PROGMEM = "PUBX,40,ZDA,0,0,0,0,0,0";

const char enableRMC[] PROGMEM = "PUBX,40,RMC,0,1,0,0,0,0";
const char enableGLL[] PROGMEM = "PUBX,40,GLL,0,1,0,0,0,0";
const char enableGSV[] PROGMEM = "PUBX,40,GSV,0,1,0,0,0,0";
const char enableGSA[] PROGMEM = "PUBX,40,GSA,0,1,0,0,0,0";
const char enableGGA[] PROGMEM = "PUBX,40,GGA,0,1,0,0,0,0";
const char enableVTG[] PROGMEM = "PUBX,40,VTG,0,1,0,0,0,0";
const char enableZDA[] PROGMEM = "PUBX,40,ZDA,0,1,0,0,0,0";

const char baud9600 [] PROGMEM = "PUBX,41,1,3,3,9600,0";
const char baud38400[] PROGMEM = "PUBX,41,1,3,3,38400,0";

//--------------------------

void changeBaud( const char *textCommand, unsigned long baud )
{
  gps.send_P( &tee, (const __FlashStringHelper *) disableRMC );
  gps.send_P( &tee, (const __FlashStringHelper *) disableGLL );
  gps.send_P( &tee, (const __FlashStringHelper *) disableGSV );
  gps.send_P( &tee, (const __FlashStringHelper *) disableGSA );
  gps.send_P( &tee, (const __FlashStringHelper *) disableGGA );
  gps.send_P( &tee, (const __FlashStringHelper *) disableVTG );
  gps.send_P( &tee, (const __FlashStringHelper *) disableZDA );
  gps.send_P( &tee, (const __FlashStringHelper *) textCommand );

  Serial.println( F("All sentences disabled for baud rate change.  Enter '1' to reenable sentences.") );
  delay( 1000 );
  gps_port.end();
  gps_port.begin( baud );

} // changeBaud

//------------------------------------

static void doSomeWork()
{
  // Print all the things!

  trace_all( DEBUG_PORT, gps, fix_data );

} // doSomeWork

//--------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("ubloxRate.INO: started\n") );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );

  #ifdef NMEAGPS_INTERRUPT_PROCESSING
    #error You must *NOT* define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
  #endif

  #if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
      !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
      !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
      !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST )

    DEBUG_PORT.println( F("\nWARNING: No NMEA sentences are enabled: no fix data will be displayed.") );

  #else
    if (gps.merging == NMEAGPS::NO_MERGING) {
      DEBUG_PORT.print  ( F("\nWARNING: displaying data from ") );
      DEBUG_PORT.print  ( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
      DEBUG_PORT.print  ( F(" sentences ONLY, and only if ") );
      DEBUG_PORT.print  ( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
      DEBUG_PORT.println( F(" is enabled.\n"
                            "  Other sentences may be parsed, but their data will not be displayed.") );
    }
  #endif

  if (LastSentenceInInterval != LAST_SENTENCE_IN_INTERVAL) {
    Serial.println(
      F("LAST_SENTENCE_IN_INTERVAL is not properly defined in NMEAGPS_cfg.h!\n"
        "   See Prerequisite 4 above") );
    for (;;); // hang here!
  }
  LastSentenceInInterval = NMEAGPS::NMEA_GLL;

  trace_header( DEBUG_PORT );
  DEBUG_PORT.flush();

  // Start the UART for the GPS device
  gps_port.begin( 9600 );
}

void loop()
{
  // Check for commands
  
  if (Serial.available()) {
    char c = Serial.read();

    switch (c) {
      case '0':
        if (lastChar == 'r') {
          sendUBX( ubxRate10Hz, sizeof(ubxRate10Hz) );
        } else {
          gps.send_P( &tee, (const __FlashStringHelper *) disableRMC );
          gps.send_P( &tee, (const __FlashStringHelper *) enableGLL );
          gps.send_P( &tee, (const __FlashStringHelper *) disableGSV );
          gps.send_P( &tee, (const __FlashStringHelper *) disableGSA );
          gps.send_P( &tee, (const __FlashStringHelper *) disableGGA );
          gps.send_P( &tee, (const __FlashStringHelper *) disableVTG );
          gps.send_P( &tee, (const __FlashStringHelper *) disableZDA );
          LastSentenceInInterval = NMEAGPS::NMEA_GLL;
        }
        break;
      case '1':
        if (lastChar == 'r') {
          sendUBX( ubxRate1Hz, sizeof(ubxRate1Hz) );
        } else {
          gps.send_P( &tee, (const __FlashStringHelper *) enableRMC );
          gps.send_P( &tee, (const __FlashStringHelper *) enableGLL );
          gps.send_P( &tee, (const __FlashStringHelper *) enableGSV );
          gps.send_P( &tee, (const __FlashStringHelper *) enableGSA );
          gps.send_P( &tee, (const __FlashStringHelper *) enableGGA );
          gps.send_P( &tee, (const __FlashStringHelper *) enableVTG );
          gps.send_P( &tee, (const __FlashStringHelper *) enableZDA );
          LastSentenceInInterval = NMEAGPS::NMEA_ZDA;
        }
        break;
      case '3':
        changeBaud( baud38400, 38400 );
        break;
      case '5':
        if (lastChar == 'r') {
          sendUBX( ubxRate5Hz, sizeof(ubxRate5Hz) );
        }
        break;
      case '6':
        if (lastChar == 'r') {
          sendUBX( ubxRate16Hz, sizeof(ubxRate16Hz) );
        }
        break;
      case '9':
        changeBaud( baud9600, 9600 );
        break;

      case 'd':
        sendUBX( ubxDisableRMC, sizeof(ubxDisableRMC) );
        //sendUBX( ubxDisableGLL, sizeof(ubxDisableGLL) );
        sendUBX( ubxDisableGSV, sizeof(ubxDisableGSV) );
        sendUBX( ubxDisableGSA, sizeof(ubxDisableGSA) );
        sendUBX( ubxDisableGGA, sizeof(ubxDisableGGA) );
        sendUBX( ubxDisableVTG, sizeof(ubxDisableVTG) );
        sendUBX( ubxDisableZDA, sizeof(ubxDisableZDA) );
        LastSentenceInInterval = NMEAGPS::NMEA_GLL;
        break;

      case 'e':
        echoing = !echoing;
        break;

      default: break;
    }
    lastChar = c;
  }

  //  Check for GPS data

  if (echoing) {
    // Use advanced character-oriented methods to echo received characters to
    //    the Serial Monitor window.
    if (gps_port.available()) {
      char c = gps_port.read();
      Serial.write( c );
      gps.handle( c );
      if (gps.available()) {
        fix_data = gps.read();
        doSomeWork();
      }
    }

  } else {
    // Use the normal fix-oriented methods to display fixes
    if (gps.available( gps_port )) {
      fix_data = gps.read();
      doSomeWork();
    }
  }
}
