#pragma once

#include <stdint.h>

#ifndef NEO_GPS_PRINT
  #define NEO_GPS_PRINT NeoGPS::Print
  #ifdef __GNUC__
    #define NEO_GPS_PRINT_DEFAULT_IMPL_WARN \
    __attribute__((deprecated("You are using a function which uses NEO_GPS_PRINT with the default implementation, which does nothing!")))
  #endif

/**
 * This file implements a non working dummy implemenation for Print
 **/
namespace NeoGPS {
  
  class Print {
  };
  
  Print & operator << ( Print & print, char ) { return print; }
  Print & operator << ( Print & print, uint8_t ) { return print; }
  Print & operator << ( Print & print, uint16_t ) { return print; }
  Print & operator << ( Print & print, uint32_t ) { return print; }
  Print & operator << ( Print & print, int32_t ) { return print; }
  Print & operator << ( Print & print, const char * ) { return print; }
  
  #ifdef USE_FLOAT
    void printFloat( Print &, float f, uint8_t decPlaces ) {}
  #endif
}

#endif

#ifndef NEO_GPS_PRINT_DEFAULT_IMPL_WARN
  #define NEO_GPS_PRINT_DEFAULT_IMPL_WARN
#endif
