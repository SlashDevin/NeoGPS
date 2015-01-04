#ifndef NMEAGPS_H
#define NMEAGPS_H

/**
 * @file NMEAGPS.h
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

#include <avr/pgmspace.h>

typedef PGM_P str_P;
#define __PROGMEM PROGMEM
class Stream;

#include "GPSfix.h"

/**
 * Enable/disable the parsing of specific sentences.
 *
 * Configuring out a sentence prevents its fields from being parsed.
 * However, the sentence type will still be recognized by /decode/ and 
 * stored in member /nmeaMessage/.  No valid flags would be available.
 *
 * Only RMC and ZDA contain date information.  Other
 * sentences contain time information.  Both date and time are 
 * required if you will be doing tmElements_t-to-time_t operations.
 */

#define NMEAGPS_PARSE_GGA
//#define NMEAGPS_PARSE_GLL
//#define NMEAGPS_PARSE_GSA
//#define NMEAGPS_PARSE_GSV
//#define NMEAGPS_PARSE_GST
#define NMEAGPS_PARSE_RMC
//#define NMEAGPS_PARSE_VTG
//#define NMEAGPS_PARSE_ZDA

/**
 * Configuration item for allowing derived types of NMEAGPS.
 * If defined, virtuals are used, with a slight size (2 bytes) and time penalty.
 * If you derive classes from NMEAGPS, you *must* define NMEAGPS_DERIVED_TYPES.
 */

#define NMEAGPS_DERIVED_TYPES
#ifdef NMEAGPS_DERIVED_TYPES
#define NMEAGPS_VIRTUAL virtual
#else
#define NMEAGPS_VIRTUAL
#endif

/**
 * Enable/disable tracking the current satellite array and,
 * optionally, all the info for each satellite.
 */

//#define NMEAGPS_PARSE_SATELLITES
//#define NMEAGPS_PARSE_SATELLITE_INFO

#ifdef NMEAGPS_PARSE_SATELLITES
#ifndef GPS_FIX_SATELLITES
#error GPS_FIX_SATELLITES must be defined in GPSfix.h!
#endif
#endif

#if defined(NMEAGPS_PARSE_SATELLITE_INFO) & \
    !defined(NMEAGPS_PARSE_SATELLITES)
#error NMEAGPS_PARSE_SATELLITES must be defined!
#endif

/**
 *
 * NMEA 0183 Parser for generic GPS Modules.  As bytes are received from
 * the device, they affect the internal FSM and set various members
 * of the current /fix/.
 *
 * @section Limitations
 * 1) Only NMEA messages of types GGA, GLL, RMC, VTG, ZDA are parsed.
 * 2) The current `fix` is only coherent _after_ the complete message is 
 * parsed and _before_ the next message begins to affect the members. 
 * /is_coherent()/ should be checked before accessing any members of /fix/.
 *
 **/

class NMEAGPS
{
    NMEAGPS( const NMEAGPS & );

public:
    /** NMEA standard message types. */
    enum nmea_msg_t {
        NMEA_UNKNOWN,
        NMEA_GGA,
        NMEA_GLL,
        NMEA_GSA,
        NMEA_GST,
        NMEA_GSV,
        NMEA_RMC,
        NMEA_VTG,
        NMEA_ZDA,
    };
    static const nmea_msg_t NMEA_FIRST_MSG = NMEA_GGA;
    static const nmea_msg_t NMEA_LAST_MSG  = NMEA_ZDA;

protected:
    //  Current fix
    gps_fix m_fix;

    /**
     * Current parser state
     */
    uint8_t         crc;        // accumulated CRC in the sentence
    uint8_t         fieldIndex; // index of current field in the sentence
    uint8_t         chrCount;   // index of current character in current field
    uint8_t         decimal;    // digits received after the decimal point
    struct {
      bool            negative:1; // field had a leading '-'
      bool            coherent:1; // fix is coherent
    } __attribute__((packed));

    /*
     * Internal FSM states
     */
    enum rxState_t {
        NMEA_IDLE,           // Waiting for initial '$'
        NMEA_RECEIVING_DATA, // Parsing fields up to the terminating '*'
        NMEA_RECEIVING_CRC   // Receiving two-byte transmitted CRC
    };
    static const uint8_t NMEA_FIRST_STATE = NMEA_IDLE;
    static const uint8_t NMEA_LAST_STATE  = NMEA_RECEIVING_CRC;

    rxState_t rxState:8;

public:

    /**
     * Constructor
     */
    NMEAGPS()
    {
      rxState = NMEA_IDLE;
      coherent = true;
    };

    /**
     * Process one character of an NMEA GPS sentence.  The  internal state machine
     * tracks what part of the sentence has been received so far.  As the
     * sentence is received, members of the /fix/ structure are updated.  
     * @return true when new /fix/ data is available and coherent.
     */
    enum decode_t { DECODE_CHR_INVALID, DECODE_CHR_OK, DECODE_COMPLETED };

    NMEAGPS_VIRTUAL decode_t decode( char c );

    /**
     * Most recent NMEA sentence type received.
     */
    enum nmea_msg_t nmeaMessage:8;
    
    //  Current fix accessor.
    //  /fix/ will be constantly changing as characters are received.
    //  For example, fix().longitude() may return nonsense data if
    //  characters for that field are currently being processed in /decode/.
    //  /is_coherent/ *must* be checked before accessing members of /fix/.
    //  If you need access to the current /fix/ at any time, you must
    //  take a snapshot while it is_coherent, and then use the snapshot
    //  later.

    const struct gps_fix & fix() const { return m_fix; };

    //  Determine whether the members of /fix/ are "currently" coherent.
    //  It will return true when a complete sentence and the CRC characters 
    //  have been received (or after a CR if no CRC is present).
    //  It will return false after a sentence's command and comma
    //  have been received (e.g., "$GPGGA,").
    //  If NMEAGPS processes characters in UART interrupts, /is_coherent/
    //  could change at any time (i.e., it would be /volatile/).

    bool is_coherent() const { return coherent; }

    //  Notes regarding a volatile /fix/:
    //
    //  If an NMEAGPS instance is fed characters from a non-interrupt
    //  context, the following method is safe:
    //
    //  void loop()
    //  {
    //    while (uart.available()) {
    //      if (gps.decode( uart.getchar() ) == DECODE_COMPLETED) {
    //        // Got something new!  Access only valid members here and/or...
    //
    //        // ...save a snapshot for later
    //        safe_fix = gps.fix();
    //      }
    //    }
    //    // Access valid members of /safe_fix/ anywhere, any time.
    //  }

#ifdef NMEAGPS_STATS
    /**
     * Internal GPS parser statistics.
     */
    struct {
        uint8_t  parser_ok;     // count of successfully parsed packets
        uint8_t  parser_crcerr; // count of CRC errors
    } statistics;
#endif

    /**
     * Request the specified NMEA sentence.  Not all devices will respond.
     */

    static void poll( Stream *device, nmea_msg_t msg );

    /**
     * Send a message to the GPS device.
     * The '$' is optional, and the '*' and CS will be added automatically.
     */

    static void send( Stream *device, const char *msg );
    static void send_P( Stream *device, str_P msg );

private:
    void rxBegin();
    void rxEnd( bool ok );

protected:
    /*
     * Table entry for NMEA sentence type string and its offset
     * in enumerated nmea_msg_t.  Proprietary sentences can be implemented
     * in derived classes by adding a second table.  Additional tables
     * can be singly-linked through the /previous/ member.  The instantiated
     * class's table is the head, and should be returned by the derived
     * /msg_table/ function.  Tables should be sorted by the commonality
     * of the starting characters: alphabetical would work but is not strictly
     * required.
     */
    struct msg_table_t {
      uint8_t             offset;  // nmea_msg_t enum starting value
      const msg_table_t  *previous;
      uint8_t             size;    // number of entries in table array
      const char * const *table;   // array of NMEA sentence strings
    };

    static const char * const std_nmea[] __PROGMEM;
    static const uint8_t      std_nmea_size;
    static const msg_table_t  nmea_msg_table __PROGMEM;

    NMEAGPS_VIRTUAL const msg_table_t *msg_table() const
      { return &nmea_msg_table; };

    /*
     * Use the list of tables to recognize an NMEA sentence type.
     */
    decode_t parseCommand( char c );

    /*
     * Depending on the NMEA sentence type, parse one field of the expected type.
     */
    NMEAGPS_VIRTUAL bool parseField( char chr );

    /*
     * Parse the primary NMEA field types into /fix/ members.
     */

    bool parseFix( char chr ); // aka STATUS or MODE
    bool parseTime( char chr );
    bool parseDDMMYY( char chr );
    bool parseLat( char chr );
    bool parseNS( char chr );
    bool parseLon( char chr );
    bool parseEW( char chr );
    bool parseSpeed( char chr );
    bool parseHeading( char chr );
    bool parseAlt(char chr );
    bool parseHDOP( char chr );
    bool parseVDOP( char chr );
    bool parsePDOP( char chr );
    bool parse_lat_err( char chr );
    bool parse_lon_err( char chr );
    bool parse_alt_err( char chr );
    bool parseSatellites( char chr );

    // Helper macro for parsing the 4 consecutive fields of a location
#define PARSE_FIELD(i,f) case i: return parse##f( chr );
#define PARSE_LOC(i) PARSE_FIELD(i,Lat) \
PARSE_FIELD(i+1,NS); \
PARSE_FIELD(i+2,Lon); \
PARSE_FIELD(i+3,EW);

    // Optional SATELLITE VIEW array    -----------------------
#ifdef NMEAGPS_PARSE_SATELLITES
public:
    struct satellite_view_t
    {
      uint8_t  id;
#ifdef NMEAGPS_PARSE_SATELLITE_INFO
      uint8_t  elevation; // 0..99 deg
      uint16_t azimuth;   // 0..359 deg
      uint8_t  snr;       // 0..99 dBHz
      bool     tracked;
#endif
    } __attribute__((packed));

    static const uint8_t MAX_SATELLITES = 20;
    satellite_view_t satellites[ MAX_SATELLITES ];

protected:
    uint8_t sat_index; // only used during parsing

#endif

    // Parse floating-point numbers into a /whole_frac/
    bool parseFloat( gps_fix::whole_frac & val, char chr, uint8_t max_decimal );

    // Parse floating-point numbers into a uint16_t
    bool parseFloat( uint16_t & val, char chr, uint8_t max_decimal );

    // Parse NMEA lat/lon dddmm.mmmm degrees
    bool parseDDDMM( int32_t & val, char chr );

    // Parse integer into 8-bit int
    bool parseInt( uint8_t &val, uint8_t chr )
    {
      bool is_comma = (chr == ',');
      if (chrCount == 0) {
        if (is_comma)
          return false;
        val = (chr - '0');
      } else if (!is_comma)
        val = (val*10) + (chr - '0');
      return true;
    }

    // Parse integer into 16-bit int
    bool parseInt( uint16_t &val, uint8_t chr )
    {
      bool is_comma = (chr == ',');
      if (chrCount == 0) {
        if (is_comma)
          return false;
        val = (chr - '0');
      } else if (!is_comma)
        val = (val*10) + (chr - '0');
      return true;
    }
} __attribute__((packed));

#endif
