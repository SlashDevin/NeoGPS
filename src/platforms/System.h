#pragma once

#ifndef NEO_GPS_SYSTEM
  #define NEO_GPS_SYSTEM NeoGPS::System
  #ifdef __GNUC__
    #define NEO_GPS_SYSTEM_DEFAULT_IMPL_WARN \
    __attribute__((deprecated("You are using a function which uses NEO_GPS_SYSTEM with the default implementation, which does nothing!")))
  #endif

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

#ifndef NEO_GPS_SYSTEM_DEFAULT_IMPL_WARN
  #define NEO_GPS_SYSTEM_DEFAULT_IMPL_WARN
#endif
