#pragma once

/**
 * This file contains platform related definitions and imports.
 **/
#ifdef ARDUINO
  #include <Arduino.h>
  
  #include <Stream.h>
  #include <Print.h>
  
  #ifdef __AVR__
    #include <avr/interrupt.h>
  #endif
#else
  #include "platforms/Stream.h"
  #include "platforms/Print.h"
  #include "platforms/System.h"
  
  #include <stdint.h>
  #define PROGMEM

  #define pgm_read_byte(x) (*(x))
  #define __FlashStringHelper char
  #define F(x) (x)
  #define str_P char *

  #include <math.h>
  constexpr double pi() { return std::atan(1)*4; }
  
  #define PI pi()
  #define TWO_PI pi() * 2.0
  
#endif
