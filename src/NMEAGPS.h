#ifndef NMEAGPS_H
#define NMEAGPS_H

//------------------------------------------------------
// @file NMEAGPS.h
// @version 4.1.0
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

#include "CosaCompat.h"

#include <Arduino.h>
#ifdef __AVR__
  #include <avr/interrupt.h>
#endif

#include "GPSfix.h"
#include "NMEAGPS_cfg.h"

//------------------------------------------------------
//
// NMEA 0183 Parser for generic GPS Modules.
//
// As bytes are received from the device, they affect the 
// internal FSM and set various members of the current /fix/.  
// As multiple sentences are received, they are (optionally) 
// merged into a single fix.  When the last sentence in a time 
// interval (usually 1 second) is received, the fix is stored 
// in the (optional) buffer of fixes.
//
// Only these NMEA messages are parsed:
//      GGA, GLL, GSA, GST, GSV, RMC, VTG, and ZDA.

class NMEAGPS
{
    NMEAGPS & operator =( const NMEAGPS & );
    NMEAGPS( const NMEAGPS & );

public:

    NMEAGPS();
    
    //.......................................................................
    // NMEA standard message types (aka "sentences")

    enum nmea_msg_t {
        NMEA_UNKNOWN,

        #if defined(NMEAGPS_PARSE_GGA) | defined(NMEAGPS_RECOGNIZE_ALL)
          NMEA_GGA,
        #endif
        
        #if defined(NMEAGPS_PARSE_GLL) | defined(NMEAGPS_RECOGNIZE_ALL)
          NMEA_GLL,
        #endif
        
        #if defined(NMEAGPS_PARSE_GSA) | defined(NMEAGPS_RECOGNIZE_ALL)
          NMEA_GSA,
        #endif
        
        #if defined(NMEAGPS_PARSE_GST) | defined(NMEAGPS_RECOGNIZE_ALL)
          NMEA_GST,
        #endif
        
        #if defined(NMEAGPS_PARSE_GSV) | defined(NMEAGPS_RECOGNIZE_ALL)
          NMEA_GSV,
        #endif
        
        #if defined(NMEAGPS_PARSE_RMC) | defined(NMEAGPS_RECOGNIZE_ALL)
          NMEA_RMC,
        #endif
        
        #if defined(NMEAGPS_PARSE_VTG) | defined(NMEAGPS_RECOGNIZE_ALL)
          NMEA_VTG,
        #endif
        
        #if defined(NMEAGPS_PARSE_ZDA) | defined(NMEAGPS_RECOGNIZE_ALL)
          NMEA_ZDA,
        #endif
        
        NMEAMSG_END // a bookend that tells how many enums there were
      };

    CONST_CLASS_DATA nmea_msg_t NMEA_FIRST_MSG = (nmea_msg_t) (NMEA_UNKNOWN+1);
    CONST_CLASS_DATA nmea_msg_t NMEA_LAST_MSG  = (nmea_msg_t) (NMEAMSG_END-1);

    //=======================================================================
    // FIX-ORIENTED methods: available, read and handle
    //=======================================================================

    //.......................................................................
    // The available(...) functions return the number of *fixes* that
    //   are available to be "read" from the fix buffer.  The GPS port
    //   object is passed in so a char can be read if port.available().

    uint8_t available( Stream & port )
      {
        if (processing_style == PS_POLLING)
          while (port.available())
            handle( port.read() );
        return _available();
      }
    uint8_t available() const volatile { return _available(); };

    //.......................................................................
    // Return the next available fix.  When no more fixes 
    //   are available, it returns an empty fix.

    const gps_fix read();
    
    // The typical sketch should have something like this in loop():
    //
    //    while (gps.available( Serial1 )) {
    //      gps_fix fix = gps.read();
    //      if (fix.valid.location) {
    //         ...
    //      }
    //    }

    //.......................................................................
    // As characters are processed, they can be categorized as 
    // INVALID (not part of this protocol), OK (accepted),
    // or COMPLETED (end-of-message).

    enum decode_t { DECODE_CHR_INVALID, DECODE_CHR_OK, DECODE_COMPLETED };

    //.......................................................................
    //  Process one character, possibly saving a buffered fix.
    //    It implements merging and coherency.
    //    This can be called from an ISR.

    decode_t handle( uint8_t c );

    //=======================================================================
    // CHARACTER-ORIENTED "decode" method
    //=======================================================================
    //
    //    *** MOST APPLICATIONS SHOULD USE THE FIX-ORIENTED METHODS ***
    //
    //    Using this method is only necessary if you want finer control 
    //    on how fix information is filtered and merged.
    //
    // Process one character of an NMEA GPS sentence.  The internal state 
    // machine tracks what part of the sentence has been received.  As the
    // sentence is received, members of the /fix/ structure are updated.
    // This character-oriented method does not buffer any fixes, and
    // /read()/ will always return an empty fix.
    //
    // @return DECODE_COMPLETED when a sentence has been completely received.

    NMEAGPS_VIRTUAL decode_t decode( char c );

    //.......................................................................
    //  Convert a nmea_msg_t to a PROGMEM string.
    //    Useful for printing the sentence type instead of a number.
    //    This can return "UNK" if the message is not a valid number.
    
    const __FlashStringHelper *string_for( nmea_msg_t msg ) const;

    //.......................................................................
    // Most recent NMEA sentence type received.

    enum nmea_msg_t nmeaMessage NEOGPS_BF(8);

    //.......................................................................
    //  Storage for Talker and Manufacturer IDs, if configured

    #ifdef NMEAGPS_SAVE_TALKER_ID
      char talker_id[2];
    #endif

    #ifdef NMEAGPS_SAVE_MFR_ID
      char mfr_id[3];
    #endif

    //.......................................................................
    //  Current fix accessor.
    //    *** MOST APPLICATIONS SHOULD USE read() TO GET THE CURRENT FIX  ***
    //    /fix/ will be constantly changing as characters are received.
    //
    //  For example, fix().longitude() may return nonsense data if
    //  characters for that field are currently being processed in /decode/.

    gps_fix & fix() { return m_fix; };

    //  NOTE: /is_safe/ *must* be checked before accessing members of /fix/.
    //  If you need access to the current /fix/ at any time, you should
    //  use the FIX-ORIENTED methods.

    //.......................................................................
    //  Determine whether the members of /fix/ are "currently" safe.
    //  It will return true when a complete sentence and the CRC characters 
    //  have been received (or after a CR if no CRC is present).
    //  It will return false after a new sentence starts.

    bool is_safe() const volatile { return (rxState == NMEA_IDLE); }

    //  NOTE:  When INTERRUPT_PROCESSING is enabled, is_safe() 
    //  and fix() could change at any time (i.e., they should be 
    //  considered /volatile/).

    //.......................................................................

    #ifdef NMEAGPS_STATS
      struct statistics_t {
          uint32_t ok;     // count of successfully parsed sentences
          uint32_t errors; // NMEA checksum or other message errors
          uint32_t chars;
          void init()
            {
              ok     = 0L;
              errors = 0L;
              chars  = 0L;
            }
      } statistics;
    #endif

    //.......................................................................
    // Request the specified NMEA sentence.  Not all devices will respond.

    static void poll( Stream *device, nmea_msg_t msg );

    //.......................................................................
    // Send a message to the GPS device.
    // The '$' is optional, and the '*' and CS will be added automatically.

    static void send( Stream *device, const char *msg );
    static void send_P( Stream *device, const __FlashStringHelper *msg );

    //.......................................................................
    // Indicate that the next sentence should initialize the internal data.
    //    This is useful for coherency or custom filtering.
    
    bool intervalComplete() const { return _intervalComplete; }
    void intervalComplete( bool val ) { _intervalComplete = val; }

    //.......................................................................
    // Set all parsed data to initial values.

    void data_init()
    {
      fix().init();

      #ifdef NMEAGPS_PARSE_SATELLITES
        sat_count = 0;
      #endif
    }

    //.......................................................................
    // Reset the parsing process.
    //   This is used internally after a CS error, or could be used 
    //   externally to abort processing if it has been too long 
    //   since any data was received.

    void reset()
    {
      rxState = NMEA_IDLE;
    }

    //.......................................................................
    //  The OVERRUN flag is set whenever a fix is not read by the time
    //  the next update interval starts.  You must clear it when you
    //  detect the condition.

    bool overrun() const { return _overrun; }
    void overrun( bool val ) { _overrun = val; }

    //.......................................................................

    enum merging_t { NO_MERGING, EXPLICIT_MERGING, IMPLICIT_MERGING };
    static const merging_t
      merging = NMEAGPS_MERGING; // see NMEAGPS_cfg.h

    enum processing_style_t { PS_POLLING, PS_INTERRUPT };
    static const processing_style_t 
      processing_style = NMEAGPS_PROCESSING_STYLE;  // see NMEAGPS_cfg.h

    static const bool keepNewestFixes = NMEAGPS_KEEP_NEWEST_FIXES;

    //.......................................................................
    //  Control access to this object.  This preserves atomicity when
    //     the processing style is interrupt-driven.

    void lock() const
      {
        if (processing_style == PS_INTERRUPT)
          noInterrupts();
      }

    void unlock() const
      {
        if (processing_style == PS_INTERRUPT)
          interrupts();
      }

protected:
    //  Current fix
    gps_fix m_fix;

    // Current parser state
    uint8_t      crc;            // accumulated CRC in the sentence
    uint8_t      fieldIndex;     // index of current field in the sentence
    uint8_t      chrCount;       // index of current character in current field
    uint8_t      decimal;        // digits received after the decimal point
    struct {
      bool     negative          NEOGPS_BF(1); // field had a leading '-'
      bool     _comma_needed     NEOGPS_BF(1); // field needs a comma to finish parsing
      bool     group_valid       NEOGPS_BF(1); // multi-field group valid
      bool     _overrun          NEOGPS_BF(1); // an entire fix was dropped
      bool     _intervalComplete NEOGPS_BF(1); // automatically set after LAST received
      #if (NMEAGPS_FIX_MAX == 0)
        bool   _fixesAvailable   NEOGPS_BF(1);
      #endif
      #ifdef NMEAGPS_PARSE_PROPRIETARY
        bool   proprietary       NEOGPS_BF(1); // receiving proprietary message
      #endif
    } NEOGPS_PACKED;

    #ifdef NMEAGPS_PARSING_SCRATCHPAD
      union {
        uint32_t U4;
        uint16_t U2[2];
        uint8_t  U1[4];
      } scratchpad;
    #endif

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

    // Internal FSM states
    enum rxState_t {
        NMEA_IDLE,             // Waiting for initial '$'
        NMEA_RECEIVING_HEADER, // Parsing sentence type field
        NMEA_RECEIVING_DATA,   // Parsing fields up to the terminating '*'
        NMEA_RECEIVING_CRC     // Receiving two-byte transmitted CRC
    };
    CONST_CLASS_DATA uint8_t NMEA_FIRST_STATE = NMEA_IDLE;
    CONST_CLASS_DATA uint8_t NMEA_LAST_STATE  = NMEA_RECEIVING_CRC;

    rxState_t rxState NEOGPS_BF(8);

    //.......................................................................

    uint8_t _available() const volatile { return _fixesAvailable; };

    //.......................................................................
    //  Buffered fixes.

    #if (NMEAGPS_FIX_MAX > 0)
      gps_fix buffer[ NMEAGPS_FIX_MAX ]; // could be empty, see NMEAGPS_cfg.h
      uint8_t _fixesAvailable;
      uint8_t _firstFix;
      uint8_t _currentFix;
    #endif

    //.......................................................................
    //  Identify when an update interval is completed, according to the
    //  most recently-received sentence.  In this base class, it just
    //  looks at the nmeaMessage member.  Derived classes may have
    //  more complex, specific conditions.

    NMEAGPS_VIRTUAL bool intervalCompleted() const
      { return (nmeaMessage == LAST_SENTENCE_IN_INTERVAL); }
                               // see NMEAGPS_cfg.h

    //.......................................................................
    //  When a fix has been fully assembled from a batch of sentences, as
    //  determined by the configured merging technique and ending with the
    //  LAST_SENTENCE_IN_INTERVAL, it is stored in the (optional) buffer
    //  of fixes.  They are removed with /read()/.

    void storeFix();

    //.......................................................................
    // Table entry for NMEA sentence type string and its offset
    // in enumerated nmea_msg_t.  Proprietary sentences can be implemented
    // in derived classes by adding a second table.  Additional tables
    // can be singly-linked through the /previous/ member.  The instantiated
    // class's table is the head, and should be returned by the derived
    // /msg_table/ function.  Tables should be sorted alphabetically.

    struct msg_table_t {
      uint8_t             offset;  // nmea_msg_t enum starting value
      const msg_table_t  *previous;
      uint8_t             size;    // number of entries in table array
      const char * const *table;   // array of NMEA sentence strings
    };

    static const msg_table_t  nmea_msg_table __PROGMEM;

    NMEAGPS_VIRTUAL const msg_table_t *msg_table() const
      { return &nmea_msg_table; };

    //.......................................................................
    //  These virtual methods can accept or reject 
    //    the talker ID (for standard sentences) or 
    //    the manufacturer ID (for proprietary sentences).
    //  The default is to accept *all* IDs.
    //  Override them if you want to reject certain IDs, or you want
    //    to handle COMPLETED sentences from certain IDs differently.

    #ifdef NMEAGPS_PARSE_TALKER_ID
      NMEAGPS_VIRTUAL bool parseTalkerID( char ) { return true; };
    #endif

    #ifdef NMEAGPS_PARSE_PROPRIETARY
      #ifdef NMEAGPS_PARSE_MFR_ID
        NMEAGPS_VIRTUAL bool parseMfrID( char ) { return true; };
      #endif
    #endif

    //.......................................................................
    // Try to recognize an NMEA sentence type, after the IDs have been accepted.

    decode_t parseCommand( char c );
    decode_t parseCommand( const msg_table_t *msgs, uint8_t cmdCount, char c );

    //.......................................................................
    // Parse various NMEA sentences

    bool parseGGA( char chr );
    bool parseGLL( char chr );
    bool parseGSA( char chr );
    bool parseGST( char chr );
    bool parseGSV( char chr );
    bool parseRMC( char chr );
    bool parseVTG( char chr );
    bool parseZDA( char chr );

    //.......................................................................
    // Depending on the NMEA sentence type, parse one field of an expected type.

    NMEAGPS_VIRTUAL bool parseField( char chr );

    //.......................................................................
    // Parse the primary NMEA field types into /fix/ members.

    bool parseFix        ( char chr ); // aka STATUS or MODE
    bool parseTime       ( char chr );
    bool parseDDMMYY     ( char chr );
    bool parseLat        ( char chr );
    bool parseNS         ( char chr );
    bool parseLon        ( char chr );
    bool parseEW         ( char chr );
    bool parseSpeed      ( char chr );
    bool parseHeading    ( char chr );
    bool parseAlt        ( char chr );
    bool parseGeoidHeight( char chr );
    bool parseHDOP       ( char chr );
    bool parseVDOP       ( char chr );
    bool parsePDOP       ( char chr );
    bool parse_lat_err   ( char chr );
    bool parse_lon_err   ( char chr );
    bool parse_alt_err   ( char chr );
    bool parseSatellites ( char chr );

    // Helper macro for parsing the 4 consecutive fields of a location
    #define PARSE_LOC(i) case i: return parseLat( chr );\
      case i+1: return parseNS ( chr ); \
      case i+2: return parseLon( chr ); \
      case i+3: return parseEW ( chr );

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

    //.......................................................................
    // Parse floating-point numbers into a /whole_frac/
    // @return true when the value is fully populated.

    bool parseFloat( gps_fix::whole_frac & val, char chr, uint8_t max_decimal );

    //.......................................................................
    // Parse floating-point numbers into a uint16_t
    // @return true when the value is fully populated.

    bool parseFloat( uint16_t & val, char chr, uint8_t max_decimal );

    //.......................................................................
    // Parse NMEA lat/lon dddmm.mmmm degrees

    bool parseDDDMM
      (
        #if defined( GPS_FIX_LOCATION )
          int32_t & val,
        #endif
        #if defined( GPS_FIX_LOCATION_DMS )
          DMS_t & dms,
        #endif
        char chr
      );

    //.......................................................................
    // Parse integer into 8-bit int
    // @return true when non-empty value

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

    //.......................................................................
    // Parse integer into signed 8-bit int
    // @return true when non-empty value

    bool parseInt( int8_t &val, uint8_t chr )
    {
      bool is_comma = (chr == ',');

      if (chrCount == 0) {
        if (is_comma)
          return false; // empty field!

        if (chr == '-') {
          negative = true;
          comma_needed( true ); // to negate
        } else if (chr == '+') {
          ;
        } else {
          val = (chr - '0');
        }
      } else if (!is_comma) {
        val = (val*10) + (chr - '0');

      } else if (negative) {
        val = -val;
      }

      return true;
    }

    //.......................................................................
    // Parse integer into 16-bit int
    // @return true when non-empty value

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

    //.......................................................................
    // Parse integer into 32-bit int
    // @return true when non-empty value

    bool parseInt( uint32_t &val, uint8_t chr )
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
