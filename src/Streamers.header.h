#pragma once

//  Copyright (C) 2014-2017, SlashDevin
//
//  This file is part of NeoGPS
//
//  NeoGPS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  NeoGPS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with NeoGPS.  If not, see <http://www.gnu.org/licenses/>.

#include "Platform.h"

#include <NMEAGPS.header.h>

/**
 * Print valid fix data to the given stream with the format
 *   "status,dateTime,lat,lon,heading,speed,altitude,satellites,
 *       hdop,vdop,pdop,lat_err,lon_err,alt_err"
 * The "header" above contains the actual compile-time configuration.
 * A comma-separated field will be empty if the data is NOT valid.
 * @param[in] outs output stream.
 * @param[in] fix gps_fix instance.
 * @return The outs object.
 */
NEO_GPS_PRINT & operator <<( NEO_GPS_PRINT & outs, const gps_fix &fix ) NEO_GPS_PRINT_DEFAULT_IMPL_WARN;

void trace_header( NEO_GPS_PRINT & outs ) NEO_GPS_PRINT_DEFAULT_IMPL_WARN;

void trace_all( NEO_GPS_PRINT & outs, const NMEAGPS &gps, const gps_fix &fix ) NEO_GPS_PRINT_DEFAULT_IMPL_WARN;
