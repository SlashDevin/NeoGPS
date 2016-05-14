#ifndef DMS_H
#define DMS_H

#include "NeoGPS_cfg.h"

enum Hemisphere_t { NORTH_H = 0, SOUTH_H = 1, EAST_H = 0, WEST_H = 1 };

class DMS_t
{
public:
  uint8_t      degrees;
  uint8_t      minutes       NEOGPS_BF(6);
  Hemisphere_t hemisphere    NEOGPS_BF(2);
  uint8_t      seconds_whole NEOGPS_BF(6);
  uint16_t     seconds_frac  NEOGPS_BF(10); // 1000ths

  void init() { degrees = minutes = seconds_whole = seconds_frac = 0;
                hemisphere = NORTH_H; }

  float secondsF() const { return seconds_whole + 0.001 * seconds_frac; };
  char  NS      () const { return (hemisphere == SOUTH_H) ? 'S' : 'N'; };
  char  EW      () const { return (hemisphere ==  WEST_H) ? 'W' : 'E'; };

  //----------------------------------------------------
  // A utility function to convert from integer 'lat' or 'lon', scaled by 10^7
  //   Note that no division is used, and shifts are on byte boundaries.  Fast!

  void  From( int32_t deg_1E7 )
    {
      const uint32_t E7 = 10000000UL;

      if (deg_1E7 < 0) {
        deg_1E7 = -deg_1E7;
        hemisphere = SOUTH_H; // or WEST_H
      } else
        hemisphere = NORTH_H; // or EAST_H

      const uint32_t div_E32 = 429; // 1e-07 * 2^32
      degrees = ((deg_1E7 >> 16) * div_E32) >> 16;
      uint32_t remainder = deg_1E7 - degrees * E7;

      remainder *= 60; // to minutes * E7
      minutes = ((remainder >> 16) * div_E32) >> 16;
      remainder -= minutes * E7;

      remainder *= 60; // to seconds * E7
      uint32_t secs = ((remainder >> 16) * div_E32) >> 16;
      remainder -= secs * E7;

      const uint32_t div_1E4_E24 = 1677; // 0.00001 * 2^24
      seconds_frac  = (((remainder >> 8) * div_1E4_E24) >> 16); // thousandths
      seconds_whole = secs;

      // Carry if thousandths too big
      if (seconds_frac >= 1000) {
        seconds_frac -= 1000;
        seconds_whole++;
        if (seconds_whole >= 60) {
          seconds_whole -= 60;
          minutes++;
          if (minutes >= 60) {
            minutes -= 60;
            degrees++;
          }
        }
      }

    }; // From
    
} NEOGPS_PACKED;

#endif