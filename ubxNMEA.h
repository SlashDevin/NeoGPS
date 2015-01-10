#ifndef _UBXNMEA_H_
#define _UBXNMEA_H_

/**
 * @file UBXNMEA.h
 * @version 2.1
 *
 * @section License
 * Copyright (C) 2014, SlashDevin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "NMEAGPS.h"

/**
 * Enable/disable the parsing of specific NMEA sentences.
 *
 * Configuring out a sentence prevents its fields from being parsed.
 * However, the sentence type will still be recognized by /decode/ and 
 * stored in member /nmeaMessage/.  No valid flags would be available.
 *
 */
#define NMEAGPS_PARSE_PUBX_00
#define NMEAGPS_PARSE_PUBX_04

#ifndef NMEAGPS_DERIVED_TYPES
#error You must "#define NMEAGPS_DERIVED_TYPES" in NMEAGPS.h!
#endif

/**
 * NMEA 0183 Parser for ublox Neo-6 GPS Modules.
 *
 * @section Limitations
 * Very limited support for ublox proprietary NMEA messages.
 * Only NMEA messages of types PUBX,00 and PUBX,04 are parsed.
 */

class ubloxNMEA : public NMEAGPS
{
    ubloxNMEA( const ubloxNMEA & );

public:

    ubloxNMEA() {};

    /** ublox proprietary NMEA message types. */
    enum pubx_msg_t {
        PUBX_00 = NMEA_LAST_MSG+1,
        PUBX_04 = PUBX_00+4
    };
    static const nmea_msg_t PUBX_FIRST_MSG = (nmea_msg_t) PUBX_00;
    static const nmea_msg_t PUBX_LAST_MSG  = (nmea_msg_t) PUBX_04;

protected:
    static const msg_table_t ublox_msg_table __PROGMEM;

    const msg_table_t *msg_table() const { return &ublox_msg_table; };

    bool parseField( char chr );
    bool parseFix( char chr );
} __attribute__((packed));

#endif
