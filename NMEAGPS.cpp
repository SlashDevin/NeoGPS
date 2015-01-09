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

#ifdef NMEAGPS_ACCUMULATE_FIX
#define INVALIDATE(m) m_fix.valid.m = false;
#define INIT_FIX(m)
#else
#define INVALIDATE(m)
#define INIT_FIX(m) m.valid.init()
#endif

/**
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

/*
 * Prepare internal members to receive data from sentence fields.
 */
void NMEAGPS::sentenceBegin()
{
  crc          = 0;
  nmeaMessage  = NMEA_UNKNOWN;
  rxState      = NMEA_RECEIVING_DATA;
  fieldIndex   = 0;
  chrCount     = 0;
  comma_needed = false;
}


/*
 * All fields from a sentence have been parsed.
 */

void NMEAGPS::sentenceOk()
{
  rxState = NMEA_IDLE;

  // Terminate the last field with a comma if the parser needs it.
  if (comma_needed) {
    comma_needed = false;
    chrCount++;
    parseField(',');
  }

  safe = true;

#ifdef NMEAGPS_STATS
  statistics.ok++;
#endif
}


void NMEAGPS::sentenceInvalid()
{
  rxState = NMEA_IDLE;

  // There was something wrong with the sentence,
  // all the values are suspect.  Start over.
  m_fix.valid.init();
  nmeaMessage = NMEA_UNKNOWN;
}


void NMEAGPS::sentenceUnrecognized()
{
  rxState     = NMEA_IDLE;
  nmeaMessage = NMEA_UNKNOWN;
}

NMEAGPS::decode_t NMEAGPS::decode( char c )
{
  decode_t res = DECODE_CHR_OK;

  if (c == '$') {  // Always restarts
    sentenceBegin();

  } else {
    switch (rxState) {
      case NMEA_IDLE: //---------------------------
          // Reject non-start characters

          res         = DECODE_CHR_INVALID;
          nmeaMessage = NMEA_UNKNOWN;
          break;

      case NMEA_RECEIVING_DATA: //---------------------------
          // Receive complete sentence

          if (c == '*') {   // Line finished, CRC follows
              rxState = NMEA_RECEIVING_CRC;
              chrCount = 0;

          } else if ((c == CR) || (c == LF)) { // Line finished, no CRC
              sentenceOk();
              res = DECODE_COMPLETED;

          } else if ((c < ' ') || ('~' < c)) { // Invalid char
              sentenceInvalid();
              res = DECODE_CHR_INVALID;

          } else {            // normal data character

              crc ^= c;  // accumulate CRC as the chars come in...

              if (fieldIndex == 0) {
                //  The first field is the sentence type.  It will be used later
                //  by the virtual /parseField/
                decode_t cmd_res = parseCommand( c );
                if (cmd_res == DECODE_COMPLETED) {
                  INIT_FIX(m_fix);
                  safe = false;
                } else if (cmd_res == DECODE_CHR_INVALID) {
                  sentenceUnrecognized();
                }

              } else if (!parseField( c )) {
                sentenceInvalid();
              }

              if (c == ',') {
                // Start the next field
                comma_needed = false;
                fieldIndex++;
                chrCount     = 0;
              } else
                chrCount++;
          }
          break;
          
          
      case NMEA_RECEIVING_CRC: //---------------------------
        {
          bool err;
          uint8_t nybble = parseHEX( c );
          if (chrCount == 0) {
            chrCount++;
            err = ((crc >> 4) != nybble);
          } else { // == 1
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
        }
        break;
    }
  }

  return res;
}

/*
 * NMEA Sentence strings
 */
static const char gpgga[] __PROGMEM =  "GPGGA";
static const char gpgll[] __PROGMEM =  "GPGLL";
static const char gpgsa[] __PROGMEM =  "GPGSA";
static const char gpgst[] __PROGMEM =  "GPGST";
static const char gpgsv[] __PROGMEM =  "GPGSV";
static const char gprmc[] __PROGMEM =  "GPRMC";
static const char gpvtg[] __PROGMEM =  "GPVTG";
static const char gpzda[] __PROGMEM =  "GPZDA";

const char * const NMEAGPS::std_nmea[] __PROGMEM = {
  gpgga,
  gpgll,
  gpgsa,
  gpgst,
  gpgsv,
  gprmc,
  gpvtg,
  gpzda
};

const NMEAGPS::msg_table_t NMEAGPS::nmea_msg_table __PROGMEM =
  {
    NMEAGPS::NMEA_FIRST_MSG,
    (const msg_table_t *) NULL,
    sizeof(NMEAGPS::std_nmea)/sizeof(NMEAGPS::std_nmea[0]),
    NMEAGPS::std_nmea
  };


NMEAGPS::decode_t NMEAGPS::parseCommand( char c )
{
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
    else
      // Try the next table
      check_this_table = false;

    if (check_this_table) {
      uint8_t i = entry;

      const char * const *table   = (const char * const *) pgm_read_word( &msgs->table );
      const char *        table_i = (const char *) pgm_read_word( &table[i] );
      
      for (;;) {
        char rc = pgm_read_byte( &table_i[chrCount] );
        if (c == rc) {
          // ok so far...
          entry = i;
          res = DECODE_CHR_OK;
          break;
        }

        if ((c == ',') && (rc == 0)) {
          // End of string and it still matches:  it's this one!
          res = DECODE_COMPLETED;
          break;
        }

        // Mismatch, check another entry
        uint8_t next_msg = i+1;
        if (next_msg >= table_size) {
          // No more entries in this table.
          break;
        }

        //  See if the next entry starts with the same characters.
        const char *table_next = (const char *) pgm_read_word( &table[next_msg] );
        for (uint8_t j = 0; j < chrCount; j++)
          if (pgm_read_byte( &table_i[j] ) != pgm_read_byte( &table_next[j] )) {
            // Nope, a different start to this entry
            break;
          }
        i = next_msg;
        table_i = table_next;
      }
    }

    if (res == DECODE_CHR_INVALID) {
      msgs = (const msg_table_t *) pgm_read_word( &msgs->previous );
      if (msgs) {
        // Try the current character in the previous table
        continue;
      } // else
        // No more tables, chr is invalid.
      
    } else
      //  This entry is good so far.
      nmeaMessage = (nmea_msg_t) (entry + msg_offset);

    return res;
  }
}

//---------------------------------------------

bool NMEAGPS::parseField(char chr)
{
    bool ok = true;
    switch (nmeaMessage) {

        case NMEA_GGA:
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
          break;

        case NMEA_GLL:
#ifdef NMEAGPS_PARSE_GLL
          switch (fieldIndex) {
              PARSE_LOC(1);
              case 5: return parseTime( chr );
//            case 6:  duplicate info
              case 7: return parseFix( chr );
          }
#endif
          break;

        case NMEA_GSA:
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

              // It's not clear how this sentence relates to GSV.  GSA only
              // only allows 12 satellites, while GSV allows any number.  
              // In the absence of guidance, GSV shall have priority over GSA 
              // with repect to populating the satellites array.  Ignore the
              // satellite fields if GSV is enabled.

#ifndef NMEAGPS_PARSE_GSV

              case 1: break; // allows "default:" case for SV fields
              case 3:
                if (chrCount == 0) {
                  INVALIDATE( satellites );
                  m_fix.satellites = 0;
                  sat_index = 0;
                }
              default:
                if (chr == ',') {
                  if (chrCount > 0) {
                    m_fix.valid.satellites = true;
                    m_fix.satellites++;
                    sat_index = m_fix.satellites;
                  }
                } else
                  parseInt( satellites[m_fix.satellites].id, chr );
                break;
#endif
#endif
          }
#endif
          break;

        case NMEA_GST:
#ifdef NMEAGPS_PARSE_GST
          switch (fieldIndex) {
            case 1: return parseTime( chr );
            case 6: return parse_lat_err( chr );
            case 7: return parse_lon_err( chr );
            case 8: return parse_alt_err( chr );
          }
#endif
          break;

        case NMEA_GSV:
#ifdef NMEAGPS_PARSE_GSV
          switch (fieldIndex) {
              case 3: return parseSatellites( chr );
#ifdef NMEAGPS_PARSE_SATELLITES
              case 1:
                // allows "default:" case for SV fields
                break;
              case 2: // GSV message number (e.g., 2nd of n)
                if (chr != ',')
                  // sat_index is temporarily used to hold the MsgNo...
                  parseInt( sat_index, chr );
                else {
                  // ...then it's converted to the real sat_index
                  // based on up to 4 satellites per msg.
                  sat_index = (sat_index - 1) * 4;
                  if (sat_index == 0)
                    for (uint8_t i=0; i<MAX_SATELLITES; i++)
                      satellites[i].id = 0;
                }
                break;
              default:
                if (sat_index < MAX_SATELLITES) {
                  switch (fieldIndex % 4) {
#ifdef NMEAGPS_PARSE_SATELLITE_INFO
                    case 0: parseInt( satellites[sat_index].id       , chr ); break;
                    case 1: parseInt( satellites[sat_index].elevation, chr ); break;
                    case 2:
                      if (chr != ',')
                        parseInt( satellites[sat_index].azimuth, chr );
                      else
                        sat_index++; // field 3 can be omitted, increment now
                      break;
                    case 3:
                      if (chr != ',')
                        parseInt( satellites[sat_index-1].snr, chr );
                      else
                        satellites[sat_index-1].tracked = (chrCount != 0);
                      break;
#else
                    case 0:
                      if (chr != ',')
                        parseInt( satellites[sat_index].id, chr );
                      else
                        sat_index++;
                      break;
#endif
                  }
              }
#endif
          }
#endif
          break;
                  
        case NMEA_RMC:
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
          break;

        case NMEA_VTG:
#ifdef NMEAGPS_PARSE_VTG
          switch (fieldIndex) {
              case 1: return parseHeading( chr );
              case 5: return parseSpeed( chr );
              case 9: return parseFix( chr );
          }
#endif
          break;

        case NMEA_ZDA:
#ifdef NMEAGPS_PARSE_ZDA
          switch (fieldIndex) {
            case 1: return parseTime( chr );
#ifdef GPS_FIX_DATE
            case 2:
              if (chrCount == 0)
                INVALIDATE( date );
              parseInt( m_fix.dateTime.date , chr );
              break;
            case 3: parseInt( m_fix.dateTime.month, chr ); break;
            case 4:
              if (chr != ',')
                parseInt( m_fix.dateTime.year, chr );
              else
                m_fix.valid.date = true;
              break;
#endif
          }
#endif
          break;

        default:
            break;
    }

    return ok;
}

//---------------------------------

bool NMEAGPS::parseTime(char chr)
{
#ifdef GPS_FIX_TIME
  switch (chrCount) {
      case 0: INVALIDATE( time )
              comma_needed = true;
              m_fix.dateTime.hours    = (chr - '0')*10; break;
      case 1: m_fix.dateTime.hours   += (chr - '0');    break;
      case 2: m_fix.dateTime.minutes  = (chr - '0')*10; break;
      case 3: m_fix.dateTime.minutes += (chr - '0');    break;
      case 4: m_fix.dateTime.seconds  = (chr - '0')*10; break;
      case 5: m_fix.dateTime.seconds += (chr - '0');    break;
      default:
        if (chr == ',')
          m_fix.valid.time = true;
        else if (chrCount == 7)
          m_fix.dateTime_cs  = (chr - '0')*10;
        else if (chrCount == 8)
          m_fix.dateTime_cs += (chr - '0');
        break;
  }
#endif

  return true;
}

//---------------------------------

bool NMEAGPS::parseDDMMYY( char chr )
{
#ifdef GPS_FIX_DATE
  switch (chrCount) {
    case 0: INVALIDATE( date );
            comma_needed = true;
            m_fix.dateTime.date   = (chr - '0')*10; break;
    case 1: m_fix.dateTime.date  += (chr - '0');    break;
    case 2: m_fix.dateTime.month  = (chr - '0')*10; break;
    case 3: m_fix.dateTime.month += (chr - '0');    break;
    case 4: m_fix.dateTime.year   = (chr - '0')*10; break;
    case 5: m_fix.dateTime.year  += (chr - '0');    break;
    default:
      if (chr == ',')
        m_fix.valid.date = true;
      break;
  }
#endif

  return true;
}

//---------------------------------

bool NMEAGPS::parseFix( char chr )
{
  if (chrCount == 0) {
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
}

//---------------------------------

bool NMEAGPS::parseFloat( gps_fix::whole_frac & val, char chr, uint8_t max_decimal )
{
  bool done = false;
  
  if (chrCount == 0) {
    comma_needed = true;
    val.init();
    decimal = 0;
    negative = (chr == '-');
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
  } else
    comma_needed = false;

  return done;

} // parseFloat

//---------------------------------

bool NMEAGPS::parseFloat( uint16_t & val, char chr, uint8_t max_decimal )
{
  bool done = false;

  if (chrCount == 0) {
    decimal      = 0;
    val          = 0;
    comma_needed = true;
    negative = (chr == '-');
    if (negative) return done;
  }

  if (chr == ',') {
    if (val)
      while (decimal++ <= max_decimal)
        val *= 10;
    done = true;
  } else if (chr == '.')
    decimal = 1;
  else if (decimal++ <= max_decimal)
    val = val*10 + (chr - '0');
  else
    comma_needed = false;

  return done;

} // parseFloat

//---------------------------------

inline uint8_t
to_binary(uint8_t value)
{
  uint8_t high = (value >> 4);
  uint8_t low = (value & 0x0f);
  return ((high << 3) + (high << 1) + low);
}

bool NMEAGPS::parseDDDMM( int32_t & val, char chr )
{
#ifdef GPS_FIX_LOCATION

  // parse lat/lon dddmm.mmmm fields

  if (chrCount == 0) {
    val          = 0;
    decimal      = 0;
    comma_needed = true;
  }
  
  if ((chr == '.') || ((chr == ',') && !decimal)) {
    // Now we know how many digits are in degrees; all but the last two.
    // Switch from BCD (digits) to binary minutes.
    decimal = 1;
    uint8_t *valBCD = (uint8_t *) &val;
    uint8_t  deg     = to_binary( valBCD[1] );
    if (valBCD[2] != 0)
      deg += 100; // only possible if abs(longitude) >= 100.0 degrees
    val = (deg * 60) + to_binary( valBCD[0] );
    // val now in units of minutes
    if (chr == '.') return true;
  }
  
  if (chr == ',') {
    if (val) {
      // If the last chars in ".mmmm" were not received,
      //    force the value into its final state.
      while (decimal++ < 6)
        val *= 10;
      // Value was in minutes x 1000000, convert to degrees x 10000000.
      val += (val*2 + 1)/3; // aka (100*val+30)/60, but without sign truncation
    }
  } else if (!decimal) {
    // val is BCD until *after* decimal point
    val = (val<<4) | (chr - '0');
  } else if (decimal++ < 6) {
    val = val*10 + (chr - '0');
  } else
    comma_needed = false;
#endif

  return true;

} // parseDDDMM

//---------------------------------

bool NMEAGPS::parseLat( char chr )
{
#ifdef GPS_FIX_LOCATION
  if (chrCount == 0) {
    INVALIDATE( location );
    group_valid = (chr != ',');
  }
  parseDDDMM( m_fix.lat, chr );
#endif
  return true;
}

bool NMEAGPS::parseNS( char chr )
{
#ifdef GPS_FIX_LOCATION
  if (chr == 'S')
    m_fix.lat = -m_fix.lat;
  else if (chr == 'N')
    ;
  else if ((chr == ',') && (chrCount == 0))
    group_valid = false;
#endif
  return true;
}

bool NMEAGPS::parseLon( char chr )
{
#ifdef GPS_FIX_LOCATION
  if ((chr == ',') && (chrCount == 0))
    group_valid = false;
  parseDDDMM( m_fix.lon, chr );
#endif
  return true;
}

bool NMEAGPS::parseEW( char chr )
{
#ifdef GPS_FIX_LOCATION
  if (chr == 'W')
    m_fix.lon = -m_fix.lon;
  else if (chr == 'E')
    ;
  else if ((chr == ',') && (chrCount == 0))
    group_valid = false;

  if (group_valid)
    m_fix.valid.location = true;
#endif
  return true;
}

//---------------------------------

bool NMEAGPS::parseSpeed( char chr )
{
#ifdef GPS_FIX_SPEED
  if (chrCount == 0)
    INVALIDATE( speed );
  if (parseFloat( m_fix.spd, chr, 3 )) {
    m_fix.valid.speed = (chrCount != 0);
  }
#endif
  return true;
}

bool NMEAGPS::parseHeading( char chr )
{
#ifdef GPS_FIX_HEADING
  if (chrCount == 0)
    INVALIDATE( heading );
  if (parseFloat( m_fix.hdg, chr, 2 )) {
    m_fix.valid.heading = (chrCount != 0);
  }
#endif
  return true;
}

bool NMEAGPS::parseAlt(char chr )
{
#ifdef GPS_FIX_ALTITUDE
  if (chrCount == 0)
    INVALIDATE( altitude );
  if (parseFloat( m_fix.alt, chr, 2 )) {
    m_fix.valid.altitude = (chrCount != 0);
  }
#endif
  return true;
}

bool NMEAGPS::parseSatellites( char chr )
{
#ifdef GPS_FIX_SATELLITES
  if (chrCount == 0)
    INVALIDATE( satellites );
  if (parseInt( m_fix.satellites, chr ))
    m_fix.valid.satellites = (chrCount != 0);
#endif
  return true;
}

bool NMEAGPS::parseHDOP( char chr )
{
#ifdef GPS_FIX_HDOP
  if (chrCount == 0)
    INVALIDATE( hdop );
  if (parseFloat( m_fix.hdop, chr, 3 ))
    m_fix.valid.hdop = (chrCount != 0);
#endif
  return true;
}

bool NMEAGPS::parseVDOP( char chr )
{
#ifdef GPS_FIX_VDOP
  if (chrCount == 0)
    INVALIDATE( vdop );
  if (parseFloat( m_fix.vdop, chr, 3 ))
    m_fix.valid.vdop = (chrCount != 0);
#endif
  return true;
}

bool NMEAGPS::parsePDOP( char chr )
{
#ifdef GPS_FIX_PDOP
  if (chrCount == 0)
    INVALIDATE( pdop );
  if (parseFloat( m_fix.pdop, chr, 3 ))
    m_fix.valid.pdop = (chrCount != 0);
#endif
  return true;
}

bool NMEAGPS::parse_lat_err( char chr )
{
#ifdef GPS_FIX_lat_ERR
  if (chrCount == 0)
    INVALIDATE( lat_err );
  if (parseFloat( m_fix.lat_err_cm, chr, 2 ))
    m_fix.valid.lat_err = (chrCount != 0);
#endif
  return true;
}

bool NMEAGPS::parse_lon_err( char chr )
{
#ifdef GPS_FIX_lon_ERR
  if (chrCount == 0)
    INVALIDATE( lon_err );
  if (parseFloat( m_fix.lon_err_cm, chr, 2 ))
    m_fix.valid.lon_err = (chrCount != 0);
#endif
  return true;
}

bool NMEAGPS::parse_alt_err( char chr )
{
#ifdef GPS_FIX_alt_ERR
  if (chrCount == 0)
    INVALIDATE( alt_err );
  if (parseFloat( m_fix.alt_err_cm, chr, 2 ))
    m_fix.valid.alt_err = (chrCount != 0);
#endif
  return true;
}

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
}

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
    uint8_t crc = 0;
    while (*msg) {
      crc ^= *msg;
      device->print( *msg++ );
    }

    send_trailer( device, crc );
  }
}

//---------------------------------

void NMEAGPS::send_P( Stream *device, str_P msg )
{
  if (msg) {
    uint8_t crc = 0;
    const char *ptr = (const char *)msg;
    char chr = pgm_read_byte(ptr++);
    if (chr && (chr != '$'))
      device->print('$');
    while (chr) {
      crc ^= chr;
      device->print( chr );
      chr = pgm_read_byte(ptr++);
    }

    send_trailer( device, crc );
  }
}
