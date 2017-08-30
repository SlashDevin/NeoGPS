#pragma once

#include <stdint.h>

#ifdef ARDUINO
  #include <platforms/arduino/platform.h>
#elif defined LINUX
  #include <platforms/linux/platform.h>
#else
  #include <platforms/Stream.h>
  #include <platforms/Print.h>
  #include <platforms/System.h>
#endif

#ifdef __AVR__
  #include <avr/pgmspace.h>
  #include <avr/interrupt.h>
#else
  #define PGM_P const char *
  #define pgm_read_byte(x) (*(x))
  #define __FlashStringHelper char
  #define F(x) (x)
#endif

typedef PGM_P str_P;

#include <math.h>
  
#ifndef PI
  #define PI 3.14159265358979323846
#endif
#ifndef TWO_PI
  #define TWO_PI PI * 2.0
#endif

#ifndef PROGMEM
  #define PROGMEM
#endif

#define __PROGMEM PROGMEM
