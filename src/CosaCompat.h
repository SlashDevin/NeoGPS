#ifndef COSACOMPAT_H
#define COSACOMPAT_H

#ifdef __AVR__

  #include <avr/pgmspace.h>

#else

  #define PGM_P const char *

#endif

typedef PGM_P str_P;
#define __PROGMEM PROGMEM

#endif