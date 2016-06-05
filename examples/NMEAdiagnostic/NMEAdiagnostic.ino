#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAdiagnostic.ino
//
//  Description:  This program tries different baud rates until
//    valid NMEA sentences are detected.  Some GPS devices may
//    have a binary mode that does not emit NMEA sentences.  You
//    may have to send a special command or use a utility program
//    to configure it to emit NMEA sentences instead of binary messages.
//
//  Prerequisites:
//     1) Your GPS device has been correctly powered.
//          Be careful when connecting 3.3V devices.
//     2) Your GPS device is correctly connected to an Arduino serial port.
//          See GPSport.h for the default connections.
//
//  'Serial' is for debug output to the Serial Monitor window.
//
//======================================================================

#if defined( UBRR1H )
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

#include "Streamers.h"
#ifdef NeoHWSerial_h
  #define DEBUG_PORT NeoSerial
#else
  #define DEBUG_PORT Serial
#endif

// Check configuration

#ifndef NMEAGPS_RECOGNIZE_ALL
  #error You must define NMEAGPS_RECOGNIZE_ALL in NMEAGPS_cfg.h!
#endif

#ifdef NMEAGPS_IMPLICIT_MERGING
  #error You must *undefine* NMEAGPS_IMPLICIT_MERGING in NMEAGPS_cfg.h! \
   Please use EXPLICIT or NO_MERGING.
#endif

#ifdef NMEAGPS_INTERRUPT_PROCESSING
  #error You must *NOT* define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
#endif

static NMEAGPS  gps          ; // This parses received characters
static gps_fix  all_data     ; // A composite of all GPS data fields
static uint32_t last_rx = 0UL; // The last millis() time a character was
                               // received from GPS.
static uint32_t baudStartTime = 0UL;
static uint8_t  warnings = 0;
static uint8_t  errors   = 0;

//--------------------------

static void hang()
{
  DEBUG_PORT.println( F("\n** NMEAdiagnostic completed **\n") );

  if (warnings) {
    DEBUG_PORT.print( warnings );
    DEBUG_PORT.print( F(" warnings") );
  }
  if (warnings && errors)
    DEBUG_PORT.print( F(" and ") );
  if (errors) {
    DEBUG_PORT.print( errors );
    DEBUG_PORT.print( F(" errors") );
  }
  if (warnings || errors)
    DEBUG_PORT.println();

  DEBUG_PORT.flush();

  exit(0);

} // hang

//--------------------------
// Baud rates to check

static long  baud_table[] =
  { 1200, 2400, 4800, 9600, 14400, 19200, 28800, 31250, 38400, 
    57600, 115200 };
static const uint8_t num_bauds          = sizeof(baud_table)/sizeof(baud_table[0]);
static const uint8_t INITIAL_BAUD_INDEX = 3; // 9600
static       uint8_t baud_index         = INITIAL_BAUD_INDEX;
static       bool    triedDifferentBaud = false;

//--------------------------

static void tryBaud()
{
  long baud = baud_table[baud_index];
  DEBUG_PORT.print( F("\n____________________________\n\nChecking ") );
  DEBUG_PORT.print( baud );
  DEBUG_PORT.print( F(" baud...\n") );
  DEBUG_PORT.flush();

  gps_port.begin( baud );
  baudStartTime = millis();

} // tryBaud

//--------------------------

static void tryAnotherBaudRate()
{
  gps_port.end();
  while (gps_port.available())
    gps_port.read();

  if (baud_index == INITIAL_BAUD_INDEX) {
    baud_index = 0;

  } else {
    baud_index++;
    if (baud_index == INITIAL_BAUD_INDEX)
      baud_index++; // skip it, we already tried it

    if (baud_index >= num_bauds) {
      baud_index = INITIAL_BAUD_INDEX;
      DEBUG_PORT.print( F("\n  All baud rates tried!\n") );
      hang();
    }
  }

  tryBaud();

  triedDifferentBaud = true;

} // tryAnotherBaudRate

//------------------------------------

static const uint16_t MAX_SAMPLE = 256;
static uint8_t        someChars[ MAX_SAMPLE ];
static uint16_t       someCharsIndex = 0;

static void dumpSomeChars()
{
  if (someCharsIndex > 0) {
    DEBUG_PORT.print( F("Received data:\n") );

    const uint16_t  bytes_per_line = 32;
          char      ascii[ bytes_per_line ];
          uint8_t  *ptr            = &someChars[0];

    for (uint16_t i=0; i<someCharsIndex; ) {
      uint16_t j;
      
      for (j=0; (i<someCharsIndex) && (j<bytes_per_line); i++, j++) {
        uint8_t c = *ptr++;
        if (c < 0x10)
          DEBUG_PORT.print('0');
        DEBUG_PORT.print( c, HEX );
        if ((' ' <= c) && (c <= '~'))
          ascii[ j ] = c;
        else
          ascii[ j ] = '.';
      }

      uint16_t jmax = j;
      while (j++ < bytes_per_line)
        DEBUG_PORT.print( F("  ") );
      DEBUG_PORT.print( ' ' );
      
      for (j=0; j<jmax; j++)
        DEBUG_PORT.print( ascii[ j ] );
      DEBUG_PORT.print( '\n' );
    }

    someCharsIndex = 0;
  }
} // dumpSomeChars

//----------------------------------------------------------------
//  Listen to see if the GPS device is correctly 
//  connected and functioning.

static void listenForSomething()
{
  uint32_t current_ms       = millis();
  uint32_t ms_since_last_rx = current_ms - last_rx;

  if (ms_since_last_rx > 5) {

    // The GPS device has not sent any characters for at least 5ms.
    //   See if we've been getting chars sometime during the last second.
    //   If not, the GPS may not be working or connected properly.

           bool     getting_chars   =  (ms_since_last_rx < 1000UL);
    static uint32_t last_quiet_time = 0UL;
           bool     just_went_quiet =
                            (((int32_t) (last_rx - last_quiet_time)) > 0L);
           bool     next_quiet_time =
                               ((current_ms - last_quiet_time) >= 1000UL);

    if ((getting_chars && just_went_quiet)
          ||
        (!getting_chars && next_quiet_time)) {

      last_quiet_time = current_ms;  // Remember for next loop

      // Try to diagnose the problem
      
      static uint8_t tries = 1;
      if (!getting_chars) {

        if (tries++ >= 3) {
          errors++;
          DEBUG_PORT.println( F("\nCheck GPS device and/or connections.  No data received.\n") );
          hang();
        }

      } else {
        tries = 1;
        
        DEBUG_PORT.println( F("No valid sentences, but characters are being received.\n"
        "Check baud rate or device protocol configuration.\n" ) );

        dumpSomeChars();
        delay( 2000 );

        tryAnotherBaudRate();
      }
    }
  }
  
} // listenForSomething

//------------------------------------

static void GPSloop()
{
  static bool valid_sentence_received = false;

  while (gps_port.available()) {
    last_rx = millis();

    uint8_t c = gps_port.read();

    if (someCharsIndex < MAX_SAMPLE)
      someChars[ someCharsIndex++ ] = c;

    if (gps.decode( c ) == NMEAGPS::DECODE_COMPLETED) {
      all_data |= gps.fix();
      valid_sentence_received = true;

      static bool last_sentence_received = false;
      if (gps.nmeaMessage == LAST_SENTENCE_IN_INTERVAL)
        last_sentence_received = true;

      DEBUG_PORT.print( F("Received ") );
      DEBUG_PORT.println( gps.string_for( gps.nmeaMessage ) );

      static uint8_t sentences_printed = 0;
             bool    long_enough       = (millis() - baudStartTime > 3000);

      if ((sentences_printed++ >= 20) || long_enough) {

        if ((someCharsIndex >= MAX_SAMPLE) || long_enough) {

          // We received one or more sentences, display the baud rate
          DEBUG_PORT.print( F("\n\n**** NMEA sentence(s) detected!  ****\n") );
          dumpSomeChars();
          DEBUG_PORT << F("\nDevice baud rate is ") <<
            baud_table[ baud_index ] << '\n';

          DEBUG_PORT.print( F("\nGPS data fields received:\n\n  ") );
          trace_header( DEBUG_PORT );
          DEBUG_PORT.print( F("  ") );
          trace_all( DEBUG_PORT, gps, all_data );

          if (!last_sentence_received) {
            warnings++;
            DEBUG_PORT.print( F("\nWarning: LAST_SENTENCE_IN_INTERVAL defined to be ") );
            DEBUG_PORT.print( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
            DEBUG_PORT.println( F(", but was never received.\n"
                              "  Please use NMEAorder.ino to determine which sentences your GPS device sends, and then\n"
                              "  use the last one for the definition in NMEAGPS_cfg.h.") );
          }

          hang();
        }
      }
    }
  }

  if (!valid_sentence_received)
    listenForSomething();

} // GPSloop

//--------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("NMEAdiagnostic.INO: started\n") );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );

  if (sizeof(gps_fix) <= 2) {
    warnings++;
    DEBUG_PORT.print( F("\nWarning: no fields are enabled in GPSfix_cfg.h.\n  Only the following information will be displayed:\n    ") );
    trace_header( DEBUG_PORT );
  }

  #if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
      !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
      !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
      !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST )
    warnings++;
    DEBUG_PORT.println( F("\nWarning: no messages are enabled for parsing in NMEAGPS_cfg.h.\n  No fields will be valid, including the 'status' field.") );
  #endif

  DEBUG_PORT.flush();

  // Start the UART for the GPS device
  tryBaud();

} // setup

//--------------------------

void loop()
{
  GPSloop();
}
