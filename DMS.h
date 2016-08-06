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
#include <stdint.h>

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
    
} NEOGPS_PACKED;

class Print;

extern Print & operator << ( Print & outs, const DMS_t & );

#endif