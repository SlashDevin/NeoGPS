#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAtimezone.ino
//
//  Description:  This program shows how to offset the GPS dateTime member
//          into your specific timezone.  GPS devices do not know which
//          timezone they are in, so they always report a UTC time.  This
//          is the same as GMT.
//
//  Prerequisites:
//     1) NMEA.ino works with your device
//     2) GPS_FIX_TIME is enabled in GPSfix_cfg.h
//     3) NMEAGPS_PARSE_RMC is enabled in NMEAGPS_cfg.h.  You could use 
//        any sentence that contains a time field.  Be sure to change the 
//        "if" statement in GPSloop from RMC to your selected sentence.
//
//  'Serial' is for trace output to the Serial Monitor window.
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

static NMEAGPS  gps         ; // This parses received characters
static gps_fix  fix_data;
static const NMEAGPS::nmea_msg_t LAST_SENTENCE_IN_INTERVAL = NMEAGPS::NMEA_GLL;

#if !defined(GPS_FIX_TIME)
  #error You must define GPS_FIX_TIME in GPSfix_cfg.h!
#endif

#if !defined(NMEAGPS_PARSE_RMC)
  #error You must define NMEAGPS_PARSE_RMC in NMEAGPS_cfg.h!
#endif

//----------------------------------------------------------------

static void doSomeWork()
{
  // Display the local time

  if (fix_data.valid.time) {
    // Set these values to the offset of your timezone from GMT
    static const int32_t         zone_hours   = -4L; // EST
    static const int32_t         zone_minutes =  0L; // usually zero
    static const NeoGPS::clock_t zone_offset  =
                      zone_hours   * NeoGPS::SECONDS_PER_HOUR +
                      zone_minutes * NeoGPS::SECONDS_PER_MINUTE;

    Serial << NeoGPS::time_t( fix_data.dateTime + zone_offset );
  }
  Serial.println();

  // Clear out what we just printed.  If you need this data elsewhere,
  //   don't do this.
  gps.data_init();
  fix_data.init();

} // doSomeWork

//------------------------------------

static void GPSloop()
{  
  while (gps_port.available()) {

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC) // or your favorite...
        fix_data = gps.fix();

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

  Serial.print( F("NMEAtimezone.INO: started\n") );
  Serial.print( F("fix object size = ") );
  Serial.println( sizeof(gps.fix()) );
  Serial.print( F("NMEAGPS object size = ") );
  Serial.println( sizeof(gps) );
  Serial.println( F("Looking for GPS device on " USING_GPS_PORT) );
  Serial.println( F("Local time") );
  Serial.flush();
  
  // Start the UART for the GPS device
  gps_port.begin( 9600 );
}

//--------------------------

void loop()
{
  GPSloop();
}
