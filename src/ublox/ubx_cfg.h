#ifndef UBX_CFG_H
#define UBX_CFG_H

//--------------------------------------------------------------------
// Enable/disable the parsing of specific UBX messages.
//
// Configuring out a message prevents its fields from being parsed.
// However, the message type will still be recognized by /decode/ and 
// stored in member /rx_msg/.  No valid flags would be available.

#define UBLOX_PARSE_STATUS
#define UBLOX_PARSE_TIMEGPS
#define UBLOX_PARSE_TIMEUTC
#define UBLOX_PARSE_POSLLH
//#define UBLOX_PARSE_DOP
#define UBLOX_PARSE_VELNED
//#define UBLOX_PARSE_SVINFO
//#define UBLOX_PARSE_CFGNAV5
//#define UBLOX_PARSE_MONVER

//--------------------------------------------------------------------
// Identify the last UBX message in an update interval.
//    (There are two parts to a UBX message, the class and the ID.)
// For coherency, you must determine which UBX message is last!
// This section *should* pick the correct last UBX message for NEO-6M devices.

#define UBX_LAST_MSG_CLASS_IN_INTERVAL ublox::UBX_NAV

#if defined(UBLOX_PARSE_DOP)
  #define UBX_LAST_MSG_ID_IN_INTERVAL    ublox::UBX_NAV_DOP
#elif defined(UBLOX_PARSE_VELNED)
  #define UBX_LAST_MSG_ID_IN_INTERVAL    ublox::UBX_NAV_VELNED
#elif defined(UBLOX_PARSE_POSLLH)
  #define UBX_LAST_MSG_ID_IN_INTERVAL    ublox::UBX_NAV_POSLLH
#elif defined(UBLOX_PARSE_SVINFO)
  #define UBX_LAST_MSG_ID_IN_INTERVAL    ublox::UBX_NAV_SVINFO
#else
  #define UBX_LAST_MSG_ID_IN_INTERVAL    ublox::UBX_NAV_STATUS
#endif

#endif