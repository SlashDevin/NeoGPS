#ifndef GPSFIX_H
#define GPSFIX_H

/**
 * @file GPSfix.h
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

#include "Time.h"
#include "GPSfix_cfg.h"

/**
 * A structure for holding a GPS fix: time, position, velocity, etc.
 *
 * Because GPS devices report various subsets of a coherent fix, 
 * this class tracks which members of the fix are being reported: 
 * each part has its own validity flag. Also, operator |= implements 
 * merging multiple reports into one consolidated report.
 *
 * @section Limitations
 * Reports are not really fused with an algorithm; if present in 
 * the source, they are simply replaced in the destination.
 *
 */

class gps_fix {
public:

  gps_fix() { init(); };

  /**
   * A structure for holding the two parts of a floating-point number.
   * This is used for Altitude, Heading and Speed, which require more
   * significant digits than a 16-bit number.  The decimal point is
   * used as a field separator for these two parts.  This is more efficient
   * than calling the 32-bit math subroutines on a single scaled long integer.
   * This requires knowing the exponent on the fraction when a simple type
   * (e.g., float or int) is needed.
   */

  struct whole_frac {
    int16_t whole;
    int16_t frac;
    void init() { whole = 0; frac = 0; };
    int32_t int32_00() const { return ((int32_t)whole) * 100L + frac; };
    int16_t int16_00() const { return whole * 100 + frac; };
    int32_t int32_000() const { return whole * 1000L + frac; };
    float float_00() const { return ((float)whole) + ((float)frac)*0.01; };
    float float_000() const { return ((float)whole) + ((float)frac)*0.001; };
  } NEOGPS_PACKED;

#ifdef GPS_FIX_LOCATION
    int32_t       lat;  // degrees * 1e7, negative is South
    int32_t       lon;  // degrees * 1e7, negative is West

    int32_t latitudeL() const { return lat; };
    float   latitude () const { return ((float) lat) * 1.0e-7; }; // accuracy loss

    int32_t longitudeL() const { return lon; };
    float   longitude () const { return ((float) lon) * 1.0e-7; }; // accuracy loss
#endif

#ifdef GPS_FIX_ALTITUDE
    whole_frac    alt; // .01 meters

    int32_t altitude_cm() const { return alt.int32_00(); };
    float   altitude   () const { return alt.float_00(); };
#endif

#ifdef GPS_FIX_SPEED
    whole_frac    spd; // .001 nautical miles per hour

    uint32_t speed_mkn() const { return spd.int32_000(); };
    float    speed    () const { return spd.float_000(); };
#endif

#ifdef GPS_FIX_HEADING
    whole_frac    hdg; //  .01 degrees

    uint16_t heading_cd() const { return hdg.int16_00(); };
    float    heading   () const { return hdg.float_00(); };
#endif

 /**
   * Dilution of Precision is a measure of the current satellite
   * constellation geometry WRT how 'good' it is for determining a position.  This
   * is _independent_ of signal strength and many other factors that may be
   * internal to the receiver.  It _cannot_ be used to determine position accuracy
   * in meters.
   */
#ifdef GPS_FIX_HDOP
  uint16_t           hdop; // Horizontal Dilution of Precision x 1000
#endif
#ifdef GPS_FIX_VDOP
  uint16_t           vdop; // Horizontal Dilution of Precision x 1000
#endif
#ifdef GPS_FIX_PDOP
  uint16_t           pdop; // Horizontal Dilution of Precision x 1000
#endif

#ifdef GPS_FIX_LAT_ERR
  uint16_t lat_err_cm;
  float lat_err() const { return lat_err_cm / 100.0; }
#endif

#ifdef GPS_FIX_LON_ERR
  uint16_t lon_err_cm;
  float lon_err() const { return lon_err_cm / 100.0; }
#endif

#ifdef GPS_FIX_ALT_ERR
  uint16_t alt_err_cm;
  float alt_err() const { return alt_err_cm / 100.0; }
#endif

#ifdef GPS_FIX_SATELLITES
  uint8_t   satellites;
#endif

#if defined(GPS_FIX_DATE) | defined(GPS_FIX_TIME)
  NeoGPS::time_t  dateTime;
  uint8_t dateTime_cs; // hundredths of a second
#endif

  /**
   * The current fix status or mode of the GPS device.  Unfortunately, the NMEA
   * sentences are a little inconsistent in their use of "status" and "mode".
   * Both fields are mapped onto this enumerated type.  Be aware that
   * different manufacturers interpret them differently.  This can cause 
   * problems in sentences which include both types (e.g., GPGLL).
   *
   * Note: Sorted by increasing accuracy.  See also /operator |=/.
   */
   
  enum status_t {
    STATUS_NONE,
    STATUS_EST,
    STATUS_TIME_ONLY,
    STATUS_STD,
    STATUS_DGPS
  };

  status_t  status NEOGPS_BF(8);

  //  Flags to indicate which members of this fix are valid.

  struct valid_t {
    bool status NEOGPS_BF(1);

#if defined(GPS_FIX_DATE)
    bool date NEOGPS_BF(1);
#endif

#if defined(GPS_FIX_TIME)
    bool time NEOGPS_BF(1);
#endif

#ifdef GPS_FIX_LOCATION
    bool location NEOGPS_BF(1);
#endif

#ifdef GPS_FIX_ALTITUDE
    bool altitude NEOGPS_BF(1);
#endif

#ifdef GPS_FIX_SPEED
    bool speed NEOGPS_BF(1);
#endif

#ifdef GPS_FIX_HEADING
    bool heading NEOGPS_BF(1);
#endif

#ifdef GPS_FIX_SATELLITES
    bool satellites NEOGPS_BF(1);
#endif

#ifdef GPS_FIX_HDOP
    bool hdop NEOGPS_BF(1);
#endif
#ifdef GPS_FIX_VDOP
    bool vdop NEOGPS_BF(1);
#endif
#ifdef GPS_FIX_PDOP
    bool pdop NEOGPS_BF(1);
#endif

#ifdef GPS_FIX_LAT_ERR
    bool lat_err NEOGPS_BF(1);
#endif

#ifdef GPS_FIX_LON_ERR
    bool lon_err NEOGPS_BF(1);
#endif

#ifdef GPS_FIX_ALT_ERR
    bool alt_err NEOGPS_BF(1);
#endif

    void init()
      {
        uint8_t *all = (uint8_t *) this;
        for (uint8_t i=0; i<sizeof(*this); i++)
          *all++ = 0;
      }

    void operator |=( const valid_t & r )
      {
        uint8_t *all = (uint8_t *) this;
        const uint8_t *r_all = (const uint8_t *) &r;
        for (uint8_t i=0; i<sizeof(*this); i++)
          *all++ |= *r_all++;
      }
  } NEOGPS_PACKED
      valid;

  /*
   *  Initialize a fix.  All configured members are set to zero.
   */
  void init()
  {
#ifdef GPS_FIX_LOCATION
    lat = lon = 0;
#endif

#ifdef GPS_FIX_ALTITUDE
    alt.init();
#endif

#ifdef GPS_FIX_SPEED
    spd.init();
#endif

#ifdef GPS_FIX_HEADING
    hdg.init();
#endif

#ifdef GPS_FIX_HDOP
    hdop = 0;
#endif
#ifdef GPS_FIX_VDOP
    vdop = 0;
#endif
#ifdef GPS_FIX_PDOP
    pdop = 0;
#endif

#ifdef GPS_FIX_LAT_ERR
    lat_err_cm = 0;
#endif
#ifdef GPS_FIX_LON_ERR
    lon_err_cm = 0;
#endif
#ifdef GPS_FIX_ALT_ERR
    alt_err_cm = 0;
#endif

#ifdef GPS_FIX_SATELLITES
    satellites = 0;
#endif

    dateTime.init();
    dateTime_cs = 0;
    
    status = STATUS_NONE;

    valid.init();
  };

    /**
     * Merge valid fields from the right fix into a "fused" fix 
     * on the left (i.e., /this/).
     */

    gps_fix & operator |=( const gps_fix & r )
    {
      // Replace /status/  only if the right is more "accurate".
      if (r.valid.status && (!valid.status || (status < r.status)))
        status = r.status;

#ifdef GPS_FIX_DATE
      if (r.valid.date) {
        dateTime.date  = r.dateTime.date;
        dateTime.month = r.dateTime.month;
        dateTime.year  = r.dateTime.year;
      }
#endif

#ifdef GPS_FIX_TIME
      if (r.valid.time) {
        dateTime.hours   = r.dateTime.hours;
        dateTime.minutes = r.dateTime.minutes;
        dateTime.seconds = r.dateTime.seconds;
        dateTime_cs      = r.dateTime_cs;
      }
#endif

#ifdef GPS_FIX_LOCATION
      if (r.valid.location) {
        lat = r.lat;
        lon = r.lon;
      }
#endif

#ifdef GPS_FIX_ALTITUDE
      if (r.valid.altitude)
        alt = r.alt;
#endif

#ifdef GPS_FIX_HEADING
      if (r.valid.heading)
        hdg = r.hdg;
#endif

#ifdef GPS_FIX_SPEED
      if (r.valid.speed)
        spd = r.spd;
#endif

#ifdef GPS_FIX_SATELLITES
      if (r.valid.satellites)
        satellites = r.satellites;
#endif

#ifdef GPS_FIX_HDOP
      if (r.valid.hdop)
        hdop = r.hdop;
#endif

#ifdef GPS_FIX_VDOP
      if (r.valid.vdop)
        vdop = r.vdop;
#endif

#ifdef GPS_FIX_PDOP
      if (r.valid.pdop)
        pdop = r.pdop;
#endif

#ifdef GPS_FIX_LAT_ERR
      if (r.valid.lat_err)
        lat_err_cm = r.lat_err_cm;
#endif

#ifdef GPS_FIX_LON_ERR
      if (r.valid.lon_err)
        lon_err_cm = r.lon_err_cm;
#endif

#ifdef GPS_FIX_ALT_ERR
      if (r.valid.alt_err)
        alt_err_cm = r.alt_err_cm;
#endif

      // Update all the valid flags
      valid |= r.valid;

      return *this;
    }

} NEOGPS_PACKED;

#endif