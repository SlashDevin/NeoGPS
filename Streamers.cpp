#include "Streamers.h"

Stream& operator <<( Stream &outs, const bool b )
  { outs.print( b ? 't' : 'f' ); return outs; }

Stream& operator <<( Stream &outs, const char c ) { outs.print(c); return outs; }

Stream& operator <<( Stream &outs, const uint16_t v ) { outs.print(v); return outs; }

Stream& operator <<( Stream &outs, const uint32_t v ) { outs.print(v); return outs; }

Stream& operator <<( Stream &outs, const int32_t v ) { outs.print(v); return outs; }

Stream& operator <<( Stream &outs, const uint8_t v ) { outs.print(v); return outs; }

Stream& operator <<( Stream &outs, const __FlashStringHelper *s )
{ outs.print(s); return outs; }

Stream& operator <<( Stream &outs, const tmElements_t & t)
{
  uint16_t full_year = tmYearToCalendar( t.Year );
  outs << full_year << '-';
  if (t.Month < 10) outs << '0';
  outs << t.Month << '-';
  if (t.Day < 10) outs << '0';
  outs << t.Day << ' ';
  if (t.Hour < 10) outs << '0';
  outs << t.Hour << ':';
  if (t.Minute < 10) outs << '0';
  outs << t.Minute << ':';
  if (t.Second < 10) outs << '0';
  outs << t.Second;
  return (outs);
}
