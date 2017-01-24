#ifndef STREAMERS_H
#define STREAMERS_H

#include <Arduino.h>

extern Print & operator <<( Print & outs, const bool b );
extern Print & operator <<( Print & outs, const char c );
extern Print & operator <<( Print & outs, const uint16_t v );
extern Print & operator <<( Print & outs, const uint32_t v );
extern Print & operator <<( Print & outs, const int32_t v );
extern Print & operator <<( Print & outs, const uint8_t v );
extern Print & operator <<( Print & outs, const __FlashStringHelper *s );

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
extern Print & operator <<( Print &outs, const gps_fix &fix );

class NMEAGPS;

extern void trace_header( Print & outs );
extern void trace_all( Print & outs, const NMEAGPS &gps, const gps_fix &fix );

#endif