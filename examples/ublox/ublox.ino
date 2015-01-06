/*
  Serial is for trace output.
  Serial1 should be connected to the GPS device.
*/

#include <Arduino.h>

#include "Streamers.h"

// Set this to your debug output device.
Stream & trace = Serial;

#include "ubxGPS.h"

#if defined(GPS_FIX_DATE) & !defined(GPS_FIX_TIME)
// uncomment this to display just one pulse-per-day.
//#define PULSE_PER_DAY
#endif

//--------------------------

class MyGPS : public ubloxGPS
{
public:

    gps_fix merged;

    enum
      {
        GETTING_STATUS, 
        GETTING_LEAP_SECONDS, 
        GETTING_UTC, 
        RUNNING
      }
        state:8;

    uint32_t last_rx;
    uint32_t last_trace;
    uint32_t last_sentence;


    bool ok_to_process;
    
    MyGPS( Stream *device ) : ubloxGPS( device )
      {
        state = GETTING_STATUS;
        last_rx = 0L;
        last_trace = 0L;
        last_sentence = 0L;
        ok_to_process = false;
      };

    //--------------------------

    void begin()
      {
        last_rx = millis();
        last_trace = seconds;
      }

    //--------------------------

    void run()
    {
      bool rx = false;

      while (Serial1.available()) {
        rx = true;
        if (decode( Serial1.read() ) == DECODE_COMPLETED) {
          if (ok_to_process)
            processSentence();
        }
      }

      uint32_t ms = millis();

      if (rx)
        last_rx = ms;

      else {
        if (ok_to_process && ((ms - last_rx) > 2000L)) {
          last_rx = ms;
          trace << F("RESTART!\n");
          if (state != GETTING_STATUS) {
            state = GETTING_STATUS;
            enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_STATUS );
          }
        }
      }
    }

    //--------------------------

    bool processSentence()
      {
        bool old_otp = ok_to_process;
        ok_to_process = false;

        bool ok = false;

        if (!ok && (nmeaMessage >= (nmea_msg_t)ubloxGPS::PUBX_00)) {
          ok = true;
        }

        if (!ok && (rx().msg_class != ublox::UBX_UNK)) {
          ok = true;

          // Use the STATUS message as a pulse-per-second
          if ((rx().msg_class == ublox::UBX_NAV) &&
              (rx().msg_id == ublox::UBX_NAV_STATUS))
            seconds++;
        }

        if (ok) {

          switch (state) {
            case GETTING_STATUS:
              if (fix().status != gps_fix::STATUS_NONE) {
                trace << F("Acquired status: ") << (uint8_t) fix().status << '\n';
#if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE)
                if (enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEGPS ))
                  state = GETTING_LEAP_SECONDS;
                else
                  trace << F("enable TIMEGPS failed!\n");
              }
              break;

            case GETTING_LEAP_SECONDS:
              if (GPSTime::leap_seconds != 0) {
                trace << F("Acquired leap seconds: ") << GPSTime::leap_seconds << '\n';
              }
              if (!disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEGPS ))
                trace << F("disable TIMEGPS failed!\n");
              else if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC ))
                trace << F("enable TIMEUTC failed!\n");
              else
                state = GETTING_UTC;
              break;

            case GETTING_UTC:
              if (GPSTime::start_of_week() != 0) {
                trace << F("Acquired UTC: ") << fix().dateTime << '\n';
                trace << F("Acquired Start-of-Week: ") << GPSTime::start_of_week() << '\n';

#if defined(GPS_FIX_LOCATION) | defined(GPS_FIX_ALTITUDE) | \
    defined(GPS_FIX_SPEED) | defined(GPS_FIX_HEADING)
                if (!disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC ))
                  trace << F("disable TIMEUTC failed!\n");
                else
                  state = RUNNING;
#else
                state = RUNNING;
#endif

#else

#if defined(GPS_FIX_TIME) | defined(GPS_FIX_DATE)
                if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC ))
                  trace << F("enable TIMEUTC failed!\n");
                else
                  state = RUNNING;
#else
                state = RUNNING;
#endif

#endif

#if (defined(GPS_FIX_LOCATION) | defined(GPS_FIX_ALTITUDE)) & \
    defined(UBLOX_PARSE_POSLLH)

                if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_POSLLH ))
                  trace << F("enable POSLLH failed!\n");
#endif

#if (defined(GPS_FIX_SPEED) | defined(GPS_FIX_HEADING)) & \
    defined(UBLOX_PARSE_VELNED)
                if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_VELNED ))
                  trace << F("enable VELNED failed!\n");
#endif

#if defined(NMEAGPS_PARSE_SATELLITES) & \
    defined(UBLOX_PARSE_SVINFO)
                if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_SVINFO ))
                  trace << PSTR("enable SVINFO failed!\n");
#endif

                trace_header();
              }
              break;

            default:
              // See if we stepped into a different time interval,
              //   or if it has finally become valid after a cold start.

              bool newInterval;
#if defined(GPS_FIX_TIME)
              newInterval = (fix().valid.time &&
                            (!merged.valid.time ||
                             (merged.dateTime.Second != fix().dateTime.Second) ||
                             (merged.dateTime.Minute != fix().dateTime.Minute) ||
                             (merged.dateTime.Hour   != fix().dateTime.Hour)));
#elif defined(PULSE_PER_DAY)
              newInterval = (fix().valid.date &&
                            (!merged.valid.date ||
                             (merged.dateTime.Day   != fix().dateTime.Day) ||
                             (merged.dateTime.Month != fix().dateTime.Month) ||
                             (merged.dateTime.Year  != fix().dateTime.Year)));
#else
              //  No date/time configured, so let's assume it's a new
              //  if the seconds have changed.
              newInterval = (seconds != last_sentence);
#endif

              if (newInterval) {

                //  Since we're into the next time interval, we throw away
                //     all of the previous fix and start with what we
                //     just received.
                merged = fix();

              } else {
                // Accumulate all the reports in this time interval
                merged |= fix();
              }
              break;
          }
        }

        last_sentence = seconds;

        ok_to_process = old_otp;

        return ok;
      }

    //--------------------------

    void traceIt()
    {
      if ((state == RUNNING) && (last_trace != 0))
        trace_all( *this, merged );

      last_trace = seconds;

    } // traceIt


} __attribute__((packed));

static MyGPS gps( &Serial1 );

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);
  trace << F("fix object size = ") << sizeof(gps.fix()) << '\n';
  trace << F("ubloxGPS object size = ") << sizeof(ubloxGPS) << '\n';
  trace << F("MyGPS object size = ") << sizeof(gps) << '\n';
  trace.flush();

  // Start the UART for the GPS device
  Serial1.begin(9600);

  gps.begin();

  // Turn off the preconfigured NMEA standard messages
  for (uint8_t i=NMEAGPS::NMEA_FIRST_MSG; i<=NMEAGPS::NMEA_LAST_MSG; i++) {
    ublox::configNMEA( gps, (NMEAGPS::nmea_msg_t) i, 0 );
  }

  // Turn on the UBX status message
  gps.enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_STATUS );

  // Turn off things that may be left on by a previous build
  gps.disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEGPS );
  gps.disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC );
  gps.disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_VELNED );
  gps.disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_POSLLH );

  gps.ok_to_process = true;
}

//--------------------------

void loop()
{
  gps.run();

  if ((gps.last_trace != seconds) &&
      (millis() - gps.last_rx > 5)) {

    // It's been 5ms since we received anything,
    // log what we have so far...
    gps.traceIt();
  }
}
