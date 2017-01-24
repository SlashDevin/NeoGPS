/**
 * @file NMEAGPS.cpp
 * @version 4.1.0
 *
 * @section License
 * Copyright (C) 2016, SlashDevin
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

#include <Stream.h>

// Check configurations
 
#if defined( GPS_FIX_LOCATION_DMS ) & \
    !defined( NMEAGPS_PARSING_SCRATCHPAD )

  // The fractional part of the NMEA minutes can have 5 significant figures.
  //   This requires more temporary storage than is available in the DMS_t.
  #error You must enable NMEAGPS_PARSING_SCRATCHPAD in NMEAGPS_cfg.h when GPS_FIX_LOCATION_DMS is enabled in GPSfix_cfg.h!

#endif

#ifndef CR
  #define CR ((char)13)
#endif
#ifndef LF
  #define LF ((char)10)
#endif

//----------------------------------------------------------------
//  Parse a single character as HEX and returns byte value.

inline static uint8_t parseHEX(char a)
{
  a |= 0x20; // make it lowercase
  if (('a' <= a) && (a <= 'f'))
      return a - 'a' + 10;
  else
      return a - '0';
}

//----------------------------------------------------------------
// Format lower nybble of value as HEX and returns character.

static char formatHex( uint8_t val )
{
  val &= 0x0F;
  return (val >= 10) ? ((val - 10) + 'A') : (val + '0');
}

//----------------------------------------------------------------

inline uint8_t to_binary(uint8_t value)
{
  uint8_t high = (value >> 4);
  uint8_t low = (value & 0x0f);
  return ((high << 3) + (high << 1) + low);
}

//----------------------------------------------------------------

NMEAGPS::NMEAGPS()
{
  #ifdef NMEAGPS_STATS
    statistics.init();
  #endif

  data_init();

  reset();
}

//----------------------------------------------------------------
// Prepare internal members to receive data from sentence fields.

void NMEAGPS::sentenceBegin()
{
  crc          = 0;
  nmeaMessage  = NMEA_UNKNOWN;
  rxState      = NMEA_RECEIVING_HEADER;
  chrCount     = 0;
  comma_needed( false );

  #ifdef NMEAGPS_PARSE_PROPRIETARY
    proprietary = false;

    #ifdef NMEAGPS_SAVE_MFR_ID
      mfr_id[0] =
      mfr_id[1] =
      mfr_id[2] = 0;
    #endif
  #endif

  #ifdef NMEAGPS_SAVE_TALKER_ID
    talker_id[0] =
    talker_id[1] = 0;
  #endif

  // If the previous interval was completed,
  //   this is the start of a new interval.

  if (intervalComplete()) {
    intervalComplete( false );

    #ifdef NMEAGPS_PARSE_SATELLITES
      sat_count = 0;
    #endif
  }
}

//----------------------------------------------------------------
// All fields from a sentence have been parsed.

void NMEAGPS::sentenceOk()
{
  // Terminate the last field with a comma if the parser needs it.
  if (comma_needed()) {
    comma_needed( false );
    chrCount++;
    parseField(',');
  }

  #ifdef NMEAGPS_STATS
    statistics.ok++;
  #endif

  //  This implements coherency.
  intervalComplete( intervalCompleted() );

  reset();
}

//----------------------------------------------------------------
// There was something wrong with the sentence.

void NMEAGPS::sentenceInvalid()
{
  // All the values are suspect.  Start over.
  m_fix.valid.init();
  nmeaMessage = NMEA_UNKNOWN;

  reset();
}

//----------------------------------------------------------------
//  The sentence is well-formed, but is an unrecognized type

void NMEAGPS::sentenceUnrecognized()
{
  nmeaMessage = NMEA_UNKNOWN;

  reset();
}

//----------------------------------------------------------------

void NMEAGPS::headerReceived()
{
  NMEAGPS_INIT_FIX(m_fix);
  fieldIndex = 1;
  chrCount   = 0;
  rxState    = NMEA_RECEIVING_DATA;
}

//----------------------------------------------------------------
// Process one character of an NMEA GPS sentence. 

NMEAGPS::decode_t NMEAGPS::decode( char c )
{
  #ifdef NMEAGPS_STATS
    statistics.chars++;
  #endif

  decode_t res = DECODE_CHR_OK;

  if (c == '$') {  // Always restarts
    sentenceBegin();

  } else if (rxState == NMEA_RECEIVING_DATA) { //---------------------------
    // Receive complete sentence

    if (c == '*') {                // Line finished, CRC follows
        rxState = NMEA_RECEIVING_CRC;
        chrCount = 0;

    } else if ((' ' <= c) && (c <= '~')) { // Normal data character

        crc ^= c;  // accumulate CRC as the chars come in...

        if (!parseField( c ))
          sentenceInvalid();
        else if (c == ',') {
          // Start the next field
          comma_needed( false );
          fieldIndex++;
          chrCount     = 0;
        } else
          chrCount++;

    // This is an undocumented option.  It could be useful
    // for testing, but all real devices will output a CS.
    #ifdef NMEAGPS_CS_OPTIONAL
      } else if ((c == CR) || (c == LF)) { // Line finished, no CRC
        sentenceOk();
        res = DECODE_COMPLETED;
    #endif

    } else {                           // Invalid char
      sentenceInvalid();
      res = DECODE_CHR_INVALID;
    }
    
    
  } else if (rxState == NMEA_RECEIVING_HEADER) { //------------------------

    //  The first field is the sentence type.  It will be used
    //  later by the virtual /parseField/.

    crc ^= c;  // accumulate CRC as the chars come in...

    decode_t cmd_res = parseCommand( c );

    if (cmd_res == DECODE_CHR_OK) {
      chrCount++;
    } else if (cmd_res == DECODE_COMPLETED) {
      headerReceived();
    } else // DECODE_CHR_INVALID
      sentenceUnrecognized();


  } else if (rxState == NMEA_RECEIVING_CRC) { //---------------------------
    bool    err;
    uint8_t nybble = parseHEX( c );

    if (chrCount == 0) {
      chrCount++;
      err = ((crc >> 4) != nybble);
    } else { // chrCount == 1
      err = ((crc & 0x0F) != nybble);
      if (!err) {
        sentenceOk();
        res = DECODE_COMPLETED;
      }
    }

    if (err) {
      #ifdef NMEAGPS_STATS
        statistics.errors++;
      #endif
      sentenceInvalid();
    }

  } else if (rxState == NMEA_IDLE) { //---------------------------
    // Reject non-start characters

    res         = DECODE_CHR_INVALID;
    nmeaMessage = NMEA_UNKNOWN;
  }

  return res;

} // decode

//----------------------------------------------------------------

NMEAGPS::decode_t NMEAGPS::handle( uint8_t c )
{
  decode_t res = decode( c );

  if (res == DECODE_COMPLETED) {
    storeFix();

  } else if ((NMEAGPS_FIX_MAX == 0) && _available() && !is_safe()) {
    // No buffer, and m_fix is was modified by the last char
    overrun( true );
  }

  return res;

} // handle

//----------------------------------------------------------------

void NMEAGPS::storeFix()
{
  // Room for another fix?

  bool room = ((NMEAGPS_FIX_MAX == 0) &&  !_available()) ||
              ((NMEAGPS_FIX_MAX >  0) && (_available() < NMEAGPS_FIX_MAX));

  if (!room) {
    overrun( true );

    if (keepNewestFixes) {

      #if NMEAGPS_FIX_MAX > 0

        // Write over the oldest fix (_firstFix), so "pop" it off the front.
        _firstFix++;
        if (_firstFix >= NMEAGPS_FIX_MAX)
          _firstFix = 0;

        // this new one is not available until the interval is complete
        _fixesAvailable--;

      #else
        // Write over the one and only fix.  It may not be complete.
        _fixesAvailable = false;
      #endif

      // Now there's room!
      room = true;
    }
  }

  if (room) {
    // YES, save it.
    //   Note: If FIX_MAX == 0, this just marks _fixesAvailable = true.

    if ((NMEAGPS_FIX_MAX > 0) && (merging == EXPLICIT_MERGING)) {
      // Accumulate all sentences
      buffer[ _currentFix ] |= fix();
    }

    if ((merging == NO_MERGING) || intervalComplete()) {

      #if NMEAGPS_FIX_MAX > 0

        if (merging != EXPLICIT_MERGING)
          buffer[ _currentFix ] = fix();

        _currentFix++;
        if (_currentFix >= NMEAGPS_FIX_MAX)
          _currentFix = 0;

        if (_fixesAvailable < NMEAGPS_FIX_MAX)
          _fixesAvailable++;

      #else // FIX_MAX == 0
        _fixesAvailable = true;
      #endif

    }
  }

} // storeFix

//----------------------------------------------------------------
// NMEA Sentence strings (alphabetical)

#if defined(NMEAGPS_PARSE_GGA) | defined(NMEAGPS_RECOGNIZE_ALL)
  static const char gga[] __PROGMEM =  "GGA";
#endif
#if defined(NMEAGPS_PARSE_GLL) | defined(NMEAGPS_RECOGNIZE_ALL)
  static const char gll[] __PROGMEM =  "GLL";
#endif
#if defined(NMEAGPS_PARSE_GSA) | defined(NMEAGPS_RECOGNIZE_ALL)
  static const char gsa[] __PROGMEM =  "GSA";
#endif
#if defined(NMEAGPS_PARSE_GST) | defined(NMEAGPS_RECOGNIZE_ALL)
  static const char gst[] __PROGMEM =  "GST";
#endif
#if defined(NMEAGPS_PARSE_GSV) | defined(NMEAGPS_RECOGNIZE_ALL)
  static const char gsv[] __PROGMEM =  "GSV";
#endif
#if defined(NMEAGPS_PARSE_RMC) | defined(NMEAGPS_RECOGNIZE_ALL)
  static const char rmc[] __PROGMEM =  "RMC";
#endif
#if defined(NMEAGPS_PARSE_VTG) | defined(NMEAGPS_RECOGNIZE_ALL)
  static const char vtg[] __PROGMEM =  "VTG";
#endif
#if defined(NMEAGPS_PARSE_ZDA) | defined(NMEAGPS_RECOGNIZE_ALL)
  static const char zda[] __PROGMEM =  "ZDA";
#endif

static const char * const std_nmea[] __PROGMEM =
  {
    #if defined(NMEAGPS_PARSE_GGA) | defined(NMEAGPS_RECOGNIZE_ALL)
      gga,
    #endif
    #if defined(NMEAGPS_PARSE_GLL) | defined(NMEAGPS_RECOGNIZE_ALL)
      gll,
    #endif
    #if defined(NMEAGPS_PARSE_GSA) | defined(NMEAGPS_RECOGNIZE_ALL)
      gsa,
    #endif
    #if defined(NMEAGPS_PARSE_GST) | defined(NMEAGPS_RECOGNIZE_ALL)
      gst,
    #endif
    #if defined(NMEAGPS_PARSE_GSV) | defined(NMEAGPS_RECOGNIZE_ALL)
      gsv,
    #endif
    #if defined(NMEAGPS_PARSE_RMC) | defined(NMEAGPS_RECOGNIZE_ALL)
      rmc,
    #endif
    #if defined(NMEAGPS_PARSE_VTG) | defined(NMEAGPS_RECOGNIZE_ALL)
      vtg,
    #endif
    #if defined(NMEAGPS_PARSE_ZDA) | defined(NMEAGPS_RECOGNIZE_ALL)
      zda
    #endif
  };

const NMEAGPS::msg_table_t NMEAGPS::nmea_msg_table __PROGMEM =
  {
    NMEAGPS::NMEA_FIRST_MSG,
    (const msg_table_t *) NULL,
    sizeof(std_nmea)/sizeof(std_nmea[0]),
    std_nmea
  };

//----------------------------------------------------------------
//  For NMEA, start with talker or manufacture ID

NMEAGPS::decode_t NMEAGPS::parseCommand( char c )
{
  if (c == ',') {
    // End of field, did we get a sentence type yet?
    return
      (nmeaMessage == NMEA_UNKNOWN) ?
        DECODE_CHR_INVALID :
        DECODE_COMPLETED;
  }

  #ifdef NMEAGPS_PARSE_PROPRIETARY
    if ((chrCount == 0) && (c == 'P')) {
      //  Starting a proprietary message...
      proprietary = true;
      return DECODE_CHR_OK;
    }
  #endif
  
  uint8_t cmdCount = chrCount;

  #ifdef NMEAGPS_PARSE_PROPRIETARY
    if (proprietary) {

      // Next three chars are the manufacturer ID
      if (chrCount < 4) {
        #ifdef NMEAGPS_SAVE_MFR_ID
          mfr_id[chrCount-1] = c;
        #endif

        #ifdef NMEAGPS_PARSE_MFR_ID
          if (!parseMfrID( c ))
            return DECODE_CHR_INVALID;
        #endif

        return DECODE_CHR_OK;
      }

      cmdCount -= 4;

    } else
  #endif
  { // standard

    // First two chars are talker ID
    if (chrCount < 2) {
      #ifdef NMEAGPS_SAVE_TALKER_ID
        talker_id[chrCount] = c;
      #endif

      #ifdef NMEAGPS_PARSE_TALKER_ID
        if (!parseTalkerID( c ))
          return DECODE_CHR_INVALID;
      #endif

      return DECODE_CHR_OK;
    }
    
    cmdCount -= 2;
  }

  //  The remaining characters are the message type.

  const msg_table_t *msgs = msg_table();

  return parseCommand( msgs, cmdCount, c );

} // parseCommand

//----------------------------------------------------------------
//  Determine the NMEA sentence type

NMEAGPS::decode_t NMEAGPS::parseCommand
  ( const msg_table_t *msgs, uint8_t cmdCount, char c )
{
  for (;;) {
    uint8_t  table_size       = pgm_read_byte( &msgs->size );
    uint8_t  msg_offset       = pgm_read_byte( &msgs->offset );
    decode_t res              = DECODE_CHR_INVALID;
    bool     check_this_table = true;
    uint8_t  entry;

    if (nmeaMessage == NMEA_UNKNOWN) {
      // We're just starting
      entry = 0;

    } else if ((msg_offset <= nmeaMessage) && (nmeaMessage < msg_offset+table_size)) {
      // In range of this table, pick up where we left off
      entry = nmeaMessage - msg_offset;
    }
    #ifdef NMEAGPS_DERIVED_TYPES
      else
        check_this_table = false;
    #endif

    if (check_this_table) {
      uint8_t i = entry;

      #ifdef __AVR__
        const char * const *table   = (const char * const *) pgm_read_ptr( &msgs->table );
        const char *        table_i = (const char *) pgm_read_ptr( &table[i] );
      #else
        const char * const *table   = msgs->table;
        const char *        table_i = table[i];
      #endif

      for (;;) {
        char rc = pgm_read_byte( &table_i[cmdCount] );
        if (c == rc) {
          // ok so far...
          entry = i;
          res = DECODE_CHR_OK;
          break;
        }

        if (c < rc) {
          // Alphabetical rejection, check next table
          break;
        }

        // Ok to check another entry in this table
        uint8_t next_msg = i+1;
        if (next_msg >= table_size) {
          // No more entries in this table.
          break;
        }

        //  See if the next entry starts with the same characters.
        #ifdef __AVR__
          const char *table_next = (const char *) pgm_read_word( &table[next_msg] );
        #else
          const char *table_next = table[next_msg];
        #endif
        for (uint8_t j = 0; j < cmdCount; j++)
          if (pgm_read_byte( &table_i[j] ) != pgm_read_byte( &table_next[j] )) {
            // Nope, a different start to this entry
            break;
          }
        i = next_msg;
        table_i = table_next;
      }
    }

    if (res == DECODE_CHR_INVALID) {

      #ifdef NMEAGPS_DERIVED_TYPES
        #ifdef __AVR__
          msgs = (const msg_table_t *) pgm_read_word( &msgs->previous );
        #else
          msgs = msgs->previous;
        #endif
        if (msgs) {
          // Try the current character in the previous table
          continue;
        } // else
          // No more tables, chr is invalid.
      #endif
      
    } else {
      //  This entry is good so far.
      nmeaMessage = (nmea_msg_t) (entry + msg_offset);
    }

    return res;
  }

} // parseCommand

//----------------------------------------------------------------

const __FlashStringHelper *NMEAGPS::string_for( nmea_msg_t msg ) const
{
  if (msg == NMEA_UNKNOWN)
    return F("UNK");

  const msg_table_t *msgs = msg_table();

  for (;;) {
    uint8_t  table_size       = pgm_read_byte( &msgs->size );
    uint8_t  msg_offset       = pgm_read_byte( &msgs->offset );

    if ((msg_offset <= msg) && (msg < msg_offset+table_size)) {
      // In range of this table
      #ifdef __AVR__
        const char * const *table   = (const char * const *) pgm_read_word( &msgs->table );
        return
          (const __FlashStringHelper *) 
            pgm_read_word( &table[ ((uint8_t)msg) - msg_offset ] );
      #else
        const char * const *table   = msgs->table;
        return
          (const __FlashStringHelper *) 
            table[ ((uint8_t)msg) - msg_offset ];
      #endif
      
    }
 
    #ifdef NMEAGPS_DERIVED_TYPES
      // Try the previous table
      #ifdef __AVR__
        msgs = (const msg_table_t *) pgm_read_word( &msgs->previous );
      #else
        msgs = (const msg_table_t *) &msgs->previous;
      #endif
      if (msgs)
        continue;
    #endif

    return (const __FlashStringHelper *)  NULL;
  }

} // string_for

//----------------------------------------------------------------

bool NMEAGPS::parseField(char chr)
{
    switch (nmeaMessage) {

      #if defined(NMEAGPS_PARSE_GGA)
        case NMEA_GGA: return parseGGA( chr );
      #endif

      #if defined(NMEAGPS_PARSE_GLL)
        case NMEA_GLL: return parseGLL( chr );
      #endif

      #if defined(NMEAGPS_PARSE_GSA)
        case NMEA_GSA: return parseGSA( chr );
      #endif

      #if defined(NMEAGPS_PARSE_GST)
        case NMEA_GST: return parseGST( chr );
      #endif

      #if defined(NMEAGPS_PARSE_GSV)
        case NMEA_GSV: return parseGSV( chr );
      #endif

      #if defined(NMEAGPS_PARSE_RMC)
        case NMEA_RMC: return parseRMC( chr );
      #endif

      #if defined(NMEAGPS_PARSE_VTG)
        case NMEA_VTG: return parseVTG( chr );
      #endif

      #if defined(NMEAGPS_PARSE_ZDA)
        case NMEA_ZDA: return parseZDA( chr );
      #endif

      default:
          break;
    }

    return true;

} // parseField

//----------------------------------------------------------------

bool NMEAGPS::parseGGA( char chr )
{
  #ifdef NMEAGPS_PARSE_GGA
    switch (fieldIndex) {
        case  1: return parseTime( chr );
        PARSE_LOC(2);
        case  6: return parseFix( chr );
        case  7: return parseSatellites( chr );
        case  8: return parseHDOP( chr );
        case  9: return parseAlt( chr );
        case 11: return parseGeoidHeight( chr );
    }
  #endif

  return true;

} // parseGGA

//----------------------------------------------------------------

bool NMEAGPS::parseGLL( char chr )
{
  #ifdef NMEAGPS_PARSE_GLL
    switch (fieldIndex) {
        PARSE_LOC(1);
        case 5: return parseTime( chr );
        case 7: return parseFix( chr );
    }
  #endif

  return true;

} // parseGLL

//----------------------------------------------------------------

bool NMEAGPS::parseGSA( char chr )
{
  #ifdef NMEAGPS_PARSE_GSA

    switch (fieldIndex) {
      case 2:
        if (chrCount == 0) {
          if ((chr == '2') || (chr == '3')) {
            m_fix.status = gps_fix::STATUS_STD;
            m_fix.valid.status = true;
          } else if (chr == '1') {
            m_fix.status = gps_fix::STATUS_NONE;
            m_fix.valid.status = true;
          }
        }
        break;

      case 15: return parsePDOP( chr );
      case 16: return parseHDOP( chr );
      case 17: return parseVDOP( chr );
         #if defined(GPS_FIX_VDOP) & !defined(NMEAGPS_COMMA_NEEDED)
           #error When GSA and VDOP are enabled, you must define NMEAGPS_COMMA_NEEDED in NMEAGPS_cfg.h!
         #endif

      #ifdef NMEAGPS_PARSE_SATELLITES

        // It's not clear how this sentence relates to GSV and GGA.  
        // GSA only allows 12 satellites, while GSV allows any number.
        // GGA just says how many are used to calculate a fix.

          case 1: break; // allows "default:" case for SV fields

        // GGA shall have priority over GSA with respect to populating the
        // satellites field.  Ignore the satellite field if GGA is enabled.
        #ifndef NMEAGPS_PARSE_GGA
          case 2: return parseSatellites( chr );
        #endif

        // GSV shall have priority over GSA with respect to populating the
        // satellites array.  Ignore the satellite fields if GSV is enabled.
        #ifndef NMEAGPS_PARSE_GSV
          case 3:
            if (chrCount == 0) {
              sat_count = 0;
              comma_needed( true );
            }
          default:
            if (chr == ',') {
              if (chrCount > 0) {
                sat_count++;
              }
            } else
              parseInt( satellites[ sat_count ].id, chr );
            break;
        #endif
      #endif
    }
  #endif

  return true;

} // parseGSA

//----------------------------------------------------------------

bool NMEAGPS::parseGST( char chr )
{
  #ifdef NMEAGPS_PARSE_GST
    switch (fieldIndex) {
      case 1: return parseTime( chr );
      case 6: return parse_lat_err( chr );
      case 7: return parse_lon_err( chr );
      case 8: return parse_alt_err( chr );
    }
  #endif

  return true;

} // parseGST

//----------------------------------------------------------------

bool NMEAGPS::parseGSV( char chr )
{
  #if defined(NMEAGPS_PARSE_GSV) & defined(NMEAGPS_PARSE_SATELLITES)
    if (sat_count < NMEAGPS_MAX_SATELLITES) {
      if (fieldIndex >= 4) {

        switch (fieldIndex % 4) {
          #ifdef NMEAGPS_PARSE_SATELLITE_INFO
            case 0: parseInt( satellites[sat_count].id       , chr ); break;
            case 1: parseInt( satellites[sat_count].elevation, chr ); break;
            case 2:
              if (chr != ',')
                parseInt( satellites[sat_count].azimuth, chr );
              else
                sat_count++; // field 3 can be omitted, increment now
              break;
            case 3:
              if (chr != ',') {
                uint8_t snr = satellites[sat_count-1].snr;
                parseInt( snr, chr );
                satellites[sat_count-1].snr = snr;
                comma_needed( true );
              } else
                satellites[sat_count-1].tracked = (chrCount != 0);
              break;
          #else
            case 0:
              if (chr != ',')
                parseInt( satellites[sat_count].id, chr );
              else
                sat_count++;
              break;
          #endif
        }
      }
    }
  #endif

  return true;

} // parseGSV

//----------------------------------------------------------------

bool NMEAGPS::parseRMC( char chr )
{
  #ifdef NMEAGPS_PARSE_RMC
    switch (fieldIndex) {
      case 1:  return parseTime   ( chr );
      case 2:  return parseFix    ( chr );
      PARSE_LOC(3);
      case 7:  return parseSpeed  ( chr );
      case 8:  return parseHeading( chr );
      case 9:  return parseDDMMYY ( chr );
      case 12: return parseFix    ( chr );
    }
  #endif

  return true;

} // parseRMC

//----------------------------------------------------------------

bool NMEAGPS::parseVTG( char chr )
{
  #ifdef NMEAGPS_PARSE_VTG
    switch (fieldIndex) {
      case 1: return parseHeading( chr );
      case 5: return parseSpeed( chr );
      case 9: return parseFix( chr );
    }
  #endif

  return true;

} // parseVTG

//----------------------------------------------------------------

bool NMEAGPS::parseZDA( char chr )
{
  #ifdef NMEAGPS_PARSE_ZDA
    switch (fieldIndex) {
      case 1: return parseTime( chr );

      #ifdef GPS_FIX_DATE
        case 2:
          if (chrCount == 0)
            NMEAGPS_INVALIDATE( date );
          parseInt( m_fix.dateTime.date , chr );
          break;
        case 3: parseInt( m_fix.dateTime.month, chr ); break;
        case 4:
          if (chr != ',') {
            // year is BCD until terminating comma.
            //   This essentially keeps the last two digits
            if (chrCount == 0) {
              comma_needed( true );
              m_fix.dateTime.year = (chr - '0');
            } else
              m_fix.dateTime.year = (m_fix.dateTime.year << 4) + (chr - '0');
          } else {
            m_fix.dateTime.year = to_binary( m_fix.dateTime.year );
            m_fix.valid.date = true;
          }
          break;
      #endif
    }
  #endif

  return true;

} // parseZDA

//----------------------------------------------------------------

bool NMEAGPS::parseTime(char chr)
{
  #ifdef GPS_FIX_TIME
    switch (chrCount) {
      case 0: NMEAGPS_INVALIDATE( time );
              m_fix.dateTime.hours    = (chr - '0')*10; break;
      case 1: m_fix.dateTime.hours   += (chr - '0');    break;
      case 2: m_fix.dateTime.minutes  = (chr - '0')*10; break;
      case 3: m_fix.dateTime.minutes += (chr - '0');    break;
      case 4: m_fix.dateTime.seconds  = (chr - '0')*10; break;
      case 5: m_fix.dateTime.seconds += (chr - '0');    break;
      case 7: m_fix.dateTime_cs       = (chr - '0')*10; break;
      case 8: m_fix.dateTime_cs      += (chr - '0');
              m_fix.valid.time = true;
              break;
    }
  #endif

  return true;

} // parseTime

//----------------------------------------------------------------

bool NMEAGPS::parseDDMMYY( char chr )
{
  #ifdef GPS_FIX_DATE
    switch (chrCount) {
      case 0: NMEAGPS_INVALIDATE( date );
              m_fix.dateTime.date   = (chr - '0')*10; break;
      case 1: m_fix.dateTime.date  += (chr - '0');    break;
      case 2: m_fix.dateTime.month  = (chr - '0')*10; break;
      case 3: m_fix.dateTime.month += (chr - '0');    break;
      case 4: m_fix.dateTime.year   = (chr - '0')*10; break;
      case 5: m_fix.dateTime.year  += (chr - '0');
              m_fix.valid.date = true;
              break;
    }
  #endif

  return true;

} // parseDDMMYY

//----------------------------------------------------------------

bool NMEAGPS::parseFix( char chr )
{
  if (chrCount == 0) {
    NMEAGPS_INVALIDATE( status );
    bool ok = true;
    if ((chr == '1') || (chr == 'A'))
      m_fix.status = gps_fix::STATUS_STD;
    else if ((chr == '0') || (chr == 'N') || (chr == 'V'))
      m_fix.status = gps_fix::STATUS_NONE;
    else if ((chr == '2') || (chr == 'D'))
      m_fix.status = gps_fix::STATUS_DGPS;
    else if ((chr == '6') || (chr == 'E'))
      m_fix.status = gps_fix::STATUS_EST;
    else
      ok = false;
    if (ok)
      m_fix.valid.status = true;
  }

  return true;

} // parseFix

//----------------------------------------------------------------

bool NMEAGPS::parseFloat
  ( gps_fix::whole_frac & val, char chr, uint8_t max_decimal )
{
  bool done = false;
  
  if (chrCount == 0) {
    val.init();
    comma_needed( true );
    decimal      = 0;
    negative     = (chr == '-');
    if (negative) return done;
  }

  if (chr == ',') {
    // End of field, make sure it's scaled up
    if (!decimal)
      decimal = 1;
    if (val.frac)
      while (decimal++ <= max_decimal)
        val.frac *= 10;
    if (negative) {
      val.frac  = -val.frac;
      val.whole = -val.whole;
    }
    done = true;
  } else if (chr == '.') {
    decimal = 1;
  } else if (!decimal) {
    val.whole = val.whole*10 + (chr - '0');
  } else if (decimal++ <= max_decimal) {
    val.frac = val.frac*10 + (chr - '0');
  }

  return done;

} // parseFloat

//----------------------------------------------------------------

bool NMEAGPS::parseFloat( uint16_t & val, char chr, uint8_t max_decimal )
{
  bool done = false;

  if (chrCount == 0) {
    val          = 0;
    comma_needed( true );
    decimal      = 0;
    negative     = (chr == '-');
    if (negative) return done;
  }

  if (chr == ',') {
    if (val)
      while (decimal++ <= max_decimal)
        val *= 10;
    if (negative)
      val = -val;
    done = true;
  } else if (chr == '.')
    decimal = 1;
  else if (decimal++ <= max_decimal)
    val = val*10 + (chr - '0');

  return done;

} // parseFloat

//----------------------------------------------------------------

#ifdef GPS_FIX_LOCATION_DMS

  static void finalizeDMS( uint32_t min_frac, DMS_t & dms )
  {
    // To convert from fractional minutes (hundred thousandths) to 
    //   seconds_whole and seconds_frac,
    //
    //   seconds = min_frac * 60/100000
    //           = min_frac * 0.0006

    #ifdef __AVR__
      // Fixed point conversion factor 0.0006 * 2^26 = 40265
      const uint32_t to_seconds_E26 = 40265UL;

      uint32_t secs_E26      = min_frac * to_seconds_E26;
      uint8_t  secs          = secs_E26 >> 26;
      uint32_t remainder_E26 = secs_E26 - (((uint32_t) secs) << 26);

      // thousandths = (rem_E26 * 1000) >> 26;
      //             = (rem_E26 *  125) >> 23;             // 1000 = (125 << 3)
      //             = ((rem_E26 >> 8) * 125) >> 15;       // avoid overflow
      //             = (((rem_E26 >> 8) * 125) >> 8) >> 7; // final shift 15 in two steps
      //             = ((((rem_E26 >> 8) * 125) >> 8) + 72) >> 7;
      //                                                   // round up
      uint16_t frac_x1000   = ((((remainder_E26 >> 8) * 125UL) >> 8) + 64) >> 7;

    #else // not __AVR__

      min_frac *= 6UL;
      uint32_t secs       = min_frac / 10000UL;
      uint16_t frac_x1000 = (min_frac - (secs * 10000UL) + 5) / 10;

    #endif

    // Rounding up can yield a frac of 1000/1000ths. Carry into whole.
    if (frac_x1000 >= 1000) {
      frac_x1000 -= 1000;
      secs       += 1;
    }
    dms.seconds_whole  = secs;
    dms.seconds_frac   = frac_x1000;

  } // finalizeDMS

#endif

//.................................................
// From http://www.hackersdelight.org/divcMore.pdf

static uint32_t divu3( uint32_t n )
{
  #ifdef __AVR__
    uint32_t q = (n >> 2) + (n >> 4); // q = n*0.0101 (approx).
    q = q + (q >> 4); // q = n*0.01010101.
    q = q + (q >> 8);
    q = q + (q >> 16);

    uint32_t r = n - q*3; // 0 <= r <= 15.
    return q + (11*r >> 5); // Returning q + r/3.
  #else
    return n/3;
  #endif
}

//.................................................
// Parse lat/lon dddmm.mmmm fields

bool NMEAGPS::parseDDDMM
  (
    #if defined( GPS_FIX_LOCATION )
      int32_t & val,
    #endif
    #if defined( GPS_FIX_LOCATION_DMS )
      DMS_t & dms,
    #endif
    char chr
  )
{
  #if defined( GPS_FIX_LOCATION ) | defined( GPS_FIX_LOCATION_DMS )

    if (chrCount == 0) {
      #ifdef GPS_FIX_LOCATION
        val        = 0;
      #endif
      #ifdef GPS_FIX_LOCATION_DMS
        dms.init();
      #endif
      decimal      = 0;
      comma_needed( true );
    }
    
    if ((chr == '.') || ((chr == ',') && !decimal)) {
      // Now we know how many digits are in degrees; all but the last two.
      // Switch from BCD (digits) to binary minutes.
      decimal = 1;
      #ifdef GPS_FIX_LOCATION
        uint8_t *valBCD = (uint8_t *) &val;
      #else
        uint8_t *valBCD = (uint8_t *) &dms;
      #endif
      uint8_t  deg     = to_binary( valBCD[1] );
      if (valBCD[2] != 0)
        deg += 100; // only possible if abs(longitude) >= 100.0 degrees

      // Convert val to minutes
      uint8_t min = to_binary( valBCD[0] );
      #ifdef GPS_FIX_LOCATION
        val = (deg * 60) + min;
      #endif
      #ifdef GPS_FIX_LOCATION_DMS
        dms.degrees   = deg;
        dms.minutes   = min;
        scratchpad.U4 = 0;
      #endif

      if (chr == '.') return true;
    }

    if (chr == ',') {
      // If the last chars in ".mmmmmm" were not received,
      //    force the value into its final state.

      #ifdef GPS_FIX_LOCATION_DMS
        if (decimal <= 5) {
          if (decimal == 5)
            scratchpad.U4 *= 10;
          else if (decimal == 4)
            scratchpad.U4 *= 100;
          else if (decimal == 3)
            scratchpad.U4 *= 1000;
          else if (decimal == 2)
            scratchpad.U4 *= 10000;

          finalizeDMS( scratchpad.U4, dms );
        }
      #endif

      #ifdef GPS_FIX_LOCATION
        if (decimal == 4)
          val *= 100;
        else if (decimal == 5)
          val *= 10;
        else if (decimal == 6)
          ;
        else if (decimal > 6)
          return true; // already converted at decimal==7
        else if (decimal == 3)
          val *= 1000;
        else if (decimal == 2)
          val *= 10000;
        else if (decimal == 1)
          val *= 100000;

        // Convert minutes x 1000000 to degrees x 10000000.
        val += divu3(val*2 + 1); // same as 10 * ((val+30)/60) without trunc
      #endif

    } else if (!decimal) {

      // BCD until *after* decimal point

      #ifdef GPS_FIX_LOCATION
        val = (val<<4) | (chr - '0');
      #else
        uint32_t *val = (uint32_t *) &dms;
        *val = (*val<<4) | (chr - '0');
      #endif

    } else {

      decimal++;

      #ifdef GPS_FIX_LOCATION_DMS
        if (decimal <= 6) {
          scratchpad.U4 = scratchpad.U4 * 10 + (chr - '0');
          if (decimal == 6)
            finalizeDMS( scratchpad.U4, dms );
        }
      #endif

      #ifdef GPS_FIX_LOCATION
        if (decimal <= 6) {

          val = val*10 + (chr - '0');

        } else if (decimal == 7) {

          // Convert now, while we still have the 6th decimal digit
          val += divu3(val*2 + 1); // same as 10 * ((val+30)/60) without trunc
          if (chr >= '9')
            val += 2;
          else if (chr >= '4')
            val += 1;
        }
      #endif
    }

  #endif

  return true;

} // parseDDDMM

//----------------------------------------------------------------

bool NMEAGPS::parseLat( char chr )
{
  #if defined( GPS_FIX_LOCATION ) | defined( GPS_FIX_LOCATION_DMS )
    if (chrCount == 0) {
      group_valid = (chr != ',');
      if (group_valid)
        NMEAGPS_INVALIDATE( location );
    }

    if (group_valid) {
      parseDDDMM
        (
          #if defined( GPS_FIX_LOCATION )
            m_fix.location._lat, 
          #endif
          #if defined( GPS_FIX_LOCATION_DMS )
            m_fix.latitudeDMS,
          #endif
          chr
        );
    }
  #endif

  return true;

} // parseLatitude

//----------------------------------------------------------------

bool NMEAGPS::parseNS( char chr )
{
  #if defined( GPS_FIX_LOCATION ) | defined( GPS_FIX_LOCATION_DMS )
    if (group_valid && (chr == 'S')) {
      #ifdef GPS_FIX_LOCATION
        m_fix.location._lat = -m_fix.location._lat;
      #endif
      #ifdef GPS_FIX_LOCATION_DMS
        m_fix.latitudeDMS.hemisphere = SOUTH_H;
      #endif
    }
  #endif

  return true;

} // parseNS

//----------------------------------------------------------------

bool NMEAGPS::parseLon( char chr )
{
  #if defined( GPS_FIX_LOCATION ) | defined( GPS_FIX_LOCATION_DMS )
    if ((chr == ',') && (chrCount == 0))
      group_valid = false;

    if (group_valid) {
      parseDDDMM
        (
          #if defined( GPS_FIX_LOCATION )
            m_fix.location._lon, 
          #endif
          #if defined( GPS_FIX_LOCATION_DMS )
            m_fix.longitudeDMS,
          #endif
          chr
        );
    }
  #endif

  return true;

} // parseLon

//----------------------------------------------------------------

bool NMEAGPS::parseEW( char chr )
{
  #if defined( GPS_FIX_LOCATION ) | defined( GPS_FIX_LOCATION_DMS )
    if (group_valid) {
      if (chr == 'W') {
        #ifdef GPS_FIX_LOCATION
          m_fix.location._lon = -m_fix.location._lon;
        #endif
        #ifdef GPS_FIX_LOCATION_DMS
          m_fix.longitudeDMS.hemisphere = WEST_H;
        #endif
      }
      m_fix.valid.location = true;
    }
  #endif
  
  return true;

} // parseEW

//----------------------------------------------------------------

bool NMEAGPS::parseSpeed( char chr )
{
  #ifdef GPS_FIX_SPEED
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( speed );
    if (parseFloat( m_fix.spd, chr, 3 ))
      m_fix.valid.speed = (chrCount != 0);
  #endif

  return true;

} // parseSpeed

//----------------------------------------------------------------

bool NMEAGPS::parseHeading( char chr )
{
  #ifdef GPS_FIX_HEADING
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( heading );
    if (parseFloat( m_fix.hdg, chr, 2 ))
      m_fix.valid.heading = (chrCount != 0);
  #endif

  return true;

} // parseHeading

//----------------------------------------------------------------

bool NMEAGPS::parseAlt(char chr )
{
  #ifdef GPS_FIX_ALTITUDE
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( altitude );
    if (parseFloat( m_fix.alt, chr, 2 ))
      m_fix.valid.altitude = (chrCount != 0);
  #endif

  return true;

} // parseAlt

//----------------------------------------------------------------

bool NMEAGPS::parseGeoidHeight( char chr )
{
  #ifdef GPS_FIX_GEOID_HEIGHT
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( geoidHeight );
    if (parseFloat( m_fix.geoidHt, chr, 2 ))
      m_fix.valid.geoidHeight = (chrCount != 0);
  #endif

  return true;

} // parseGeoidHeight

//----------------------------------------------------------------

bool NMEAGPS::parseSatellites( char chr )
{
  #ifdef GPS_FIX_SATELLITES
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( satellites );
    if (parseInt( m_fix.satellites, chr )) {
      m_fix.valid.satellites = true;
    }
  #endif

  return true;

} // parseSatellites

//----------------------------------------------------------------

bool NMEAGPS::parseHDOP( char chr )
{
  #ifdef GPS_FIX_HDOP
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( hdop );
    if (parseFloat( m_fix.hdop, chr, 3 ))
      m_fix.valid.hdop = (chrCount != 0);
  #endif

  return true;

} // parseHDOP

//----------------------------------------------------------------

bool NMEAGPS::parseVDOP( char chr )
{
  #ifdef GPS_FIX_VDOP
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( vdop );
    if (parseFloat( m_fix.vdop, chr, 3 ))
      m_fix.valid.vdop = (chrCount != 0);
  #endif

  return true;

} // parseVDOP

//----------------------------------------------------------------

bool NMEAGPS::parsePDOP( char chr )
{
  #ifdef GPS_FIX_PDOP
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( pdop );
    if (parseFloat( m_fix.pdop, chr, 3 ))
      m_fix.valid.pdop = (chrCount != 0);
  #endif

  return true;

} // parsePDOP

//----------------------------------------------------------------

bool NMEAGPS::parse_lat_err( char chr )
{
  #ifdef GPS_FIX_LAT_ERR
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( lat_err );
    if (parseFloat( m_fix.lat_err_cm, chr, 2 ))
      m_fix.valid.lat_err = (chrCount != 0);
  #endif

  return true;

} // parse_lat_err

//----------------------------------------------------------------

bool NMEAGPS::parse_lon_err( char chr )
{
  #ifdef GPS_FIX_LON_ERR
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( lon_err );
    if (parseFloat( m_fix.lon_err_cm, chr, 2 ))
      m_fix.valid.lon_err = (chrCount != 0);
  #endif

  return true;

} // parse_lon_err

//----------------------------------------------------------------

bool NMEAGPS::parse_alt_err( char chr )
{
  #ifdef GPS_FIX_ALT_ERR
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( alt_err );
    if (parseFloat( m_fix.alt_err_cm, chr, 2 ))
      m_fix.valid.alt_err = (chrCount != 0);
  #endif

  return true;

} // parse_alt_err

//----------------------------------------------------------------

const gps_fix NMEAGPS::read()
{
  gps_fix fix;

  if (_fixesAvailable) {
    lock();

      #if (NMEAGPS_FIX_MAX > 0)
        _fixesAvailable--;
        fix = buffer[ _firstFix ];
        if (merging == EXPLICIT_MERGING)
          // Prepare to accumulate all fixes in an interval
          buffer[ _firstFix ].init();
        if (++_firstFix >= NMEAGPS_FIX_MAX)
          _firstFix = 0;
      #else
        if (is_safe()) {
          _fixesAvailable = false;
          fix = m_fix;
        }
      #endif

    unlock();
  }

  return fix;

} // read

//----------------------------------------------------------------

void NMEAGPS::poll( Stream *device, nmea_msg_t msg )
{
  //  Only the ublox documentation references talker ID "EI".  
  //  Other manufacturer's devices use "II" and "GP" talker IDs for the GPQ sentence.
  //  However, "GP" is reserved for the GPS device, so it seems inconsistent
  //  to use that talker ID when requesting something from the GPS device.

  #if defined(NMEAGPS_PARSE_GGA) | defined(NMEAGPS_RECOGNIZE_ALL)
    static const char gga[] __PROGMEM = "EIGPQ,GGA";
  #endif
  #if defined(NMEAGPS_PARSE_GLL) | defined(NMEAGPS_RECOGNIZE_ALL)
    static const char gll[] __PROGMEM = "EIGPQ,GLL";
  #endif
  #if defined(NMEAGPS_PARSE_GSA) | defined(NMEAGPS_RECOGNIZE_ALL)
    static const char gsa[] __PROGMEM = "EIGPQ,GSA";
  #endif
  #if defined(NMEAGPS_PARSE_GST) | defined(NMEAGPS_RECOGNIZE_ALL)
    static const char gst[] __PROGMEM = "EIGPQ,GST";
  #endif
  #if defined(NMEAGPS_PARSE_GSV) | defined(NMEAGPS_RECOGNIZE_ALL)
    static const char gsv[] __PROGMEM = "EIGPQ,GSV";
  #endif
  #if defined(NMEAGPS_PARSE_RMC) | defined(NMEAGPS_RECOGNIZE_ALL)
    static const char rmc[] __PROGMEM = "EIGPQ,RMC";
  #endif
  #if defined(NMEAGPS_PARSE_VTG) | defined(NMEAGPS_RECOGNIZE_ALL)
    static const char vtg[] __PROGMEM = "EIGPQ,VTG";
  #endif
  #if defined(NMEAGPS_PARSE_ZDA) | defined(NMEAGPS_RECOGNIZE_ALL)
    static const char zda[] __PROGMEM = "EIGPQ,ZDA";
  #endif

  static const char * const poll_msgs[] __PROGMEM =
    {
      #if defined(NMEAGPS_PARSE_GGA) | defined(NMEAGPS_RECOGNIZE_ALL)
        gga,
      #endif
      #if defined(NMEAGPS_PARSE_GLL) | defined(NMEAGPS_RECOGNIZE_ALL)
        gll,
      #endif
      #if defined(NMEAGPS_PARSE_GSA) | defined(NMEAGPS_RECOGNIZE_ALL)
        gsa,
      #endif
      #if defined(NMEAGPS_PARSE_GST) | defined(NMEAGPS_RECOGNIZE_ALL)
        gst,
      #endif
      #if defined(NMEAGPS_PARSE_GSV) | defined(NMEAGPS_RECOGNIZE_ALL)
        gsv,
      #endif
      #if defined(NMEAGPS_PARSE_RMC) | defined(NMEAGPS_RECOGNIZE_ALL)
        rmc,
      #endif
      #if defined(NMEAGPS_PARSE_VTG) | defined(NMEAGPS_RECOGNIZE_ALL)
        vtg,
      #endif
      #if defined(NMEAGPS_PARSE_ZDA) | defined(NMEAGPS_RECOGNIZE_ALL)
        zda
      #endif
    };

  if ((NMEA_FIRST_MSG <= msg) && (msg <= NMEA_LAST_MSG)) {
    #ifdef __AVR__
      const __FlashStringHelper * pollCmd =
        (const __FlashStringHelper *) pgm_read_word(&poll_msgs[msg-NMEA_FIRST_MSG]);
    #else
      const __FlashStringHelper * pollCmd =
        (const __FlashStringHelper *) poll_msgs[msg-NMEA_FIRST_MSG];
    #endif
    send_P( device, pollCmd );
  }

} // poll

//----------------------------------------------------------------

static void send_trailer( Stream *device, uint8_t crc )
{
  device->print('*');

  char hexDigit = formatHex( crc>>4 );
  device->print( hexDigit );

  hexDigit = formatHex( crc );
  device->print( hexDigit );

  device->print( CR );
  device->print( LF );

} // send_trailer

//----------------------------------------------------------------

void NMEAGPS::send( Stream *device, const char *msg )
{
  if (msg && *msg) {
    if (*msg == '$')
      msg++;
    device->print('$');
    uint8_t sent_trailer = 0;
    uint8_t crc          = 0;
    while (*msg) {
      if ((*msg == '*') || (sent_trailer > 0))
        sent_trailer++;
      else
        crc ^= *msg;
      device->print( *msg++ );
    }

    if (!sent_trailer)
      send_trailer( device, crc );
  }

} // send

//----------------------------------------------------------------

void NMEAGPS::send_P( Stream *device, const __FlashStringHelper *msg )
{
  if (msg) {
    const char *ptr = (const char *)msg;
          char  chr = pgm_read_byte(ptr++);

    device->print('$');
    if (chr == '$')
      chr = pgm_read_byte(ptr++);
    uint8_t sent_trailer = 0;
    uint8_t crc          = 0;
    while (chr) {
      if ((chr == '*') || (sent_trailer > 0))
        sent_trailer++;
      else
        crc ^= chr;
      device->print( chr );

      chr = pgm_read_byte(ptr++);
    }

    if (!sent_trailer)
      send_trailer( device, crc );
  }

} // send_P
