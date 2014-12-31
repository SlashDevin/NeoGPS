/*
  Serial is for trace output.
  Serial1 should be connected to the GPS device.
*/

#include <Arduino.h>

#include "Streamers.h"

#include "ubxGPS.h"

#if !defined(GPS_FIX_DATE) & !defined(GPS_FIX_TIME)
static uint32_t seconds = 0L;
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

    bool ok_to_process;
    
    MyGPS( Stream *device ) : ubloxGPS( device )
      {
        state = GETTING_STATUS;
        ok_to_process = false;
      };

    //--------------------------

    void run()
    {
      static uint32_t last = 0;
      uint32_t ms = millis();
      if (last == 0) last = ms;

      if ((ms - last) > 2000L) {
        last = ms;
        trace << F("RESTART!\n");
        if (state != GETTING_STATUS) {
          state = GETTING_STATUS;
          enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_STATUS );
        }
      } else {
        while (Serial1.available()) {
          if (decode( Serial1.read() ) == DECODE_COMPLETED) {
            last = ms;
            if (ok_to_process)
              processSentence();
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
//trace << F("n ") << (uint8_t) nmeaMessage << '\n';
          ok = true;
        }
        if (!ok && (rx().msg_class != ublox::UBX_UNK)) {
//trace << F("u ") << (uint8_t) rx().msg_class << F("/") << (uint8_t) rx().msg_id << '\n';
          ok = true;
#if !defined(GPS_FIX_DATE) & !defined(GPS_FIX_TIME)
          if ((rx().msg_class == ublox::UBX_NAV) &&
              (rx().msg_id == ublox::UBX_NAV_STATUS))
            seconds++;
#endif
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

#if defined(GPS_FIX_LOCATION) | defined(GPS_FIX_ALTITUDE)
                if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_POSLLH ))
                  trace << F("enable POSLLH failed!\n");
#endif

#if defined(GPS_FIX_SPEED) | defined(GPS_FIX_HEADING)
                if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_VELNED ))
                  trace << F("enable VELNED failed!\n");
#endif
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
#elif defined(GPS_FIX_DATE)
              newInterval = (fix().valid.date &&
                            (!merged.valid.date ||
                             (merged.dateTime.Day   != fix().dateTime.Day) ||
                             (merged.dateTime.Month != fix().dateTime.Month) ||
                             (merged.dateTime.Year  != fix().dateTime.Year)));
#else
              //  No date/time configured, so let's assume it's a new interval
              //  if it has been a while since the last sentence was received.
              static uint32_t last_sentence = 0L;

              newInterval = (seconds != last_sentence);
              last_sentence = seconds;
#endif
//trace << F("ps mvd ") << merged.valid.date << F("/") << fix().valid.date;
//trace << F(", mvt ") << merged.valid.time << F("/") << fix().valid.time;
//trace << merged.dateTime << F("/") << fix().dateTime;
//trace << F(", ni = ") << newInterval << '\n';
              if (newInterval) {

                // Log the previous interval
                traceIt();

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

        ok_to_process = old_otp;

        return ok;
      }

    //--------------------------

    void traceIt()
    {
#if !defined(GPS_FIX_DATE) & !defined(GPS_FIX_TIME)
      trace << seconds << ',';
#endif

      trace << merged << '\n';

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
  Serial.flush();

  // Start the UART for the GPS device
  Serial1.begin(9600);

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
}
