#include <Arduino.h>
#include <NMEAGPS.h>

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
//  'Serial' is for debug output to the Serial Monitor window.
//
//======================================================================

static NMEAGPS  gps; // This parses received characters
static gps_fix  fix; // This contains all the parsed pieces

//--------------------------
// CHECK CONFIGURATION

#if !defined(GPS_FIX_TIME) | !defined(GPS_FIX_DATE)
  #error You must define GPS_FIX_TIME and DATE in GPSfix_cfg.h!
#endif

#if !defined(NMEAGPS_PARSE_RMC) & !defined(NMEAGPS_PARSE_ZDA)
  #error You must define NMEAGPS_PARSE_RMC or ZDA in NMEAGPS_cfg.h!
#endif

//--------------------------
//   PICK A SERIAL PORT:
//
// *Best* choice is a HardwareSerial port:
//    You could use Serial on any board, but you will have to
//       disconnect the Arduino RX pin 0 from the GPS TX pin to
//       upload a new sketch over USB.  This is very reliable
//#define gpsPort Serial
//#define GPS_PORT_NAME "Serial"

// Use Serial1 on a Mega, Leo or Due board
#define gpsPort Serial1
#define GPS_PORT_NAME "Serial1"

// Use NeoHWSerial if you want to handle GPS characters
//   in an Interrupt Service Routine.
//   Also uncomment NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h.
//#include <NeoHWSerial.h>
//#define gpsPort NeoSerial
//#define GPS_PORT_NAME "NeoSerial"

//--------------------------
// 2nd best:
//#include <AltSoftSerial.h>
//AltSoftSerial gpsPort; // must be on specific pins (8 & 9 for an UNO)
//#define GPS_PORT_NAME "AltSoftSerial"

// Use NeoICSerial if you want to handle GPS characters
//   in an Interrupt Service Routine.
//   Also uncomment NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h.
//#include <NeoICSerial.h>
//NeoICSerial gpsPort; // must be on specific pins (8 & 9 for an UNO)
//#define GPS_PORT_NAME "NeoICSerial"

//--------------------------
// 3rd best: must be baud rate 9600, 19200 or 38400
//   NeoSWSerial supports handling GPS characters
//   in an Interrupt Service Routine.  If you want to do that,
//   also uncomment NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h.
//#include <NeoSWSerial.h>
//NeoSWSerial gpsPort(3, 2);
//#define GPS_PORT_NAME "NeoSWSerial(3,2)"

//--------------------------
// Worst: SoftwareSerial NOT RECOMMENDED

#ifdef NeoHWSerial_h
  //  Can't use Serial when NeoHWSerial is used.  Print to "NeoSerial" instead.
  #define DEBUG_PORT NeoSerial
#else
  #define DEBUG_PORT Serial
#endif

//--------------------------
// Set these values to the offset of your timezone from GMT

static const int32_t          zone_hours   = -5L; // EST
static const int32_t          zone_minutes =  0L; // usually zero
static const NeoGPS::clock_t  zone_offset  =
                                zone_hours   * NeoGPS::SECONDS_PER_HOUR +
                                zone_minutes * NeoGPS::SECONDS_PER_MINUTE;

// Uncomment one DST changeover rule, or define your own:
#define USA_DST
//#define EU_DST

#if defined(USA_DST)
  static const uint8_t springMonth =  3;
  static const uint8_t springDate  = 14; // latest 2nd Sunday
  static const uint8_t springHour  =  2;
  static const uint8_t fallMonth   = 11;
  static const uint8_t fallDate    =  7; // latest 1st Sunday
  static const uint8_t fallHour    =  2;
  #define CALCULATE_DST

#elif defined(EU_DST)
  static const uint8_t springMonth =  3;
  static const uint8_t springDate  = 31; // latest last Sunday
  static const uint8_t springHour  =  1;
  static const uint8_t fallMonth   = 10;
  static const uint8_t fallDate    = 31; // latest last Sunday
  static const uint8_t fallHour    =  1;
  #define CALCULATE_DST
#endif

//--------------------------

void adjustTime( NeoGPS::time_t & dt )
{
  NeoGPS::clock_t seconds = dt; // convert date/time structure to seconds

  #ifdef CALCULATE_DST
    //  Calculate DST changeover times once per reset and year!
    static NeoGPS::time_t  changeover;
    static NeoGPS::clock_t springForward, fallBack;

    if ((springForward == 0) || (changeover.year != dt.year)) {

      //  Calculate the spring changeover time (seconds)
      changeover.year    = dt.year;
      changeover.month   = springMonth;
      changeover.date    = springDate;
      changeover.hours   = springHour;
      changeover.minutes = 0;
      changeover.seconds = 0;
      changeover.set_day();
      // Step back to a Sunday, if day != SUNDAY
      changeover.date -= (changeover.day - NeoGPS::time_t::SUNDAY);
      springForward = (NeoGPS::clock_t) changeover;

      //  Calculate the fall changeover time (seconds)
      changeover.month   = fallMonth;
      changeover.date    = fallDate;
      changeover.hours   = fallHour - 1; // to account for the "apparent" DST +1
      changeover.set_day();
      // Step back to a Sunday, if day != SUNDAY
      changeover.date -= (changeover.day - NeoGPS::time_t::SUNDAY);
      fallBack = (NeoGPS::clock_t) changeover;
    }
  #endif

  //  First, offset from UTC to the local timezone
  seconds += zone_offset;

  #ifdef CALCULATE_DST
    //  Then add an hour if DST is in effect
    if ((springForward <= seconds) && (seconds < fallBack))
      seconds += NeoGPS::SECONDS_PER_HOUR;
  #endif

  dt = seconds; // convert seconds back to a date/time structure

} // adjustTime

//--------------------------

static void GPSloop()
{  
  while (gps.available( gpsPort )) {
    fix = gps.read();
    // Display the local time

    if (fix.valid.time && fix.valid.date) {
      adjustTime( fix.dateTime );

      DEBUG_PORT << fix.dateTime;
    }
    DEBUG_PORT.println();
  }

} // GPSloop

//--------------------------

#ifdef NMEAGPS_INTERRUPT_PROCESSING
  static void GPSisr( uint8_t c )
  {
    gps.handle( c );
  }
#endif

//--------------------------

void setup()
{
  // Start the normal trace output
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("NMEAtimezone.INO: started\n") );
  DEBUG_PORT.println( F("Looking for GPS device on " GPS_PORT_NAME ) );
  DEBUG_PORT.println( F("Local time") );
  DEBUG_PORT.flush();
  
  // Start the UART for the GPS device
  gpsPort.begin( 9600 );
  #ifdef NMEAGPS_INTERRUPT_PROCESSING
    gpsPort.attachInterrupt( GPSisr );
  #endif
}

//--------------------------

void loop()
{
  GPSloop();
}
