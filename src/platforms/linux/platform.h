#pragma once

#ifndef NEO_GPS_PRINT
  #include <iostream>
  #include <iomanip>
  
  #define NEO_GPS_PRINT decltype(std::cout)
  
  #ifdef USE_FLOAT
    void printFloat( NEO_GPS_PRINT & io, float f, uint8_t decPlaces ) {
      std::streamsize ss = std::cout.precision();
      io << std::setprecision(decPlaces) << std::fixed << f << std::setprecision(ss);
    }
  #endif
#endif

#ifndef NEO_GPS_STREAM
  #include "GpsPort.header.h"
  #define NEO_GPS_STREAM GpsPort
#endif

#ifndef NEO_GPS_SYSTEM
  #define NEO_GPS_SYSTEM System

  class System {
  public:
    static void lock() {}
    static void unlock() {}
  };
#endif


