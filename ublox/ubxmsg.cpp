#include "ubxGPS.h"

using namespace ublox;

bool ublox::configNMEA( ubloxGPS &gps, NMEAGPS::nmea_msg_t msgType, uint8_t rate )
{
  static const ubx_nmea_msg_t ubx[] __PROGMEM = {
        UBX_GPGGA,
        UBX_GPGLL,
        UBX_GPGSA,
        UBX_GPGSV,
        UBX_GPRMC,
        UBX_GPVTG,
        UBX_GPZDA,
    };

  uint8_t msg_index = (uint8_t) msgType - (uint8_t) NMEAGPS::NMEA_FIRST_MSG;

  if (msg_index >= sizeof(ubx)/sizeof(ubx[0]))
    return false;

  msg_id_t msg_id = (msg_id_t) pgm_read_byte( &ubx[msg_index] );

  return gps.send( cfg_msg_t( UBX_NMEA, msg_id, rate ) );
}
