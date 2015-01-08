#include "Streamers.h"

#include "NMEAGPS.h"

//#define USE_FLOAT

Stream& operator <<( Stream &outs, const bool b )
  { outs.print( b ? 't' : 'f' ); return outs; }

Stream& operator <<( Stream &outs, const char c ) { outs.print(c); return outs; }

Stream& operator <<( Stream &outs, const uint16_t v ) { outs.print(v); return outs; }

Stream& operator <<( Stream &outs, const uint32_t v ) { outs.print(v); return outs; }

Stream& operator <<( Stream &outs, const int32_t v ) { outs.print(v); return outs; }

Stream& operator <<( Stream &outs, const uint8_t v ) { outs.print(v); return outs; }

Stream& operator <<( Stream &outs, const __FlashStringHelper *s )
{ outs.print(s); return outs; }

//------------------------------------------

const char gps_fix_header[] __PROGMEM =
  "Status,"

#if defined(GPS_FIX_DATE) | defined(GPS_FIX_TIME)
#if defined(GPS_FIX_DATE)
  "Date"
#endif
#if defined(GPS_FIX_DATE) & defined(GPS_FIX_TIME)
  "/"
#endif
#if defined(GPS_FIX_TIME)
  "Time"
#endif
  ","
#endif

#ifdef GPS_FIX_LOCATION
  "Lat,Lon,"
#endif

#if defined(GPS_FIX_ALTITUDE)
  "Alt,"
#endif

#if defined(GPS_FIX_SPEED)
  "Spd,"
#endif

#if defined(GPS_FIX_HEADING)
  "Hdg,"
#endif

#if defined(GPS_FIX_SATELLITES)
  "Sats,"
#endif

#if defined(GPS_FIX_HDOP)
  "HDOP,"
#endif

#if defined(GPS_FIX_VDOP)
  "VDOP,"
#endif

#if defined(GPS_FIX_PDOP)
  "PDOP,"
#endif

#if defined(GPS_FIX_LAT_ERR)
  "Lat err,"
#endif

#if defined(GPS_FIX_LON_ERR)
  "Lon err,"
#endif

#if defined(GPS_FIX_ALT_ERR)
  "Alt err,"
#endif
  ;

//...............

Stream & operator <<( Stream &outs, const gps_fix &fix )
{
  if (fix.valid.status)
    outs << (uint8_t) fix.status;
  outs << ',';

#if defined(GPS_FIX_DATE) | defined(GPS_FIX_TIME)
  bool someTime = false;
#if defined(GPS_FIX_DATE)
  someTime |= fix.valid.date;
#endif
#if defined(GPS_FIX_TIME)
  someTime |= fix.valid.time;
#endif

  if (someTime) {
    outs << fix.dateTime << '.';
    if (fix.dateTime_cs < 10)
      outs << '0';
    outs << fix.dateTime_cs;
  }
  outs << ',';
#endif

#ifdef USE_FLOAT
#ifdef GPS_FIX_LOCATION
  if (fix.valid.location) {
    outs.print( fix.latitude(), 6 );
    outs << ',';
    outs.print( fix.longitude(), 6 );
  } else
    outs << ',';
  outs << ',';
#endif
#ifdef GPS_FIX_HEADING
  if (fix.valid.heading)
    outs.print( fix.heading(), 2 );
  outs << ',';
#endif
#ifdef GPS_FIX_SPEED
  if (fix.valid.speed)
    outs.print( fix.speed(), 3 );
  outs << ',';
#endif
#ifdef GPS_FIX_ALTITUDE
  if (fix.valid.altitude)
    outs.print( fix.altitude(), 2 );
  outs << ',';
#endif

#else /* not USE_FLOAT */

#ifdef GPS_FIX_LOCATION
  if (fix.valid.location)
    outs << fix.latitudeL() << ',' << fix.longitudeL();
  else
    outs << ',';
  outs << ',';
#endif
#ifdef GPS_FIX_HEADING
  if (fix.valid.heading)
    outs << fix.heading_cd();
  outs << ',';
#endif
#ifdef GPS_FIX_SPEED
  if (fix.valid.speed)
    outs << fix.speed_mkn();
  outs << ',';
#endif
#ifdef GPS_FIX_ALTITUDE
  if (fix.valid.altitude)
    outs << fix.altitude_cm();
  outs << ',';
#endif
#endif

#ifdef GPS_FIX_SATELLITES
  if (fix.valid.satellites)
    outs << fix.satellites;
  outs << ',';
#endif

#ifdef USE_FLOAT

#ifdef GPS_FIX_HDOP
  if (fix.valid.hdop)
    outs.print( (fix.hdop * 0.001), 3 );
  outs << ',';
#endif
#ifdef GPS_FIX_VDOP
  if (fix.valid.vdop)
    outs.print( (fix.vdop * 0.001) 3 );
  outs << ',';
#endif
#ifdef GPS_FIX_PDOP
  if (fix.valid.pdop)
    outs.print( (fix.pdop * 0.001), 3 );
  outs << ',';
#endif

#ifdef GPS_FIX_LAT_ERR
  if (fix.valid.lat_err)
    outs.print( fix.lat_err(), 2 );
  outs << ',';
#endif
#ifdef GPS_FIX_LON_ERR
  if (fix.valid.lon_err)
    outs.print( fix.lon_err(), 2 );
  outs << ',';
#endif
#ifdef GPS_FIX_ALT_ERR
  if (fix.valid.alt_err)
    outs.print( fix.alt_err(), 2 );
  outs << ',';
#endif

#else

#ifdef GPS_FIX_HDOP
  if (fix.valid.hdop)
    outs << fix.hdop;
  outs << ',';
#endif
#ifdef GPS_FIX_VDOP
  if (fix.valid.vdop)
    outs << fix.vdop;
  outs << ',';
#endif
#ifdef GPS_FIX_PDOP
  if (fix.valid.pdop)
    outs << fix.pdop;
  outs << ',';
#endif

#ifdef GPS_FIX_LAT_ERR
  if (fix.valid.lat_err)
    outs << fix.lat_err_cm;
  outs << ',';
#endif
#ifdef GPS_FIX_LON_ERR
  if (fix.valid.lon_err)
    outs << fix.lon_err_cm;
  outs << ',';
#endif
#ifdef GPS_FIX_ALT_ERR
  if (fix.valid.alt_err)
    outs << fix.alt_err_cm;
  outs << ',';
#endif

#endif

  return outs;
}

//-----------------------------

static const char NMEAGPS_header[] __PROGMEM =
#if defined(NMEAGPS_PARSE_SATELLITES)
  "[sat"
#if defined(NMEAGPS_PARSE_SATELLITE_INFO)
  " elev/az @ SNR"
#endif
  "],"
#endif
  "";

void trace_header()
{
#if !defined(GPS_FIX_TIME) & !defined(GPS_FIX_DATE)
  trace << F("s,");
#endif

  trace.print( (const __FlashStringHelper *) &gps_fix_header[0] );
  trace.print( (const __FlashStringHelper *) &NMEAGPS_header[0] );

  trace << '\n';
}

//--------------------------

uint32_t seconds = 0L;

void trace_all( const NMEAGPS &gps, const gps_fix &fix )
{
#if !defined(GPS_FIX_TIME) & !defined(GPS_FIX_DATE)
  //  Date/Time not enabled, just output the interval number
  trace << seconds << ',';
#endif

  trace << fix;

#if defined(NMEAGPS_PARSE_SATELLITES)
  if (fix.valid.satellites && gps.satellites_valid()) {
    trace << '[';

    uint8_t i_max = fix.satellites;
    if (i_max > NMEAGPS::MAX_SATELLITES)
      i_max = NMEAGPS::MAX_SATELLITES;

    for (uint8_t i=0; i < i_max; i++) {
      trace << gps.satellites[i].id;
#if defined(NMEAGPS_PARSE_SATELLITE_INFO)
      trace << ' ' << 
        gps.satellites[i].elevation << '/' << gps.satellites[i].azimuth;
      trace << '@';
      if (gps.satellites[i].tracked)
        trace << gps.satellites[i].snr;
      else
        trace << '-';
#endif
      trace << ',';
    }
    trace << ']';
  }
#endif

  trace << '\n';

} // trace_all
