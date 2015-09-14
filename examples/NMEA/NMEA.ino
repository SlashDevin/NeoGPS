/*
  Serial is for trace output to the Serial Monitor window.

  For Mega Boards, Serial1 should be connected to the GPS device:
    pin 18 to the GPS RX pin, and pin 19 to the GPS TX pin.  You can
    change this to Serial2 or Serial3 (see line 25).

  For all other Boards, pin 3 should be connected to the GPS TX pin, and
    pin 4 should be connected to the GPS RX pin.  You can change these
    pin numbers below (see lines 38 and 41).
*/

#include <Arduino.h>

#include "NMEAGPS.h"
#include "Streamers.h"

//-----------------------------------------------------------
// Pick the serial port to which the GPS device is connected.

#if defined(UBRR1H)

  // The current Board (a Mega?) has an extra hardware serial port
  //   on pins 18 (TX1) and 19 (RX1)
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

  //  Here's a little preprocessor magic to get a nice string for /setup/
  #define f(x) #x
  #define STRINGIZE(f,x) f(x)
  #define USING_GPS_PORT "SoftwareSerial( RX pin " STRINGIZE(f,RX_PIN) ", TX pin " STRINGIZE(f,TX_PIN) " )"

#endif

//------------------------------------------------------------
// Set this to your debug output device, if it's not "Serial".
Stream & trace = Serial;

static NMEAGPS gps;

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);
  trace.print( F("NMEA test: started\n") );
  trace.print( F("fix object size = ") );
  trace.println( sizeof(gps.fix()) );
  trace.print( F("NMEAGPS object size = ") );
  trace.println( sizeof(NMEAGPS) );
  trace.println( F("Looking for GPS device on " USING_GPS_PORT) );

  #if defined(GPS_FIX_TIME)
    trace.print( F("Local time,") );
  #endif
  trace_header();

  trace.flush();
  
  // Start the UART for the GPS device
  gps_port.begin(9600);
}

//--------------------------

void loop()
{
  static uint32_t last_rx = 0L;
  static gps_fix  rmc_data;

  while (gps_port.available()) {
    last_rx = millis();

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

      // Make sure that the only sentence we care about is enabled
      #ifndef NMEAGPS_PARSE_RMC
        #error NMEAGPS_PARSE_RMC must be defined in NMEAGPS_cfg.h!
      #endif

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC) {
        rmc_data = gps.fix(); // copied for printing later...

        //  Use received GPRMC sentence as a pulse
        seconds++;
      }
    }
  }

  static uint32_t last_seconds = 0UL; // Remember the last time we printed

  //  Make sure we print *something* every two seconds, even if we
  //    haven't gotten any valid data.  
  //  This will show whether we have been getting *any* characters.

  static uint32_t last_time_seconds_changed = 0UL;

  if (millis() - last_time_seconds_changed > 2000UL)
    seconds += 2;

  if (last_seconds != seconds) {
    last_seconds = seconds;
    last_time_seconds_changed = millis();
  }

  // Print things out once per second, after the serial input has died down.
  // This prevents input buffer overflow during printing.

  static uint32_t last_trace = 0UL;

  if ((last_trace != seconds) && (millis() - last_rx > 5)) {
    last_trace = seconds;

    // It's been 5ms since we received anything, log what we have so far...

    #if defined(GPS_FIX_TIME)
      // Display the local time
      if (rmc_data.valid.time) {
        static const int32_t         zone_hours   = -4L; // EST
        static const int32_t         zone_minutes =  0L;
        static const NeoGPS::clock_t zone_offset  =
                          zone_hours   * NeoGPS::SECONDS_PER_HOUR +
                          zone_minutes * NeoGPS::SECONDS_PER_MINUTE;

        trace << NeoGPS::time_t( rmc_data.dateTime + zone_offset );
      }
      trace << ',';
    #endif

    trace_all( gps, rmc_data );
  }
}
