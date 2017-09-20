#pragma once

#include <Arduino.h>
  
#include <Stream.h>
#include <Print.h>

#define NEO_GPS_SYSTEM System

#define NEO_GPS_STREAM Stream

#define NEO_GPS_PRINT Print

class System {
public:
  static void lock() { noInterrupts(); }
  static void unlock() { interrutps(); }
};
  
template <typename T>
void printFloat( Print &outs, T f, uint8_t decimalPlaces) { outs.print(f, decimalPlaces); }
  
Print& operator <<( Print &outs, const bool b ) { outs.print( b ? 't' : 'f' ); return outs; }

Print& operator <<( Print &outs, const char c ) { outs.print(c); return outs; }

Print& operator <<( Print &outs, const uint16_t v ) { outs.print(v); return outs; }

Print& operator <<( Print &outs, const uint32_t v ) { outs.print(v); return outs; }

Print& operator <<( Print &outs, const int32_t v ) { outs.print(v); return outs; }

Print& operator <<( Print &outs, const uint8_t v ) { outs.print(v); return outs; }

Print& operator <<( Print &outs, const __FlashStringHelper *s ) { outs.print(s); return outs; }

