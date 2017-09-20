#ifndef COSACOMPAT_H
#define COSACOMPAT_H

#ifdef __AVR__

  #include <avr/pgmspace.h>

#else

  #define PGM_P const char *

#endif

typedef PGM_P str_P;

#define __PROGMEM PROGMEM

#ifndef ARDUINO
  #include <stdint.h>
  #define PROGMEM

  #define pgm_read_byte(x) (*(x))
  #define __FlashStringHelper char
  #define F(x) (x)

  #include <math.h>
  constexpr double pi() { return std::atan(1)*4; }
  
  #define PI pi()
  #define TWO_PI pi() * 2
#endif

#endif
