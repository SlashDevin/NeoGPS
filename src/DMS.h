#ifndef DMS_H
#define DMS_H

//------------------------------------------------------
// @file DMS.h
// @version 1.0
//
// @section License
// Copyright (C) 2016, SlashDevin
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//

#include "NeoGPS_cfg.h"
#include "Platform.h"
#include <stdint.h>

class Print;

enum Hemisphere_t { NORTH_H = 0, SOUTH_H = 1, EAST_H = 0, WEST_H = 1 };

class DMS_t
{
public:
  uint8_t      degrees;
  uint8_t      minutes       ;//NEOGPS_BF(6);
  Hemisphere_t hemisphere    ;//NEOGPS_BF(2); compiler bug!
  uint8_t      seconds_whole NEOGPS_BF(6);
  uint16_t     seconds_frac  NEOGPS_BF(10); // 1000ths

  void init() { degrees = minutes = seconds_whole = seconds_frac = 0;
                hemisphere = NORTH_H; }

  float secondsF() const { return seconds_whole + 0.001 * seconds_frac; };
  char  NS      () const { return (hemisphere == SOUTH_H) ? 'S' : 'N'; };
  char  EW      () const { return (hemisphere ==  WEST_H) ? 'W' : 'E'; };

  //.............................................................................
  // A utility function to convert from integer 'lat' or 'lon', scaled by 10^7

  void  From( int32_t deg_1E7 );

  // Print DMS as the funky NMEA DDDMM.mmmm format
  template <class Print>
  void printDDDMMmmmm( Print & outs ) const;

} NEOGPS_PACKED;

//----------------------------------------------------------------

template <class Print>
Print & operator << ( Print & outs, const DMS_t & dms )
{
  if (dms.degrees < 10)
    outs.write( '0' );
  outs.print( dms.degrees );
  outs.write( ' ' );
  if (dms.minutes < 10)
    outs.write( '0' );
  outs.print( dms.minutes );
  outs.print( F("\' ") );
  if (dms.seconds_whole < 10)
    outs.write( '0' );
  outs.print( dms.seconds_whole );
  outs.write( '.' );
  if (dms.seconds_frac < 100)
    outs.write( '0' );
  if (dms.seconds_frac < 10)
    outs.write( '0' );
  outs.print( dms.seconds_frac );
  outs.print( F("\" ") );

  return outs;
  
} // operator <<

//----------------------------------------------------------------

template <class Print>
void DMS_t::printDDDMMmmmm( Print & outs ) const
{
  outs.print( degrees );

  if (minutes < 10)
    outs.print( '0' );
  outs.print( minutes );
  outs.print( '.' );

  //  Calculate the fractional minutes from the seconds,
  //     *without* using floating-point numbers.

  uint16_t mmmm = seconds_whole * 166;  // same as 10000/60, less .66666...
  mmmm += (seconds_whole * 2 + seconds_frac/2 ) / 3;  // ... plus the remaining .66666
      // ... plus the seconds_frac, scaled by 10000/(60*1000) = 1/6, which
      //        is implemented above as 1/2 * 1/3 

  //  print leading zeroes, if necessary
  if (mmmm < 1000)
    outs.print( '0' );
  if (mmmm <  100)
    outs.print( '0' );
  if (mmmm <   10)
    outs.print( '0' );
  outs.print( mmmm );
}


#endif
