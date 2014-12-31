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

#include <HardwareSerial.h>

#ifndef CR
#define CR ((char)13)
#endif
#ifndef LF
#define LF ((char)10)
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
void NMEAGPS::rxBegin()
{
    crc = 0;
    nmeaMessage = NMEA_UNKNOWN;
    rxState = NMEA_RECEIVING_DATA;
    fieldIndex = 0;
    chrCount = 0;
}


/*
 * All fields from a sentence have been parsed.
 */
void NMEAGPS::rxEnd( bool ok )
{
  rxState = NMEA_IDLE;

  if (ok) {
    coherent = true;
#ifdef NMEAGPS_STATS
    statistics.parser_ok++;
#endif

  } else {
    m_fix.valid.init();
    nmeaMessage = NMEA_UNKNOWN;
  }
}


NMEAGPS::decode_t NMEAGPS::decode( char c )
{
  decode_t res = DECODE_CHR_OK;

  if (c == '$') {  // Always restarts
    rxBegin();

  } else {
    switch (rxState) {
      case NMEA_IDLE:
          res = DECODE_CHR_INVALID;
          nmeaMessage = NMEA_UNKNOWN;
//trace << 'X' << formatHex(c >> 4) << formatHex(c);
          break;

          // Wait until complete line is received
      case NMEA_RECEIVING_DATA:
          if (c == '*') {   // Line finished, CRC follows
              rxState = NMEA_RECEIVING_CRC;
              chrCount = 0;

          } else if ((c == CR) || (c == LF)) { // Line finished, no CRC
              rxEnd( true );
              res = DECODE_COMPLETED;

          } else if ((c < ' ') || ('~' < c)) { // Invalid char
              rxEnd( false );
              res = DECODE_CHR_INVALID;

          } else {            // normal data character

              crc ^= c;  // accumulate CRC as the chars come in...

              if (fieldIndex == 0) {
                //  The first field is the sentence type.  It will be used later
                //  by the virtual /parseField/
                decode_t cmd_res = parseCommand( c );
                if (cmd_res == DECODE_COMPLETED) {
                  m_fix.valid.init();
                  coherent = false;
                } else if (cmd_res == DECODE_CHR_INVALID) {
                  rxEnd( false );
                }

              } else if (!parseField( c )) {
//trace << PSTR("!pf @ ") << nmeaMessage << PSTR(":") << fieldIndex << PSTR("/") << chrCount << PSTR("'") << c << PSTR("'\n");
                rxEnd( false );
              }

              if (c == ',') {
                // Start the next field
                fieldIndex++;
                chrCount = 0;
              } else
                chrCount++;
          }
          break;
          
          
          // Receiving CRC characters
      case NMEA_RECEIVING_CRC:
        {
          bool err;
          uint8_t nybble = parseHEX( c );
          if (chrCount == 0) {
            chrCount++;
            err = ((crc >> 4) != nybble);
          } else { // == 1
            err = ((crc & 0x0F) != nybble);
            if (!err) {
              rxEnd( true );
              res = DECODE_COMPLETED;
            }
          }
          if (err) {
#ifdef NMEAGPS_STATS
            statistics.parser_crcerr++;
#endif
            rxEnd( false );
          }
        }
        break;
    }
  }

  return res;
}

/*
 * Sentence strings in PROGMEM
 */
static const char gpgga[] __PROGMEM =  "GPGGA";
static const char gpgll[] __PROGMEM =  "GPGLL";
static const char gpgsa[] __PROGMEM =  "GPGSA";
static const char gpgsv[] __PROGMEM =  "GPGSV";
static const char gprmc[] __PROGMEM =  "GPRMC";
static const char gpvtg[] __PROGMEM =  "GPVTG";
static const char gpzda[] __PROGMEM =  "GPZDA";

const char * const NMEAGPS::std_nmea[] __PROGMEM = {
  gpgga,
  gpgll,
  gpgsa,
  gpgsv,
  gprmc,
  gpvtg,
  gpzda
};

const uint8_t NMEAGPS::std_nmea_size = sizeof(std_nmea)/sizeof(std_nmea[0]);

const NMEAGPS::msg_table_t NMEAGPS::nmea_msg_table __PROGMEM =
  {
    NMEAGPS::NMEA_FIRST_MSG,
    (const msg_table_t *) NULL,
    NMEAGPS::std_nmea_size,
    NMEAGPS::std_nmea
  };


NMEAGPS::decode_t NMEAGPS::parseCommand( char c )
{
//static char cmd[8];
//cmd[chrCount] = c;
//cmd[chrCount+1] = 0;
//Serial.print( c );
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
//trace << PSTR(" -> ") << (uint8_t)entry << endl;
          res = DECODE_COMPLETED;
          break;
        }
        // Mismatch, see if the next entry starts with the same characters.
        uint8_t next_msg = i+1;
        if (next_msg >= table_size) {
          // No more entries in this table.
//Serial.print(cmd);
//Serial.println( F(" -> xe") );
          break;
        }
        const char *table_next = (const char *) pgm_read_word( &table[next_msg] );
        for (uint8_t j = 0; j < chrCount; j++)
          if (pgm_read_byte( &table_i[j] ) != pgm_read_byte( &table_next[j] )) {
            // Nope, a different start to this entry
//Serial.print(cmd);
//Serial.println( F(" -> ds") );
            break;
          }
        i = next_msg;
        table_i = table_next;
      }
    }

    if (res == DECODE_CHR_INVALID) {
      msgs = (const msg_table_t *) pgm_read_word( &msgs->previous );
      if (msgs) {
//Serial.print('^');
//trace << '^'; // << hex << msgs << '=' << hex << &nmea_msg_table << ' ';
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
              CASE_TIME(1);
              CASE_LOC(2);
              CASE_FIX(6);
              CASE_SAT(7);
              CASE_HDOP(8);
              CASE_ALT(9);
          }
#endif
          break;

        case NMEA_GLL:
#ifdef NMEAGPS_PARSE_GLL
          switch (fieldIndex) {
              CASE_LOC(1);
              CASE_TIME(5);
//            case 6:  duplicate info
              CASE_FIX(7);
          }
#endif
          break;

        case NMEA_GSA:
        case NMEA_GSV:
            break;
                  
        case NMEA_RMC:
#ifdef NMEAGPS_PARSE_RMC
          switch (fieldIndex) {
              CASE_TIME(1);
              CASE_FIX(2);
              CASE_LOC(3);
              CASE_SPEED(7);
              CASE_HEADING(8);
              CASE_DATE(9);
              CASE_FIX(12);
          }
#endif
          break;

        case NMEA_VTG:
#ifdef NMEAGPS_PARSE_VTG
          switch (fieldIndex) {
              CASE_HEADING(1);
              CASE_SPEED(5);
              CASE_FIX(9);
          }
#endif
          break;

        case NMEA_ZDA:
#ifdef NMEAGPS_PARSE_ZDA
          switch (fieldIndex) {
            CASE_TIME(1);
#ifdef GPS_FIX_DATE
            case 2: parseInt( m_fix.dateTime.Day , chr ); break;
            case 3: parseInt( m_fix.dateTime.Month, chr ); break;
            case 4:
              m_fix.valid.date = parseInt( m_fix.dateTime.Year, chr );
              if (chr == ',')
                m_fix.dateTime.Year = y2kYearToTm( m_fix.dateTime.Year );
              break;
#endif
          }
#endif
          break;

        default:
            ok = false;
            break;
    }

    return ok;
}

#ifdef GPS_FIX_TIME

bool NMEAGPS::parseTime(char chr)
{
  bool ok = true;

  switch (chrCount) {
      case 0: m_fix.dateTime.Hour    = (chr - '0')*10; break;
      case 1: m_fix.dateTime.Hour   += (chr - '0');    break;
      case 2: m_fix.dateTime.Minute  = (chr - '0')*10; break;
      case 3: m_fix.dateTime.Minute += (chr - '0');    break;
      case 4: m_fix.dateTime.Second  = (chr - '0')*10; break;
      case 5: m_fix.dateTime.Second += (chr - '0');    break;
      case 6: if (chr != '.') ok = false;               break;
      case 7: m_fix.dateTime_cs       = (chr - '0')*10; break;
      case 8: m_fix.dateTime_cs      += (chr - '0');    break;
      case 9:
        if (chr == ',')
          m_fix.valid.time = true;
        else
          ok = false;
        break;
  }

  return ok;
}
#endif

#ifdef GPS_FIX_DATE

bool NMEAGPS::parseDDMMYY( char chr )
{
  bool ok = true;
  switch (chrCount) {
    case 0: m_fix.dateTime.Day    = (chr - '0')*10; break;
    case 1: m_fix.dateTime.Day   += (chr - '0');    break;
    case 2: m_fix.dateTime.Month  = (chr - '0')*10; break;
    case 3: m_fix.dateTime.Month += (chr - '0');    break;
    case 4: m_fix.dateTime.Year   = (chr - '0')*10; break;
    case 5: m_fix.dateTime.Year  += (chr - '0');    break;
    case 6:
      if (chr == ',') {
        m_fix.valid.date = true;
        m_fix.dateTime.Year = y2kYearToTm( m_fix.dateTime.Year );
      } else
        ok = false;
      break;
  }
  return ok;
}

#endif

bool NMEAGPS::parseFix( char chr )
{
  bool ok = true;

  if (chrCount == 0) {
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
  } else if (chrCount == 1) {
    if (chr == ',')
      m_fix.valid.status = true;
    else
      ok = false;
  }

  return ok;
}

bool NMEAGPS::parseFloat( gps_fix::whole_frac & val, char chr, uint8_t max_decimal )
{
  if (chrCount == 0) {
    val.init();
    decimal = 0;
    negative = (chr == '-');
    if (negative) return true;
  }

  if (chr == ',') {
    // End of field, make sure it's scaled up
    if (!decimal)
      decimal = 1;
    while (decimal++ <= max_decimal)
      val.frac *= 10;
    if (negative) {
      val.frac = -val.frac;
      val.whole = -val.whole;
    }
  } else if (chr == '.') {
    decimal = 1;
  } else if (!decimal) {
    val.whole = val.whole*10 + (chr - '0');
  } else if (decimal++ <= max_decimal) {
    val.frac = val.frac*10 + (chr - '0');
  }
  return true;
}

#ifdef GPS_FIX_LOCATION

inline uint8_t
to_binary(uint8_t value)
{
  uint8_t high = (value >> 4);
  uint8_t low = (value & 0x0f);
  return ((high << 3) + (high << 1) + low);
}

bool NMEAGPS::parseDDDMM( int32_t & val, char chr )
{
  // parse lat/lon dddmm.mmmm fields

  if (chrCount == 0) {
    val = 0;
    decimal = 0;
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
  }
  return true;
}
#endif

void NMEAGPS::poll( Stream *device, nmea_msg_t msg )
{
  //  Only the ublox documentation references talker ID "EI".  
  //  Other manufacturer's devices use "II" and "GP" talker IDs for the GPQ sentence.
  //  However, "GP" is reserved for the GPS device, so it seems inconsistent
  //  to use that talker ID when requesting something from the GPS device.
  static const char pm0[] __PROGMEM = "EIGPQ,GGA";
  static const char pm1[] __PROGMEM = "EIGPQ,GLL";
  static const char pm2[] __PROGMEM = "EIGPQ,GSA";
  static const char pm3[] __PROGMEM = "EIGPQ,GSV";
  static const char pm4[] __PROGMEM = "EIGPQ,RMC";
  static const char pm5[] __PROGMEM = "EIGPQ,VTG";
  static const char pm6[] __PROGMEM = "EIGPQ,ZDA";
  static const char * const poll_msgs[] __PROGMEM = { pm0, pm1, pm2, pm3, pm4, pm5, pm6 };

  if ((NMEA_FIRST_MSG <= msg) && (msg <= NMEA_LAST_MSG))
    send_P( device, (str_P) pgm_read_word(&poll_msgs[msg-NMEA_FIRST_MSG]) );
}




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

void NMEAGPS::send_P( Stream *device, str_P msg )
{
//Serial.print("NMEAGPS::send \"");
  if (msg) {
    uint8_t crc = 0;
    const char *ptr = (const char *)msg;
    char chr = pgm_read_byte(ptr++);
    if (chr && (chr != '$')) {
//Serial.print( '$' );
      device->print('$');
    }
    while (chr) {
      crc ^= chr;
//Serial.print( chr );
      device->print( chr );
      chr = pgm_read_byte(ptr++);
    }

    send_trailer( device, crc );
  }
//Serial.println( '\"' );
}
