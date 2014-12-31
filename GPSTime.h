#ifndef GPSTIME_H
#define GPSTIME_H

#include "Time.h"

class GPSTime
{
  GPSTime();

  static time_t s_start_of_week;

public:

    /**
     * GPS time is offset from UTC by a number of leap seconds.  To convert a GPS
     * time to UTC time, the current number of leap seconds must be known.
     * See http://en.wikipedia.org/wiki/Global_Positioning_System#Leap_seconds
     */
    static uint8_t leap_seconds;

    /**
     * Some receivers report time WRT start of the current week, defined as
     * Sunday 00:00:00.  To save fairly expensive date/time calculations,
     * the UTC start of week is cached
     */
    static void start_of_week( tmElements_t & t )
      {
        s_start_of_week =
          makeTime(t)  -  
          (time_t) ((((t.Wday-1)*24L + 
                       t.Hour)*60L + 
                       t.Minute)*60L +
                       t.Second);
      }

    static time_t start_of_week()
    {
      return s_start_of_week;
    }

    /*
     * Convert a GPS time-of-week to UTC.
     * Requires /leap_seconds/ and /start_of_week/.
     */
    static time_t TOW_to_UTC( uint32_t time_of_week )
      { return (time_t) (start_of_week() + time_of_week - leap_seconds); }

    /**
     * Set /fix/ timestamp from a GPS time-of-week in milliseconds.
     * Requires /leap_seconds/ and /start_of_week/.
     **/
    static bool from_TOWms( uint32_t time_of_week_ms, tmElements_t &dt, uint16_t &ms )
    {
//trace << PSTR("from_TOWms(") << time_of_week_ms << PSTR("), sow = ") << start_of_week() << PSTR(", leap = ") << leap_seconds << endl;
      bool ok = (start_of_week() != 0) && (leap_seconds != 0);
      if (ok) {
        time_t tow_s = time_of_week_ms/1000UL;
        breakTime( TOW_to_UTC( tow_s ), dt );
        ms = (uint16_t)(time_of_week_ms - tow_s*1000UL);
      }
      return ok;
    }
};

#endif