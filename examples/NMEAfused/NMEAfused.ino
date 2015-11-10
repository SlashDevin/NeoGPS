#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAfused.ino
//
//  Description:  This program adds Explicit Merging.  All sentences
//       are merged into one safe copy.
//
//  Prerequisites:
//     1) NMEA.ino works with your device
//     2) At least one NMEA sentence has been enabled.
//     3) Implicit Merging is disabled.
//
//  Serial is for trace output to the Serial Monitor window.
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
  #include <NeoSWSerial.h>
  //#include <SoftwareSerial.h> /* NOT RECOMMENDED */
#endif
#include "GPSport.h"

#include "Streamers.h"
Stream & trace = Serial;

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
    !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
    !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
    !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST )

  #error No NMEA sentences enabled: no fix data available for fusing.

#endif

#ifdef NMEAGPS_ACCUMULATE_FIX

  #error NMEAGPS_ACCUMULATE_FIX should not be enabled when explicit merging is used.

  // This is an Explicit Merge: "fused |= gps.fix()"
  //
  // If you don't want or need the safe copy 'fused', you could
  //   use NMEA.ino with NMEAGPS_ACCUMULATE_FIX enabled instead.

#endif

//------------------------------------------------------------

static NMEAGPS  gps;
static gps_fix fused;

static const NMEAGPS::nmea_msg_t LAST_SENTENCE_IN_INTERVAL = NMEAGPS::NMEA_GLL;

//----------------------------------------------------------------

static void doSomeWork()
{
  // Print all the things!
  trace_all( gps, fused );

  // Clear out what we just printed.  If you need this data elsewhere,
  //   don't do this.
  gps.data_init();
  fused.init();

} // doSomeWork

//------------------------------------

static void GPSloop()
{  
  while (gps_port.available()) {

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

      // All enabled sentence types will be merged into one fix.
      //   This 'fused' data can be safely used anywhere in your program.

      fused |= gps.fix();

      if (gps.nmeaMessage == LAST_SENTENCE_IN_INTERVAL)
        doSomeWork();
    }
  }
} // GPSloop
  
//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);  // change this to match 'trace'.  Can't do 'trace.begin'

  Serial.print( F("NMEAfused.INO: started\n") );
  Serial.print( F("fix object size = ") );
  Serial.println( sizeof(gps.fix()) );
  Serial.print( F("NMEAGPS object size = ") );
  Serial.println( sizeof(gps) );
  Serial.println( F("Looking for GPS device on " USING_GPS_PORT) );

  trace_header();

  Serial.flush();
  
  // Start the UART for the GPS device
  gps_port.begin(9600);
}

//--------------------------

void loop()
{
  GPSloop();
}
