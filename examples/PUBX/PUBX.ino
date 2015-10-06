#include <Arduino.h>
#include "ubxNMEA.h"

//======================================================================
//  Program: PUBX.ino
//
//  Description:  This program parses NMEA proprietary messages from
//     ublox devices.  It is an extension of NMEAfused.ino.
//
//  Prerequisites:
//     1) You have a ublox GPS device
//     2) NMEAfused.ino works with your device
//     3) You have installed ubxNMEA.H and ubxNMEA.CPP
//     4) At least one NMEA standard or proprietary sentence has been enabled.
//     5) Implicit Merging is disabled.
//
//  Serial is for trace output to the Serial Monitor window.
//
//======================================================================

#include "GPSport.h"
#include "Streamers.h"
Stream & trace = Serial;

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
    !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
    !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
    !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST ) & \
    !defined( NMEAGPS_PARSE_PUBX_00 ) & !defined( NMEAGPS_PARSE_PUBX_04 )

  #error No NMEA sentences enabled: no fix data available for fusing.

#endif

//------------------------------------------------------------

static ubloxNMEA gps         ; // This parses received characters
static uint32_t  last_rx = 0L; // The last millis() time a character was
                               // received from GPS.  This is used to
                               // determine when the GPS quiet time begins.

//------------------------------------------------------------
//  Define an extra set of GPS fix information.  It will
//  hold on to the various pieces as they are received from
//  different kinds of sentences.

static gps_fix fused;

static const NMEAGPS::nmea_msg_t LAST_SENTENCE_IN_INTERVAL =
   (NMEAGPS::nmea_msg_t) ubloxNMEA::PUBX_04;

//----------------------------------------------------------------

static void poll()
{
  gps.send_P( &Serial1, PSTR("PUBX,00") );
  gps.send_P( &Serial1, PSTR("PUBX,04") );
}

//----------------------------------------------------------------

static void doSomeWork()
{
  // Print all the things!
  trace_all( gps, fused );

  // Clear out what we just printed.  If you need this data elsewhere,
  //   don't do this.
  gps.data_init();
  fused.init();

  //  Ask for the proprietary messages again
  poll();
  
} // doSomeWork

//------------------------------------

static void GPSloop()
{  
  while (gps_port.available()) {
    last_rx = millis();

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

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

  trace.print( F("PUBX: started\n") );
  trace.print( F("fix object size = ") );
  trace.println( sizeof(gps.fix()) );
  trace.print( F("ubloxNMEA object size = ") );
  trace.println( sizeof(gps) );
  trace.println( F("Looking for GPS device on " USING_GPS_PORT) );

  trace_header();

  trace.flush();
  
  // Start the UART for the GPS device
  gps_port.begin(9600);

  // Ask for the special PUBX sentences
  poll();
}

//--------------------------

void loop()
{
  GPSloop();
}
