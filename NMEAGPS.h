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

//------------------------------------------------------
// Enable/disable the parsing of specific sentences.
//
// Configuring out a sentence prevents its fields from being parsed.
// However, the sentence type will still be recognized by /decode/ and 
// stored in member /nmeaMessage/.  No valid flags would be available.
//
// Only RMC and ZDA contain date information.  Other
// sentences contain time information.  Both date and time are 
// required if you will be doing time_t-to-clock_t operations.

#define NMEAGPS_PARSE_GGA
//#define NMEAGPS_PARSE_GLL
//#define NMEAGPS_PARSE_GSA
//#define NMEAGPS_PARSE_GSV
//#define NMEAGPS_PARSE_GST
#define NMEAGPS_PARSE_RMC
//#define NMEAGPS_PARSE_VTG
//#define NMEAGPS_PARSE_ZDA

//------------------------------------------------------
// Enable/disable the talker ID and manufacturer ID processing.
//
// First, some background information.  There are two kinds of NMEA sentences:
//
// 1. Standard NMEA sentences begin with "$ttccc", where
//      "tt" is the talker ID, and
//      "ccc" is the variable-length sentence type (i.e., command).
//
//    For example, "$GPGLL,..." is a GLL sentence (Geographic Lat/Long) 
//    transmitted by talker "GP".  This is the most common talker ID.  Some
//    devices may report "$GNGLL,..." when a mix of GPS and non-GPS
//    satellites have been used to determine the GLL data.
//
// 2. Proprietary NMEA sentences (i.e., those unique to a particular
//    manufacturer) begin with "$Pmmmccc", where
//      "P" is the NMEA-defined prefix indicator for proprietary messages,
//      "mmm" is the 3-character manufacturer ID, and
//      "ccc" is the variable-length sentence type (it can be empty).
//
// No validation of manufacturer ID and talker ID is performed in this
// base class.  For example, although "GP" is a common talker ID, it is not
// guaranteed to be transmitted by your particular device, and it IS NOT
// REQUIRED.  If you need validation of these IDs, or you need to use the
// extra information provided by some devices, you have two independent
// options:
//
// 1. Enable SAVING the ID: When /decode/ returns DECODE_COMPLETED, the
// /talker_id/ and/or /mfr_id/ members will contain ID bytes.  The entire
// sentence will be parsed, perhaps modifying members of /fix/.  You should
// enable one or both IDs if you want the information in all sentences *and*
// you also want to know the ID bytes.  This add two bytes of RAM for the
// talker ID, and 3 bytes of RAM for the manufacturer ID.
//
// 2. Enable PARSING the ID:  The virtual /parse_talker_id/ and
// /parse_mfr_id/ will receive each ID character as it is received.  If it
// is not a valid ID, return /false/ to abort processing the rest of the
// sentence.  No CPU time will be wasted on the invalid sentence, and no
// /fix/ members will be modified.  You should enable this if you want to
// ignore some IDs.  You must override /parse_talker_id/ and/or
// /parse_mfr_id/ in a derived class.
//

//#define NMEAGPS_SAVE_TALKER_ID
//#define NMEAGPS_PARSE_TALKER_ID

//#define NMEAGPS_SAVE_MFR_ID
//#define NMEAGPS_PARSE_MFR_ID

//------------------------------------------------------
// Enable/disable tracking the current satellite array and,
// optionally, all the info for each satellite.
//

//#define NMEAGPS_PARSE_SATELLITES
//#define NMEAGPS_PARSE_SATELLITE_INFO
#define NMEAGPS_MAX_SATELLITES (20)

#ifdef NMEAGPS_PARSE_SATELLITES
#ifndef GPS_FIX_SATELLITES
#error GPS_FIX_SATELLITES must be defined in GPSfix.h!
#endif
#endif

#if defined(NMEAGPS_PARSE_SATELLITE_INFO) & \
    !defined(NMEAGPS_PARSE_SATELLITES)
#error NMEAGPS_PARSE_SATELLITES must be defined!
#endif

//------------------------------------------------------
// Enable/disable accumulating fix data across sentences.
//
// If not defined, the fix will contain data from only the last decoded sentence.
//
// If defined, the fix will contain data from all received sentences.  Each
// fix member will contain the last value received from any sentence that
// contains that information.  This means that fix members may contain
// information from different time intervals (i.e., they are not coherent).
//
// ALSO NOTE:  If a received sentence is rejected for any reason (e.g., CRC
//   error), all the values are suspect.  The fix will be cleared; no members
//   will be valid until new sentences are received and accepted.
//
//   This is an application tradeoff between keeping a merged copy of received
//   fix data (more RAM) vs. accommodating "gaps" in fix data (more code).
//
// SEE ALSO: NMEAfused.ino and NMEAcoherent.ino

#define NMEAGPS_ACCUMULATE_FIX

#ifdef NMEAGPS_ACCUMULATE_FIX

// When accumulating, nothing is done to the fix at the beginning of every sentence
#define NMEAGPS_INIT_FIX(m)

// ...but we invalidate one part when it starts to get parsed.  It *may* get
// validated when the parsing is finished.
#define NMEAGPS_INVALIDATE(m) m_fix.valid.m = false

#else

// When NOT accumulating, invalidate the entire fix at the beginning of every sentence
#define NMEAGPS_INIT_FIX(m) m.valid.init()

// ...so the individual parts do not need to be invalidated as they are parsed
#define NMEAGPS_INVALIDATE(m)

#endif


//------------------------------------------------------
// Enable/disable gathering interface statistics:
// CRC errors and number of sentences received
//
//#define NMEAGPS_STATS

//------------------------------------------------------
// Configuration item for allowing derived types of NMEAGPS.
// If you derive classes from NMEAGPS, you *must* define NMEAGPS_DERIVED_TYPES.
// If not defined, virtuals are not used, with a slight size (2 bytes) and 
// execution time savings.
//
#define NMEAGPS_DERIVED_TYPES

#ifdef NMEAGPS_DERIVED_TYPES
#define NMEAGPS_VIRTUAL virtual
#else
#define NMEAGPS_VIRTUAL
#endif

#if (defined(NMEAGPS_PARSE_TALKER_ID) | defined(NMEAGPS_PARSE_MFR_ID)) &  \
           !defined(NMEAGPS_DERIVED_TYPES)
#error You must define NMEAGPS_DERIVED_TYPES in NMEAGPS.h in order to parse talker ids!
#endif

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
    uint8_t      crc;            // accumulated CRC in the sentence
    uint8_t      fieldIndex;     // index of current field in the sentence
    uint8_t      chrCount;       // index of current character in current field
    uint8_t      decimal;        // digits received after the decimal point
    struct {
      bool       negative    :1; // field had a leading '-'
      bool       safe        :1; // fix is safe to access
      bool       comma_needed:1; // field needs a comma to finish parsing
      bool       group_valid :1; // multi-field group valid
      bool       proprietary :1; // receiving proprietary message
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
#ifdef NMEAGPS_STATS
      statistics.ok         = 0;
      statistics.crc_errors = 0;
#endif
      rxState               = NMEA_IDLE;
      safe                  = true;
    };

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

    /**
     * Most recent NMEA sentence type received.
     */
    enum nmea_msg_t nmeaMessage:8;
    
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
        uint32_t ok;         // count of successfully parsed sentences
        uint32_t crc_errors; // count of CRC errors
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
    void sentenceBegin       ();
    void sentenceOk          ();
    void sentenceInvalid     ();
    void sentenceUnrecognized();

protected:
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

#ifdef NMEAGPS_PARSE_TALKER_ID
    NMEAGPS_VIRTUAL bool parseTalkerID( char chr ) { return true; };
#endif

#ifdef NMEAGPS_PARSE_MFR_ID
    NMEAGPS_VIRTUAL bool parseMfrID( char chr ) { return true; };
#endif

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
      uint8_t  snr    :7; // 0..99 dBHz
      bool     tracked:1;
#endif
    } __attribute__((packed));

    satellite_view_t satellites[ NMEAGPS_MAX_SATELLITES ];
    uint8_t sat_count;

    bool satellites_valid() const { return (sat_count >= m_fix.satellites); }
protected:
#endif

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

} __attribute__((packed));

#endif
