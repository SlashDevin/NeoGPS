#ifndef STREAMERS_H
#define STREAMERS_H

#include <Arduino.h>

#include "Time.h"

extern Stream & trace; // Forward declaration of debug output device

// Note:
//
// If you use the trace object for debug print statements, you will also
// need a definition somewhere, preferably in your .INO.  For example,
//
//   Stream & trace = Serial;  // trace goes to Serial
//

extern Stream & operator <<( Stream & outs, const bool b );
extern Stream & operator <<( Stream & outs, const char c );
extern Stream & operator <<( Stream & outs, const uint16_t v );
extern Stream & operator <<( Stream & outs, const uint32_t v );
extern Stream & operator <<( Stream & outs, const int32_t v );
extern Stream & operator <<( Stream & outs, const uint8_t v );
extern Stream & operator <<( Stream & outs, const __FlashStringHelper *s );

class gps_fix;

/**
 * Print valid fix data to the given stream with the format
 *   "status,dateTime,lat,lon,heading,speed,altitude,satellites,
 *       hdop,vdop,pdop,lat_err,lon_err,alt_err"
 * The "header" above contains the actual compile-time configuration.
 * A comma-separated field will be empty if the data is NOT valid.
 * @param[in] outs output stream.
 * @param[in] fix gps_fix instance.
 * @return iostream.
 */
extern Stream & operator <<( Stream &outs, const gps_fix &fix );

class NMEAGPS;

extern void trace_header();
extern void trace_all( const NMEAGPS &gps, const gps_fix &fix );

#endif