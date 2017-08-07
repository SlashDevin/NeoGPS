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
  
  const double PI = 3.14159265358979323846;
  #define TWO_PI PI * 2.0
  
#endif
