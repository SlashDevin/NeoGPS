#pragma once

#ifndef NEO_GPS_SYSTEM
  #define NEO_GPS_SYSTEM NeoGPS::System
  #ifdef __GNUC__
    #define NEO_GPS_SYSTEM_DEFAULT_IMPL_WARN \
    __attribute__((deprecated("You are using a function which uses NEO_GPS_STREAM without defining the type.  Compilation will succeed, but the default implementation does not lock / unlock (enable / disable irqs).  To fix this #define NEO_GPS_SYSTEM your_system_wrapper_class_t  before including platform.h")))
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
