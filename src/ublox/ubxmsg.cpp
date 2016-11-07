#include "ublox/ubxGPS.h"

// Disable the entire file if derived types are not allowed.
#ifdef NMEAGPS_DERIVED_TYPES

using namespace ublox;

bool ublox::configNMEA( ubloxGPS &gps, NMEAGPS::nmea_msg_t msgType, uint8_t rate )
{
  static const ubx_nmea_msg_t ubx[] __PROGMEM = {
      #if defined(NMEAGPS_PARSE_GGA) | defined(NMEAGPS_RECOGNIZE_ALL)
        UBX_GPGGA,
      #endif
        
      #if defined(NMEAGPS_PARSE_GLL) | defined(NMEAGPS_RECOGNIZE_ALL)
        UBX_GPGLL,
      #endif
        
      #if defined(NMEAGPS_PARSE_GSA) | defined(NMEAGPS_RECOGNIZE_ALL)
        UBX_GPGSA,
      #endif
        
      #if defined(NMEAGPS_PARSE_GST) | defined(NMEAGPS_RECOGNIZE_ALL)
        UBX_GPGST,
      #endif
        
      #if defined(NMEAGPS_PARSE_GSV) | defined(NMEAGPS_RECOGNIZE_ALL)
        UBX_GPGSV,
      #endif
        
      #if defined(NMEAGPS_PARSE_RMC) | defined(NMEAGPS_RECOGNIZE_ALL)
        UBX_GPRMC,
      #endif
        
      #if defined(NMEAGPS_PARSE_VTG) | defined(NMEAGPS_RECOGNIZE_ALL)
        UBX_GPVTG,
      #endif
        
      #if defined(NMEAGPS_PARSE_ZDA) | defined(NMEAGPS_RECOGNIZE_ALL)
        UBX_GPZDA,
      #endif
    };

  uint8_t msg_index = (uint8_t) msgType - (uint8_t) NMEAGPS::NMEA_FIRST_MSG;

  if (msg_index >= sizeof(ubx)/sizeof(ubx[0]))
    return false;

  msg_id_t msg_id = (msg_id_t) pgm_read_byte( &ubx[msg_index] );

  return gps.send( cfg_msg_t( UBX_NMEA, msg_id, rate ) );
}

#endif