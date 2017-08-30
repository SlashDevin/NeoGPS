#pragma once

#ifndef NEO_GPS_STREAM
  #define NEO_GPS_STREAM NeoGPS::Stream
#endif

/**
 * This file implements a non working dummy implementation for Stream.
 **/
namespace NeoGPS {
  
  class Stream {
  public:
    bool available() { return false; }
    char read() { return '\0'; }
    void print(char) {}
  };
}
