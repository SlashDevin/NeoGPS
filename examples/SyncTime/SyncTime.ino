#include <Arduino.h>
#include <NMEAGPS.h>

//======================================================================
//  Program: SyncTime.ino
//
//  Description:  This program shows how to update the sub-second
//       part of a clock's seconds.  You can adjust the clock update
//       interval to be as small as 9ms.  It will calculate the
//       correct ms offset from each GPS second.
//
//  Prerequisites:
//     1) NMEA.ino works with your device
//     2) GPS_FIX_TIME is enabled in GPSfix_cfg.h
//     3) NMEAGPS_PARSE_RMC is enabled in NMEAGPS_cfg.h.  You could use 
//        any sentence that contains a time field.  Be sure to change the 
//        "if" statement in GPSloop from RMC to your selected sentence.
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

static NMEAGPS  gps         ; // This parses received characters
static gps_fix  fix_data;

#if !defined(GPS_FIX_TIME)
  #error You must define GPS_FIX_TIME in GPSfix_cfg.h!
#endif

#if !defined(NMEAGPS_PARSE_RMC)
  #error You must define NMEAGPS_PARSE_RMC in NMEAGPS_cfg.h!
#endif

#ifdef NMEAGPS_INTERRUPT_PROCESSING
  #error You must *NOT* define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
#endif

//----------------------------------------------------------------
// Set these values to the offset of your timezone from GMT

static const int32_t zone_hours          = -4L; // EST
static const int32_t zone_minutes        =  0L; // usually zero
static const NeoGPS::clock_t zone_offset =
                        zone_hours   * NeoGPS::SECONDS_PER_HOUR +
                        zone_minutes * NeoGPS::SECONDS_PER_MINUTE;

//----------------------------------------------------------------

const uint32_t CLOCK_INTERVAL_US = 100UL * 1000UL; // 100ms
const uint16_t GPS_INTERVAL_MS   = 1000;

//----------------------------------------------------------------

static gps_fix         fix;
static uint32_t        fix_us;       // the micros() time when fix was set
static bool            synced;

static NeoGPS::clock_t localSeconds; // seconds since the EPOCH
static NeoGPS::time_t  localTime;

static void showTime( uint16_t subs, uint16_t factor = 100 /* hundredths */ )
{
//  DEBUG_PORT << localTime;
  DEBUG_PORT.print( localTime.seconds );
  DEBUG_PORT.print( '.' );
  
  // Leading zeroes
  for (;;) {
    factor /= 10;
    if ((factor < 10) || (subs >= factor))
      break;
    DEBUG_PORT.print( '0' );
  }

  DEBUG_PORT.println( subs );

} // showTime

//--------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("SyncTime.INO: started\n") );
  DEBUG_PORT.println( F("Looking for GPS device on " USING_GPS_PORT) );
  DEBUG_PORT.println( F("Local time seconds.milliseconds") );
  DEBUG_PORT.flush();
  
  // Start the UART for the GPS device
  gps_port.begin( 9600 );
}

//--------------------------

static uint32_t lastShowTime;

void loop()
{
  uint32_t rightNow;

  while (gps.available( gps_port )) {
    fix      = gps.read();
    fix_us   = micros(); // set this with PPS pin change or input capture interrupt?
    synced   = true;

    //  If we got a GPS time value, update the local time
    if (fix.valid.time) {
      localSeconds = fix.dateTime + zone_offset;
      localTime    = localSeconds; // fill out the structure
    }
  }

  bool ok;
  if (!synced) {
    rightNow = micros();
    ok       = false;
  } else {
    synced       = false;
    rightNow     = fix_us;
    lastShowTime = rightNow;
    ok           = true;
  }

  uint16_t ms  = (rightNow - fix_us) / 1000UL;
  if (ms < GPS_INTERVAL_MS) {

    if (!ok) {
      // Step by intervals until we're current
      while ((rightNow - lastShowTime) >= CLOCK_INTERVAL_US) {
        lastShowTime += CLOCK_INTERVAL_US;
        ok            = true;
      }
    }

    if (ok)
      showTime( ms, 1000 );

  } // else wait for GPS
}
