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
 * Enable/disable tracking the current satellite array.
 */

#define NMEAGPS_PARSE_SATELLITES

#ifdef NMEAGPS_PARSE_SATELLITES
#ifndef GPS_FIX_SATELLITES
#error GPS_FIX_SATELLITES must be defined in GPSfix.h!
#endif
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
    //  could change at any time.

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
      uint8_t offset;
      const msg_table_t *previous;
      uint8_t size;
      const char * const *table;
    };

    static const char * const std_nmea[] __PROGMEM;
    static const uint8_t std_nmea_size;
    static const msg_table_t nmea_msg_table __PROGMEM;

    NMEAGPS_VIRTUAL const msg_table_t *msg_table() const { return &nmea_msg_table; };

    /*
     * Use the list of tables to recognize an NMEA sentence type.
     */
    decode_t parseCommand( char c );

    /*
     * Depending on the NMEA sentence type, parse one field of the expected type.
     */
    NMEAGPS_VIRTUAL bool parseField( char chr );

    /*
     *  Helper macro for parsing a field in a message field switch statement.
     */
#define PARSE_FIELD(i,f) case i: return parse##f( chr );

    /*
     * Macros for parsing the primary field types into /fix/ members.
     */

    // Optional TIME    -----------------------
#ifdef GPS_FIX_TIME
#define CASE_TIME(i) PARSE_FIELD(i,Time)
    bool parseTime( char chr );
#else
#define CASE_TIME(i)
#endif

    // Optional DATE    -----------------------
#ifdef GPS_FIX_DATE
#define CASE_DATE(i) PARSE_FIELD(i,DDMMYY)
    bool parseDDMMYY( char chr );
#else
#define CASE_DATE(i)
#endif

    // Required STATUS    -----------------------
#define CASE_FIX(i) PARSE_FIELD(i,Fix)
    bool parseFix( char chr );

    // Optional LOCATION    -----------------------
#ifdef GPS_FIX_LOCATION
#define CASE_LOC(i) PARSE_FIELD(i,Lat) \
PARSE_FIELD(i+1,NS); \
PARSE_FIELD(i+2,Lon); \
PARSE_FIELD(i+3,EW);

    bool parseDDDMM( int32_t & val, char chr );

    bool parseLat( char chr )
    {
      return parseDDDMM( m_fix.lat, chr );
    }

    bool parseNS( char chr )
    {
      if (chr == 'S')
        m_fix.lat = -m_fix.lat;
      return true;
    }

    bool parseLon( char chr )
    {
      return parseDDDMM( m_fix.lon, chr );
    }

    bool parseEW( char chr )
    {
      if (chr == 'W')
        m_fix.lon = -m_fix.lon;
      m_fix.valid.location = true;
      return true;
    }
#else
#define CASE_LOC(i)
#endif

    // Optional SPEED    -----------------------
#ifdef GPS_FIX_SPEED
#define CASE_SPEED(i) PARSE_FIELD(i,Speed)

    bool parseSpeed( char chr )
    {
      return m_fix.valid.speed = parseFloat( m_fix.spd, chr, 3 );
    }
#else
#define CASE_SPEED(i)
#endif

    // Optional HEADING    -----------------------
#ifdef GPS_FIX_HEADING
#define CASE_HEADING(i) PARSE_FIELD(i,Heading)

    bool parseHeading( char chr )
    {
      return m_fix.valid.heading = parseFloat( m_fix.hdg, chr, 2 );
    }
#else
#define CASE_HEADING(i)
#endif

    // Optional ALTITUDE    -----------------------
#ifdef GPS_FIX_ALTITUDE
#define CASE_ALT(i) PARSE_FIELD(i,Alt)

    bool parseAlt(char chr )
    {
      return m_fix.valid.altitude = parseFloat( m_fix.alt, chr, 2 );
    }
#else
#define CASE_ALT(i)
#endif

    // Optional number of SATELLITES    -----------------------
#ifdef GPS_FIX_SATELLITES
#define CASE_SAT(i) PARSE_FIELD(i,Satellites)

    bool parseSatellites( char chr )
    {
      m_fix.valid.satellites = parseInt( m_fix.satellites, chr );
      return true;
    }

#else
#define CASE_SAT(i)
#endif

    // Optional SATELLITE VIEW array    -----------------------
#ifdef NMEAGPS_PARSE_SATELLITES
public:
    struct satellite_view_t
    {
      uint8_t  id;
      uint8_t  elevation; // 0..99 deg
      uint16_t azimuth;   // 0..359 deg
      uint8_t  snr;       // 0..99 dBHz
      bool     tracked;
    } __attribute__((packed));

    static const uint8_t MAX_SATELLITES = 32;
    satellite_view_t satellites[ MAX_SATELLITES ];

protected:
    uint8_t sat_index; // only used during parsing
#define CASE_SV_MSG_NO(i) \
  case i: if (parseInt( sat_index, chr ) && (chr == ',')) \
            sat_index = (sat_index - 1) * 4; \
          break;

#define CASE_SV_id(i) \
  case i: return parseInt( satellites[sat_index].id, chr );
#define CASE_SV_elev(i) \
  case i: return parseInt( satellites[sat_index].elevation, chr );
#define CASE_SV_az(i) \
  case i: return parseInt( satellites[sat_index].azimuth, chr );
#define SV_snr(c) \
  c: \
    if (chr == ',') { \
      sat_index++; \
      satellites[sat_index].tracked = (chrCount == 0); \
    } else \
      parseInt( satellites[sat_index].snr, chr ); \
    break;

#else
#define CASE_SV_MSG_NO(i)
#define CASE_SV_id(i)
#define CASE_SV_elev(i)
#define CASE_SV_az(i)
#define CASE_SV_snr(i)
#endif

    // Optional Horizontal Dilution of Precision    -----------------------
#ifdef GPS_FIX_HDOP
#define CASE_HDOP(i) PARSE_FIELD(i,HDOP)

    bool parseHDOP( char chr )
    {
      m_fix.valid.hdop = parseFloat( m_fix.hdop, chr, 3 );
      return true;
    }
#else
#define CASE_HDOP(i)
#endif

    // Optional Vertical Dilution of Precision    -----------------------
#ifdef GPS_FIX_VDOP
#define CASE_VDOP(i) PARSE_FIELD(i,VDOP)

    bool parseVDOP( char chr )
    {
      m_fix.valid.vdop = parseFloat( m_fix.vdop, chr, 3 );
      return true;
    }
#else
#define CASE_VDOP(i)
#endif

    // Optional Position Dilution of Precision    -----------------------
#ifdef GPS_FIX_PDOP
#define CASE_PDOP(i) PARSE_FIELD(i,PDOP)

    bool parsePDOP( char chr )
    {
      m_fix.valid.pdop = parseFloat( m_fix.pdop, chr, 3 );
      return true;
    }
#else
#define CASE_PDOP(i)
#endif

    // Optional Latitude error    -----------------------
#ifdef GPS_FIX_LAT_ERR
#define CASE_LAT_ERR(i) PARSE_FIELD(i,_lat_err)

    bool parse_lat_err( char chr )
    {
      m_fix.valid.lat_err = parseFloat( m_fix.lat_err_cm, chr, 2 );
      return true;
    }
#else
#define CASE_LAT_ERR(i)
#endif

    // Optional Longitude error    -----------------------
#ifdef GPS_FIX_LON_ERR
#define CASE_LON_ERR(i) PARSE_FIELD(i,_lon_err)

    bool parse_lon_err( char chr )
    {
      m_fix.valid.lon_err = parseFloat( m_fix.lon_err_cm, chr, 2 );
      return true;
    }
#else
#define CASE_LON_ERR(i)
#endif

    // Optional Altitude error    -----------------------
#ifdef GPS_FIX_ALT_ERR
#define CASE_ALT_ERR(i) PARSE_FIELD(i,_alt_err)

    bool parse_alt_err( char chr )
    {
      m_fix.valid.alt_err = parseFloat( m_fix.alt_err_cm, chr, 2 );
      return true;
    }
#else
#define CASE_ALT_ERR(i)
#endif

    // Helper method for parsing floating-point numbers into a /whole_frac/
    bool parseFloat( gps_fix::whole_frac & val, char chr, uint8_t max_decimal );

    // Helper method for parsing floating-point numbers into a uint16_t
    bool parseFloat( uint16_t & val, char chr, uint8_t max_decimal );

    // Helper method for parsing integers
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
