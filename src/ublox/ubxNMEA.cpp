#include "ublox/ubxNMEA.h"

// Disable the entire file if derived types are not allowed.
#ifdef NMEAGPS_DERIVED_TYPES

//---------------------------------------------

bool ubloxNMEA::parseField(char chr)
{
  if (nmeaMessage >= (nmea_msg_t) PUBX_FIRST_MSG) {

    switch (nmeaMessage) {

      case PUBX_00: return parsePUBX_00( chr );

      #if defined(NMEAGPS_PARSE_PUBX_04) | defined(NMEAGPS_RECOGNIZE_ALL)
        case PUBX_04: return parsePUBX_04( chr );
      #endif

      default: 
        break;
    }

  } else

    // Delegate
    return NMEAGPS::parseField(chr);


  return true;

} // parseField

//----------------------------

bool ubloxNMEA::parsePUBX_00( char chr )
{
  bool ok = true;

  switch (fieldIndex) {
    case 1:
      // The first field is actually a message subtype
      if (chrCount == 0)
        ok = (chr == '0');

      else if (chrCount == 1) {
        switch (chr) {
          #if defined(NMEAGPS_PARSE_PUBX_00) | defined(NMEAGPS_RECOGNIZE_ALL)
            case '0': break;
          #endif
          #if defined(NMEAGPS_PARSE_PUBX_04) | defined(NMEAGPS_RECOGNIZE_ALL)
            case '4': nmeaMessage = (nmea_msg_t) PUBX_04; break;
          #endif
          default : ok = false;
        }

      } else // chrCount > 1
        ok = (chr == ',');
      break;

    #ifdef NMEAGPS_PARSE_PUBX_00
      case  2: return parseTime( chr );
      PARSE_LOC(3);
      case  7: return parseAlt( chr );
      case  8: return parseFix( chr );
      case 11: return parseSpeed( chr ); // kph!
      case 12: return parseHeading( chr );
      case 15: return parseHDOP( chr );
      case 16: return parseVDOP( chr );
      case 18: return parseSatellites( chr );
    #endif
  }

  return ok;

} // parsePUBX_00

//---------------------------------------------

bool ubloxNMEA::parsePUBX_04( char chr )
{
  #ifdef NMEAGPS_PARSE_PUBX_04
    switch (fieldIndex) {
        case 2: return parseTime( chr );
        case 3: return parseDDMMYY( chr );
    }
  #endif

  return true;

} // parsePUBX_04

//---------------------------------------------

bool ubloxNMEA::parseFix( char chr )
{
  if (chrCount == 0) {
    NMEAGPS_INVALIDATE( status );
    if (chr == 'N')
      m_fix.status = gps_fix::STATUS_NONE;
    else if (chr == 'T')
      m_fix.status = gps_fix::STATUS_TIME_ONLY;
    else if (chr == 'R')
      m_fix.status = gps_fix::STATUS_EST;
    else if (chr == 'G')
      m_fix.status = gps_fix::STATUS_STD;
    else if (chr == 'D')
      m_fix.status = gps_fix::STATUS_DGPS;

  } else if (chrCount == 1) {

    if (((chr == 'T') && (m_fix.status == gps_fix::STATUS_TIME_ONLY)) ||
        ((chr == 'K') && (m_fix.status == gps_fix::STATUS_EST)) ||
        (((chr == '2') || (chr == '3')) &&
         ((m_fix.status == gps_fix::STATUS_STD) ||
          (m_fix.status == gps_fix::STATUS_DGPS))) ||
        ((chr == 'F') && (m_fix.status == gps_fix::STATUS_NONE)))
      // status based on first char was ok guess
      m_fix.valid.status = true;

    else if ((chr == 'R') && (m_fix.status == gps_fix::STATUS_DGPS)) {
      m_fix.status = gps_fix::STATUS_EST; // oops, was DR instead
      m_fix.valid.status = true;
    }
  }
  
  return true;
}

#endif