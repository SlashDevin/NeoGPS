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
//  'Serial' is for trace output to the Serial Monitor window.
//
//======================================================================

#include "GPSport.h"
#include "Streamers.h"
Stream & trace = Serial;

static NMEAGPS  gps         ; // This parses received characters
static uint32_t last_rx = 0L; // The last millis() time a character was
                              // received from GPS.

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

static void tryAnotherBaudRate()
{
  gps_port.end();

  if (baud_index == INITIAL_BAUD_INDEX)
    baud_index = 0;

  else {
    baud_index++;
    if (baud_index == INITIAL_BAUD_INDEX)
      baud_index++; // skip it, we already tried it

    if (baud_index >= num_bauds) {
      baud_index = INITIAL_BAUD_INDEX;
      trace << F("\n  All baud rates tried!\n");
      for (;;)
        ; // hang!
    }
  }

  long baud = baud_table[baud_index];
  trace.print( F("\n____________________________\n\nChecking ") );
  trace.print( baud );
  trace.print( F(" baud...\n\n") );
  trace.flush();

  gps_port.begin( baud );

  triedDifferentBaud = true;

} // tryAnotherBaudRate

//------------------------------------

static bool saveSomeChars = false;
static const uint16_t MAX_SAMPLE = 256;
static uint8_t someChars[ MAX_SAMPLE ];
static uint16_t someCharsIndex = 0;

static void dumpSomeChars()
{
  trace << F("Received data:\n");

  const uint16_t  bytes_per_line = 16;
        char      ascii[ bytes_per_line ];
        uint8_t  *ptr            = &someChars[0];

  for (uint16_t i=0; i<someCharsIndex; ) {
    uint16_t j;
    
    for (j=0; (i<someCharsIndex) && (j<bytes_per_line); i++, j++) {
      uint8_t c = *ptr++;
      if (c < 0x10)
        trace.print('0');
      trace.print( c, HEX );
      if ((' ' <= c) && (c <= '~'))
        ascii[ j ] = c;
      else
        ascii[ j ] = '.';
    }

    uint16_t jmax = j;
    while (j++ < bytes_per_line)
      trace.print( F("  ") );
    trace.print( ' ' );
    
    for (j=0; j<jmax; j++)
      trace.print( ascii[ j ] );
    trace.print( '\n' );
  }

  someCharsIndex = 0;

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

           bool     getting_chars   = (ms_since_last_rx < 1000UL);
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
      
        trace.println( F("\nCheck GPS device and/or connections.  No data received.\n") );
      
        if (tries++ >= 3) {
          trace << F("END.\n");
          for (;;)
            ; // hang!
        }

      } else {
        tries = 1;
        
        trace.println( F("No valid sentences, but characters are being received.\n"
        "Check baud rate or device protocol configuration.\n" ) );

        if (saveSomeChars) {
          dumpSomeChars();
          delay( 2000 );

          tryAnotherBaudRate();
        } else
          saveSomeChars = true;
      }
    }
  }
  
} // listenForSomething

//------------------------------------

static void GPSloop()
{  
  while (gps_port.available()) {
    last_rx = millis();

    uint8_t c = gps_port.read();

    if (saveSomeChars && (someCharsIndex < MAX_SAMPLE))
      someChars[ someCharsIndex++ ] = c;

    if (gps.decode( c ) == NMEAGPS::DECODE_COMPLETED) {

      // We received a sentence, display the baud rate
      trace << F("\n\n**** NMEA sentence detected!  "
                 "Your device baud rate is ") <<
        baud_table[ baud_index ] << F("  ****\n");

      for (;;)
        ; // All done!
    }
  }

  listenForSomething();

} // GPSloop

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);  // change this to match 'trace'.  Can't do 'trace.begin'

  trace.print( F("NMEAdiagnostic.INO: started\n") );
  trace.println( F("Looking for GPS device on " USING_GPS_PORT) );
  trace.flush();
  
  // Start the UART for the GPS device
  gps_port.begin( baud_table[ baud_index ] );
}

//--------------------------

void loop()
{
  GPSloop();
}
