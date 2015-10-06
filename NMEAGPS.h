#ifndef NMEAGPS_H
#define NMEAGPS_H

//------------------------------------------------------
// @file NMEAGPS.h
// @version 2.1
//
// @section License
// Copyright (C) 2014, SlashDevin
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

#include <avr/pgmspace.h>

#include "GPSfix.h"
#include "NMEAGPS_cfg.h"

//------------------------------------------------------
//
// NMEA 0183 Parser for generic GPS Modules.  As bytes are received from
// the device, they affect the internal FSM and set various members
// of the current /fix/.
//
// @section Limitations
// 1) Only these NMEA messages are parsed:
//      GGA, GLL, GSA, GST, GSV, RMC, VTG, and ZDA.
// 2) The current `fix` is only safe to access _after_ the complete message 
// is parsed and _before_ the next message begins to affect the members. 
// If you access `fix` at any other time, /is_safe()/ must be checked. 
// Otherwise, you should make a copy of `fix` after a sentence has been
// completely DECODED.
//

class NMEAGPS
{
    NMEAGPS( const NMEAGPS & );

public:

    /**
     * Constructor
     */
    NMEAGPS();

    /**
     * Return type for /decode/.  As bytes are processed, they can be
     * categorized as INVALID (not part of this protocol), OK (accepted),
     * or COMPLETED (end-of-message).
     */
    enum decode_t { DECODE_CHR_INVALID, DECODE_CHR_OK, DECODE_COMPLETED };

    /**
     * Process one character of an NMEA GPS sentence.  The internal state 
     * machine tracks what part of the sentence has been received.  As the
     * tracks what part of the sentence has been received so far.  As the
     * sentence is received, members of the /fix/ structure are updated.  
     * @return DECODE_COMPLETED when a sentence has been completely received.
     */
    NMEAGPS_VIRTUAL decode_t decode( char c );

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

    //  Convert a nmea_msg_t to a PROGMEM string.
    //    Useful for printing the sentence type instead of a number.
    const char *string_for( nmea_msg_t msg ) const;

    /**
     * Most recent NMEA sentence type received.
     */
    enum nmea_msg_t nmeaMessage NEOGPS_BF(8);

    //  Storage for IDs, if configured

    #ifdef NMEAGPS_SAVE_TALKER_ID
      char talker_id[2];
    #endif

    #ifdef NMEAGPS_SAVE_MFR_ID
      char mfr_id[3];
    #endif

    //  Current fix accessor.
    //  /fix/ will be constantly changing as characters are received.
    //  For example, fix().longitude() may return nonsense data if
    //  characters for that field are currently being processed in /decode/.
    //  /is_safe/ *must* be checked before accessing members of /fix/.
    //  If you need access to the current /fix/ at any time, you must
    //  take a snapshot while it is_safe, and then use the snapshot
    //  later.

    gps_fix & fix() { return m_fix; };

    //  Determine whether the members of /fix/ are "currently" safe.
    //  It will return true when a complete sentence and the CRC characters 
    //  have been received (or after a CR if no CRC is present).
    //  It will return false after a sentence's command and comma
    //  have been received (e.g., "$GPGGA,").
    //  If NMEAGPS processes characters in UART interrupts, /is_safe/
    //  could change at any time (i.e., it would be /volatile/).

    bool is_safe() const { return safe; }

    //  Notes regarding a volatile /fix/:
    //
    //  If an NMEAGPS instance is fed characters from a non-interrupt
    //  context, the following method is safe:
    //
    //  void loop()
    //  {
    //    while (uart.available()) {
    //      if (gps.decode( uart.read() ) == DECODE_COMPLETED) {
    //        // Got something new!  Access only valid members here and/or...
    //
    //        // ...save a snapshot for later
    //        safe_fix = gps.fix();
    //      }
    //    }
    //    // Access valid members of /safe_fix/ anywhere, any time.
    //  }

    #ifdef NMEAGPS_STATS
      struct statistics_t {
          uint32_t ok;         // count of successfully parsed sentences
          uint32_t crc_errors; // count of CRC errors
          uint32_t chars;
          void init()
            {
              ok         = 0L;
              crc_errors = 0L;
              chars      = 0L;
            }
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

    // Set all parsed data (fix(), satellite info, etc.) to initial values.
    void data_init()
    {
      fix().init();

      #ifdef NMEAGPS_PARSE_SATELLITES
        sat_count = 0;
      #endif
    }

    // Reset the parsing process.
    //   This is used internally after a CS error, or could be used 
    //   externally to abort processing if it has been too long 
    //   since any data was received.
    void reset()
    {
      rxState = NMEA_IDLE;
      safe    = true;
    }

protected:
    //  Current fix
    gps_fix m_fix;

    /**
     * Current parser state
     */
    uint8_t      crc;            // accumulated CRC in the sentence
    uint8_t      fieldIndex;     // index of current field in the sentence
    uint8_t      chrCount;       // index of current character in current field
    uint8_t      decimal;        // digits received after the decimal point
    struct {
      bool       negative      NEOGPS_BF(1); // field had a leading '-'
      bool       safe          NEOGPS_BF(1); // fix is safe to access
      bool       _comma_needed NEOGPS_BF(1); // field needs a comma to finish parsing
      bool       group_valid   NEOGPS_BF(1); // multi-field group valid
      bool       proprietary   NEOGPS_BF(1); // receiving proprietary message
    } NEOGPS_PACKED;

    bool comma_needed()
    {
      #ifdef NMEAGPS_COMMA_NEEDED
        return _comma_needed;
      #else
        return false;
      #endif
    }

    void comma_needed( bool value )
    {
      #ifdef NMEAGPS_COMMA_NEEDED
        _comma_needed = value;
      #endif
    }

    /*
     * Internal FSM states
     */
    enum rxState_t {
        NMEA_IDLE,             // Waiting for initial '$'
        NMEA_RECEIVING_HEADER, // Parsing sentence type field
        NMEA_RECEIVING_DATA,   // Parsing fields up to the terminating '*'
        NMEA_RECEIVING_CRC     // Receiving two-byte transmitted CRC
    };
    static const uint8_t NMEA_FIRST_STATE = NMEA_IDLE;
    static const uint8_t NMEA_LAST_STATE  = NMEA_RECEIVING_CRC;

    rxState_t rxState NEOGPS_BF(8);

    /*
     * Table entry for NMEA sentence type string and its offset
     * in enumerated nmea_msg_t.  Proprietary sentences can be implemented
     * in derived classes by adding a second table.  Additional tables
     * can be singly-linked through the /previous/ member.  The instantiated
     * class's table is the head, and should be returned by the derived
     * /msg_table/ function.  Tables should be sorted alphabetically.
     */
    struct msg_table_t {
      uint8_t             offset;  // nmea_msg_t enum starting value
      const msg_table_t  *previous;
      uint8_t             size;    // number of entries in table array
      const char * const *table;   // array of NMEA sentence strings
    };

    static const msg_table_t  nmea_msg_table __PROGMEM;

    NMEAGPS_VIRTUAL const msg_table_t *msg_table() const
      { return &nmea_msg_table; };

    /*
     *  These virtual methods can accept or reject the talker ID (for standard sentences) 
     *    or the manufacturer ID (for proprietary sentences).
     *  The default is to accept *all* IDs.
     *  Override them if you want to reject certain IDs, or you want
     *  to handle COMPLETED sentences from certain IDs differently.
     */
    #ifdef NMEAGPS_PARSE_TALKER_ID
      NMEAGPS_VIRTUAL bool parseTalkerID( char chr ) { return true; };
    #endif

    #ifdef NMEAGPS_PARSE_MFR_ID
      NMEAGPS_VIRTUAL bool parseMfrID( char chr ) { return true; };
    #endif

    /*
     * Try to recognize an NMEA sentence type, after the IDs have been accepted.
     */
    decode_t parseCommand( char c );

    /*
     * Parse various NMEA sentences
     */
    bool parseGGA( char chr );
    bool parseGLL( char chr );
    bool parseGSA( char chr );
    bool parseGST( char chr );
    bool parseGSV( char chr );
    bool parseRMC( char chr );
    bool parseVTG( char chr );
    bool parseZDA( char chr );

    /*
     * Depending on the NMEA sentence type, parse one field of an expected type.
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

public:
    // Optional SATELLITE VIEW array    -----------------------
    #ifdef NMEAGPS_PARSE_SATELLITES
      struct satellite_view_t
      {
        uint8_t    id;
        #ifdef NMEAGPS_PARSE_SATELLITE_INFO
          uint8_t  elevation; // 0..99 deg
          uint16_t azimuth;   // 0..359 deg
          uint8_t  snr     NEOGPS_BF(7); // 0..99 dBHz
          bool     tracked NEOGPS_BF(1);
        #endif
      } NEOGPS_PACKED;

      satellite_view_t satellites[ NMEAGPS_MAX_SATELLITES ];
      uint8_t sat_count;

      bool satellites_valid() const { return (sat_count >= m_fix.satellites); }
    #endif

protected:

    /**
     * Parse floating-point numbers into a /whole_frac/
     * @return true when the value is fully populated.
     */
    bool parseFloat( gps_fix::whole_frac & val, char chr, uint8_t max_decimal );

    /**
     * Parse floating-point numbers into a uint16_t
     * @return true when the value is fully populated.
     */
    bool parseFloat( uint16_t & val, char chr, uint8_t max_decimal );

    /**
     * Parse NMEA lat/lon dddmm.mmmm degrees
     * @return true.
     */
    bool parseDDDMM( int32_t & val, char chr );

    /*
     * Parse integer into 8-bit int
     * @return true when non-empty value
     */
    bool parseInt( uint8_t &val, uint8_t chr )
    {
      bool is_comma = (chr == ',');
      if (chrCount == 0) {
        if (is_comma)
          return false; // empty field!
        val = (chr - '0');
      } else if (!is_comma)
        val = (val*10) + (chr - '0');
      return true;
    }

    /*
     * Parse integer into 16-bit int
     * @return true when non-empty value
     */
    bool parseInt( uint16_t &val, uint8_t chr )
    {
      bool is_comma = (chr == ',');
      if (chrCount == 0) {
        if (is_comma)
          return false; // empty field!
        val = (chr - '0');
      } else if (!is_comma)
        val = (val*10) + (chr - '0');
      return true;
    }

private:
    void sentenceBegin       ();
    void sentenceOk          ();
    void sentenceInvalid     ();
    void sentenceUnrecognized();
    void headerReceived      ();

} NEOGPS_PACKED;

#endif
