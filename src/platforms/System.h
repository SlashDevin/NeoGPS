#pragma once

#ifndef NEO_GPS_SYSTEM
  #define NEO_GPS_SYSTEM NeoGPS::System

/**
 * This file implements a non working dummy implementation for interrupts and noInterrupts.
 **/
namespace NeoGPS {
  
  class System {
  public:
    static void lock() {}
    static void unlock() {}
  };
}
#endif
