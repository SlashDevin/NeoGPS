#include <Arduino.h>

//  Serial is for trace output to the Serial Monitor window.

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
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
    !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
    !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
    !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST )

  #if defined(GPS_FIX_DATE)| defined(GPS_FIX_TIME)
    #error No NMEA sentences enabled: no fix data available for fusing.
  #else
    #warning No NMEA sentences enabled: no fix data available for fusing,\n\
   only pulse-per-second is available.
  #endif

#endif

//------------------------------------------------------------

static NMEAGPS  gps         ; // This parses received characters
static uint32_t last_rx = 0L; // The last millis() time a character was
                              // received from GPS.  This is used to
                              // determine when the GPS quiet time begins.

//------------------------------------------------------------
//  Define an extra set of GPS fix information.  It will
//  hold on to the various pieces as they are received from
//  different kinds of sentences.

static gps_fix fused;

#ifdef NMEAGPS_ACCUMULATE_FIX
  #error NMEAGPS_ACCUMULATE_FIX should not be enabled when explicit merging is used.
  // This is an Explicit Merge: "fused |= gps.fix()"
#endif

//------------------------------------------------------------
// For the NeoGPS example programs, "Streamers" is common set 
//   of printing and formatting routines for GPS data, in a
//   Comma-Separated Values text format (aka CSV).  The CSV
//   data will be printed to the "debug output device", called
//   "trace".  It's just an alias for the debug Stream.
//   Set "trace" to your debug output device, if it's not "Serial".
// If you don't need these formatters, simply delete this section.

#include "Streamers.h"
Stream & trace = Serial;

//------------------------------------
//  This is the main GPS parsing loop.

static void GPSloop()
{  
  while (gps_port.available()) {
    last_rx = millis();

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

      // All enabled sentence types will be merged into one fix.
      //   This 'fused' data can be safely used anywhere in your program.
      fused |= gps.fix();

    }
  }
} // GPSloop
  
//----------------------------------------------------------------
//  Determine whether the GPS quiet time has started, using the
//    current time, the last time a character was received,
//    and the last time a GPS quiet time started.

static bool quietTimeStarted()
{
  uint32_t current_ms       = millis();
  uint32_t ms_since_last_rx = current_ms - last_rx;

  if (ms_since_last_rx > 5) {

    // The GPS device has not sent any characters for at least 5ms.
    //   See if we've been getting chars sometime during the last second.
    //   If not, the GPS may not be working or connected properly.

    bool getting_chars = (ms_since_last_rx < 1000UL);

    static uint32_t last_quiet_time = 0UL;

    bool just_went_quiet = (((int32_t) (last_rx - last_quiet_time)) > 0L);
    bool next_quiet_time = ((current_ms - last_quiet_time) >= 1000UL);

    if ((getting_chars && just_went_quiet)
          ||
        (!getting_chars && next_quiet_time)) {

      if (!getting_chars) {
        trace.println( F("Check GPS device and/or connections.  No data received.\n") );
      }

      last_quiet_time = current_ms;  // Remember for next loop

      return true;
    }
  }

  return false;

} // quietTimeStarted

//----------------------------------------------------------------
//  This function gets called about once per second, at the beginning
//  of the GPS quiet time.  It's the best place to do anything that
//  might take a while: print a bunch of things, write to SD, send
//  an SMS, etc.
//
//  By doing the "hard" work during the quiet time, the CPU can get back to
//  reading the GPS chars as they come in, so that no chars are lost.

static void doSomeWork()
{
  // Print all the things!
  trace_all( gps, fused );

  // Clear out what we just printed.  If you need this data elsewhere,
  //   don't do this.
  gps.data_init();
  fused.init();

} // doSomeWork

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);  // change this to match 'trace'.  Can't do 'trace.begin'

  trace.print( F("NMEAfused.INO: started\n") );
  trace.print( F("fix object size = ") );
  trace.println( sizeof(gps.fix()) );
  trace.print( F("NMEAGPS object size = ") );
  trace.println( sizeof(gps) );
  trace.println( F("Looking for GPS device on " USING_GPS_PORT) );

  trace_header();

  trace.flush();
  
  // Start the UART for the GPS device
  gps_port.begin(9600);
}

//--------------------------

void loop()
{
  GPSloop();

  if (quietTimeStarted())
    doSomeWork();
}
