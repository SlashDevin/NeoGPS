/**
 * @file NMEAGPS.cpp
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
#include "Streamers.h"

#include <Stream.h>

#ifndef CR
  #define CR ((char)13)
#endif
#ifndef LF
  #define LF ((char)10)
#endif

/********************************
 * parseHEX(char a)
 * Parses a single character as HEX and returns byte value.
 */
inline static uint8_t parseHEX(char a)
{
  a |= 0x20; // make it lowercase
  if (('a' <= a) && (a <= 'f'))
      return a - 'a' + 10;
  else
      return a - '0';
}

/**
 * formatHEX(char a)
 * Formats lower nybble of value as HEX and returns character.
 */
static char formatHex( uint8_t val )
{
  val &= 0x0F;
  return (val >= 10) ? ((val - 10) + 'A') : (val + '0');
}

//---------------------------------

inline uint8_t to_binary(uint8_t value)
{
  uint8_t high = (value >> 4);
  uint8_t low = (value & 0x0f);
  return ((high << 3) + (high << 1) + low);
}

//---------------------------------

NMEAGPS::NMEAGPS()
{
  #ifdef NMEAGPS_STATS
    statistics.init();
  #endif

  data_init();

  reset();
}

/*
 * Prepare internal members to receive data from sentence fields.
 */
void NMEAGPS::sentenceBegin()
{
  crc          = 0;
  nmeaMessage  = NMEA_UNKNOWN;
  rxState      = NMEA_RECEIVING_HEADER;
  chrCount     = 0;
  comma_needed( false );
  proprietary  = false;

  #ifdef NMEAGPS_SAVE_TALKER_ID
    talker_id[0] =
    talker_id[1] = 0;
  #endif

  #ifdef NMEAGPS_SAVE_MFR_ID
    mfr_id[0] =
    mfr_id[1] =
    mfr_id[2] = 0;
  #endif
}


/*
 * All fields from a sentence have been parsed.
 */

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

  reset();
}

/**
 * There was something wrong with the sentence.
 */
void NMEAGPS::sentenceInvalid()
{
  // All the values are suspect.  Start over.
  m_fix.valid.init();
  nmeaMessage = NMEA_UNKNOWN;

  reset();
}

/**
 *  The sentence is well-formed, but is an unrecognized type
 */

void NMEAGPS::sentenceUnrecognized()
{
  nmeaMessage = NMEA_UNKNOWN;

  reset();
}

void NMEAGPS::headerReceived()
{
  NMEAGPS_INIT_FIX(m_fix);
  safe       = false;
  fieldIndex = 1;
  chrCount   = 0;
  rxState    = NMEA_RECEIVING_DATA;
}

/**
 * Process one character of an NMEA GPS sentence. 
 */

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
        statistics.crc_errors++;
      #endif
      sentenceInvalid();
    }

  } else if (rxState == NMEA_IDLE) { //---------------------------
    // Reject non-start characters

    res         = DECODE_CHR_INVALID;
    nmeaMessage = NMEA_UNKNOWN;
  }

  return res;
}

/*
 * NMEA Sentence strings
 */
static const char gga[] __PROGMEM =  "GGA";
static const char gll[] __PROGMEM =  "GLL";
static const char gsa[] __PROGMEM =  "GSA";
static const char gst[] __PROGMEM =  "GST";
static const char gsv[] __PROGMEM =  "GSV";
static const char rmc[] __PROGMEM =  "RMC";
static const char vtg[] __PROGMEM =  "VTG";
static const char zda[] __PROGMEM =  "ZDA";

static const char * const std_nmea[] __PROGMEM = {
  gga,
  gll,
  gsa,
  gst,
  gsv,
  rmc,
  vtg,
  zda
};

const NMEAGPS::msg_table_t NMEAGPS::nmea_msg_table __PROGMEM =
  {
    NMEAGPS::NMEA_FIRST_MSG,
    (const msg_table_t *) NULL,
    sizeof(std_nmea)/sizeof(std_nmea[0]),
    std_nmea
  };


NMEAGPS::decode_t NMEAGPS::parseCommand( char c )
{
  if (c == ',') {
    // End of field, did we get a sentence type yet?
    return
      (nmeaMessage == NMEA_UNKNOWN) ?
        DECODE_CHR_INVALID :
        DECODE_COMPLETED;
  }

  if ((chrCount == 0) && (c == 'P')) {
    //  Starting a proprietary message...
    proprietary = true;
    return DECODE_CHR_OK;
  }
  
  uint8_t cmdCount = chrCount;

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

  } else { // standard

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

  for (;;) {
    uint8_t  table_size       = pgm_read_byte( &msgs->size );
    uint8_t  msg_offset       = pgm_read_byte( &msgs->offset );
    decode_t res              = DECODE_CHR_INVALID;
    bool     check_this_table = true;
    uint8_t  entry;

    if (nmeaMessage == NMEA_UNKNOWN)
      // We're just starting
      entry = 0;

    else if ((msg_offset <= nmeaMessage) && (nmeaMessage < msg_offset+table_size))
      // In range of this table, pick up where we left off
      entry = nmeaMessage - msg_offset;

    #ifdef NMEAGPS_DERIVED_TYPES
      else
        check_this_table = false;
    #endif

    if (check_this_table) {
      uint8_t i = entry;

      const char * const *table   = (const char * const *) pgm_read_word( &msgs->table );
      const char *        table_i = (const char *) pgm_read_word( &table[i] );
      
      for (;;) {
        char rc = pgm_read_byte( &table_i[cmdCount] );
        if (c == rc) {
          // ok so far...
          entry = i;
          res = DECODE_CHR_OK;
          break;
        }

        if (c < rc)
          // Alphabetical rejection, check next table
          break;

        // Ok to check another entry in this table
        uint8_t next_msg = i+1;
        if (next_msg >= table_size) {
          // No more entries in this table.
          break;
        }

        //  See if the next entry starts with the same characters.
        const char *table_next = (const char *) pgm_read_word( &table[next_msg] );
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
        msgs = (const msg_table_t *) pgm_read_word( &msgs->previous );
        if (msgs) {
          // Try the current character in the previous table
          continue;
        } // else
          // No more tables, chr is invalid.
      #endif
      
    } else
      //  This entry is good so far.
      nmeaMessage = (nmea_msg_t) (entry + msg_offset);

    return res;
  }

} // parseCommand

//---------------------------------------------

const char *NMEAGPS::string_for( nmea_msg_t msg ) const
{
  if (msg == NMEA_UNKNOWN)
    return (const char *) NULL;

  const msg_table_t *msgs = msg_table();

  for (;;) {
    uint8_t  table_size       = pgm_read_byte( &msgs->size );
    uint8_t  msg_offset       = pgm_read_byte( &msgs->offset );

    if ((msg_offset <= msg) && (msg < msg_offset+table_size)) {
      // In range of this table
      const char * const *table   = (const char * const *) pgm_read_word( &msgs->table );
      return
        (const char *)
          pgm_read_word( &table[ ((uint8_t)msg) - msg_offset ] );
    }
 
    #ifdef NMEAGPS_DERIVED_TYPES
      // Try the previous table
      msgs = (const msg_table_t *) pgm_read_word( &msgs->previous );
      if (msgs)
        continue;
    #endif

    return (const char *) NULL;
  }

} // string_for

//---------------------------------------------

bool NMEAGPS::parseField(char chr)
{
    switch (nmeaMessage) {

      case NMEA_GGA: return parseGGA( chr );
      case NMEA_GLL: return parseGLL( chr );
      case NMEA_GSA: return parseGSA( chr );
      case NMEA_GST: return parseGST( chr );
      case NMEA_GSV: return parseGSV( chr );
      case NMEA_RMC: return parseRMC( chr );
      case NMEA_VTG: return parseVTG( chr );
      case NMEA_ZDA: return parseZDA( chr );

      default:
          break;
    }

    return true;

} // parseField

//---------------------------------

bool NMEAGPS::parseGGA( char chr )
{
  #ifdef NMEAGPS_PARSE_GGA
    switch (fieldIndex) {
        case 1: return parseTime( chr );
        PARSE_LOC(2);
        case 6: return parseFix( chr );
        case 7: return parseSatellites( chr );
        case 8: return parseHDOP( chr );
        case 9: return parseAlt( chr );
    }
  #endif

  return true;

} // parseGGA

//---------------------------------

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

//---------------------------------

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

      #ifdef NMEAGPS_PARSE_SATELLITES

        // It's not clear how this sentence relates to GSV.  GSA
        // only allows 12 satellites, while GSV allows any number.  
        // In the absence of guidance, GSV shall have priority 
        // over GSA with repect to populating the satellites
        // array.  Ignore the satellite fields if GSV is enabled.

        #ifndef NMEAGPS_PARSE_GSV

          case 1: break; // allows "default:" case for SV fields
          case 3:
            if (chrCount == 0) {
              NMEAGPS_INVALIDATE( satellites );
              m_fix.satellites = 0;
              sat_count = 0;
            }
          default:
            if (chr == ',') {
              if (chrCount > 0) {
                m_fix.valid.satellites = true;
                m_fix.satellites++;
                sat_count = m_fix.satellites;
              }
            } else
              parseInt( satellites[m_fix.satellites].id, chr );
            break;
        #endif
      #endif
    }
  #endif

  return true;

} // parseGSA

//---------------------------------

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

//---------------------------------

bool NMEAGPS::parseGSV( char chr )
{
  #ifdef NMEAGPS_PARSE_GSV

    switch (fieldIndex) {
        case 3: return parseSatellites( chr );

        #ifdef NMEAGPS_PARSE_SATELLITES
          case 1:
            // allows "default:" case for SV fields
            break;
          case 2: // GSV message number (e.g., 2nd of n)
            if (chr != ',')
              // sat_count is temporarily used to hold the MsgNo...
              parseInt( sat_count, chr );
            else
              // ...then it's converted to the real sat_count
              // based on up to 4 satellites per msg.
              sat_count = (sat_count - 1) * 4;
            break;

          default:
            if (sat_count < NMEAGPS_MAX_SATELLITES) {

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
        #endif
    }
  #endif

  return true;

} // parseGSV

//---------------------------------

bool NMEAGPS::parseRMC( char chr )
{
  #ifdef NMEAGPS_PARSE_RMC
    switch (fieldIndex) {
        case 1: return parseTime( chr );
        case 2: return parseFix( chr );
        PARSE_LOC(3);
        case 7: return parseSpeed( chr );
        case 8: return parseHeading( chr );
        case 9: return parseDDMMYY( chr );
        case 12: return parseFix( chr );
    }
  #endif

  return true;

} // parseRMC

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

/**
 * Parse lat/lon dddmm.mmmm fields
 */

bool NMEAGPS::parseDDDMM( int32_t & val, char chr )
{
  #ifdef GPS_FIX_LOCATION

    if (chrCount == 0) {
      val          = 0;
      decimal      = 0;
      comma_needed( true );
    }
    
    if ((chr == '.') || ((chr == ',') && !decimal)) {
      // Now we know how many digits are in degrees; all but the last two.
      // Switch from BCD (digits) to binary minutes.
      decimal = 1;
      uint8_t *valBCD = (uint8_t *) &val;
      uint8_t  deg     = to_binary( valBCD[1] );
      if (valBCD[2] != 0)
        deg += 100; // only possible if abs(longitude) >= 100.0 degrees
      // Convert val to minutes
      val = (deg * 60) + to_binary( valBCD[0] );
      if (chr == '.') return true;
    }
    
    if (chr == ',') {
      if (val) {
        // If the last chars in ".mmmm" were not received,
        //    force the value into its final state.
        if (decimal == 4)
          val *= 100;
        else if (decimal == 5)
          val *= 10;
        else if (decimal >= 6)
          ;
        else if (decimal == 3)
          val *= 1000;
        else if (decimal == 2)
          val *= 10000;
        else if (decimal == 1)
          val *= 100000;

        // Value was in minutes x 1000000, convert to degrees x 10000000.
        val += (val*2 + 1)/3; // aka (100*val+30)/60, but without sign truncation
      }
    } else if (!decimal) {
      // val is BCD until *after* decimal point
      val = (val<<4) | (chr - '0');
    } else if (decimal++ < 6) {
      val = val*10 + (chr - '0');
    }
  #endif

  return true;

} // parseDDDMM

//---------------------------------

bool NMEAGPS::parseLat( char chr )
{
  #ifdef GPS_FIX_LOCATION
    if (chrCount == 0) {
      group_valid = (chr != ',');
      if (group_valid)
        NMEAGPS_INVALIDATE( location );
    }

    if (group_valid)
      parseDDDMM( m_fix.lat, chr );
  #endif

  return true;
}

bool NMEAGPS::parseNS( char chr )
{
  #ifdef GPS_FIX_LOCATION
    if (group_valid && (chr == 'S'))
      m_fix.lat = -m_fix.lat;
  #endif

  return true;
}

bool NMEAGPS::parseLon( char chr )
{
  #ifdef GPS_FIX_LOCATION
    if ((chr == ',') && (chrCount == 0))
      group_valid = false;

    if (group_valid)
      parseDDDMM( m_fix.lon, chr );
  #endif

  return true;
}

bool NMEAGPS::parseEW( char chr )
{
  #ifdef GPS_FIX_LOCATION
    if (group_valid) {
      if (chr == 'W')
        m_fix.lon = -m_fix.lon;

      m_fix.valid.location = true;
    }
  #endif
  
  return true;
}

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

//---------------------------------

bool NMEAGPS::parseSatellites( char chr )
{
  #ifdef GPS_FIX_SATELLITES
    if (chrCount == 0)
      NMEAGPS_INVALIDATE( satellites );
    if (parseInt( m_fix.satellites, chr ))
      m_fix.valid.satellites = true;
  #endif

  return true;

} // parseSatellites

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

//---------------------------------

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

//---------------------------------

void NMEAGPS::poll( Stream *device, nmea_msg_t msg )
{
  //  Only the ublox documentation references talker ID "EI".  
  //  Other manufacturer's devices use "II" and "GP" talker IDs for the GPQ sentence.
  //  However, "GP" is reserved for the GPS device, so it seems inconsistent
  //  to use that talker ID when requesting something from the GPS device.
  static const char pm0[] __PROGMEM = "EIGPQ,GGA";
  static const char pm1[] __PROGMEM = "EIGPQ,GLL";
  static const char pm2[] __PROGMEM = "EIGPQ,GSA";
  static const char pm3[] __PROGMEM = "EIGPQ,GST";
  static const char pm4[] __PROGMEM = "EIGPQ,GSV";
  static const char pm5[] __PROGMEM = "EIGPQ,RMC";
  static const char pm6[] __PROGMEM = "EIGPQ,VTG";
  static const char pm7[] __PROGMEM = "EIGPQ,ZDA";
  static const char * const poll_msgs[] __PROGMEM = { pm0, pm1, pm2, pm3, pm4, pm5, pm6, pm7 };

  if ((NMEA_FIRST_MSG <= msg) && (msg <= NMEA_LAST_MSG))
    send_P( device, (str_P) pgm_read_word(&poll_msgs[msg-NMEA_FIRST_MSG]) );

} // poll

//---------------------------------

static void send_trailer( Stream *device, uint8_t crc )
{
  device->print('*');

  char hexDigit = formatHex( crc>>4 );
  device->print( hexDigit );

  hexDigit = formatHex( crc );
  device->print( hexDigit );

  device->print( CR );
  device->print( LF );
}


void NMEAGPS::send( Stream *device, const char *msg )
{
  if (msg && *msg) {
    device->print('$');
    if (*msg == '$')
      msg++;
    uint8_t sent_trailer = 0;
    uint8_t crc = 0;
    while (*msg) {
      crc ^= *msg;
      if (*msg == '*' || (sent_trailer > 0))
        sent_trailer++;
      device->print( *msg++ );
    }

    if (sent_trailer != 3)
      send_trailer( device, crc );
  }

} // send

//---------------------------------

void NMEAGPS::send_P( Stream *device, str_P msg )
{
  if (msg) {
    const char *ptr = (const char *)msg;
    char chr = pgm_read_byte(ptr++);
    if (chr && (chr != '$'))
      device->print('$');
    uint8_t sent_trailer = 0;
    uint8_t crc = 0;
    while (chr) {
      crc ^= chr;
      if ((chr == '*') || (sent_trailer > 0))
        sent_trailer++;
      device->print( chr );
      chr = pgm_read_byte(ptr++);
    }

    if (sent_trailer != 3)
      send_trailer( device, crc );
  }

} // send_P
