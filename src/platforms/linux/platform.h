#pragma once

#include <iostream>

#define NEO_GPS_SYSTEM System

#define NEO_GPS_STREAM GpsStream

#define NEO_GPS_PRINT std::iostream

class System {
public:
  static void lock() {}
  static void unlock() {}
};


