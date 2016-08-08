#ifndef NEOGPS_LOCATION_H
#define NEOGPS_LOCATION_H

#include <Arduino.h>

#include "NeoGPS_cfg.h"

class NMEAGPS;

namespace NeoGPS {

class Location_t
{
public:
    CONST_CLASS_DATA float LOC_SCALE = 1.0e-7;

    int32_t  lat() const      { return _lat; };
    float    latF() const     { return ((float) lat()) * LOC_SCALE; };
    void     lat( int32_t l ) { _lat = l; };

    int32_t  lon() const { return _lon; };
    float    lonF() const     { return ((float) lon()) * LOC_SCALE; };
    void     lon( int32_t l ) { _lon = l; };

    void init() { _lat = _lon = 0; };

    CONST_CLASS_DATA float EARTH_RADIUS_KM = 6371.0088;
    CONST_CLASS_DATA float RAD_PER_DEG     = PI / 180.0;
    CONST_CLASS_DATA float MI_PER_KM       = 0.621371;

    static float DistanceKm( const Location_t & p1, const Location_t p2 )
      {
        return DistanceRadians( p1, p2 ) * EARTH_RADIUS_KM;
      }

    static float DistanceMiles( const Location_t & p1, const Location_t p2 )
      {
        return DistanceRadians( p1, p2 ) * EARTH_RADIUS_KM * MI_PER_KM;
      }

    static float DistanceRadians( const Location_t & p1, const Location_t p2 )
      {
        // Haversine calculation from http://www.movable-type.co.uk/scripts/latlong.html

              float dLat      = (p2.lat() - p1.lat()) * RAD_PER_DEG * LOC_SCALE;
              float haverDLat = sin(dLat/2.0);
        haverDLat *= haverDLat; // squared
        
        float dLon      = (p2.lon() - p1.lon()) * RAD_PER_DEG * LOC_SCALE;
        float haverDLon = sin(dLon/2.0);
        haverDLon *= haverDLon; // squared
        
        float lat1 = p1.latF();
        lat1 *= RAD_PER_DEG;
        float lat2 = p2.latF();
        lat2 *= RAD_PER_DEG;
        float a = haverDLat + cos(lat1) * cos(lat2) * haverDLon;

        float c = 2 * atan2( sqrt(a), sqrt(1-a) );

        return c;
      }

    static float EquirectDistanceRadians( const Location_t & p1, const Location_t p2 )
      {
        // Equirectangular calculation from http://www.movable-type.co.uk/scripts/latlong.html

        float dLat = (p2.lat() - p1.lat()) * RAD_PER_DEG * LOC_SCALE;
        float dLon = (p2.lon() - p1.lon()) * RAD_PER_DEG * LOC_SCALE;
        float x    = dLon * cos( p1.lat()  * RAD_PER_DEG * LOC_SCALE + dLat/2 );
        return sqrt( x*x + dLat*dLat );
      }

    static float EquirectDistanceKm( const Location_t & p1, const Location_t p2 )
      {
        return EquirectDistanceRadians( p1, p2 ) * EARTH_RADIUS_KM;
      }

//private:
    friend class NMEAGPS; // This does not work?!?

    int32_t       _lat;  // degrees * 1e7, negative is South
    int32_t       _lon;  // degrees * 1e7, negative is West

} NEOGPS_PACKED;

} // NeoGPS

#endif