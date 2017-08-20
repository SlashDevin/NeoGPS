#pragma once

#include "CosaCompat.h"

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
  #include "platforms/System.h"
  
  #include <stdint.h>
  //#define __PROGMEM
  #define PROGMEM

  #define pgm_read_byte(x) (*(x))
  #define __FlashStringHelper char
  #define F(x) (x)

  #include <math.h>
  
  #define PI 3.14159265358979323846
  #define TWO_PI PI * 2.0
  
#endif

#ifndef CR
  #define CR ((char)13)
#endif
#ifndef LF
  #define LF ((char)10)
#endif
