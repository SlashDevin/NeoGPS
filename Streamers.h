#ifndef STREAMERS_H
#define STREAMERS_H

#include <Arduino.h>

#include "Time.h"

extern Stream & trace;

extern Stream & operator <<( Stream & outs, const bool b );
extern Stream & operator <<( Stream & outs, const char c );
extern Stream & operator <<( Stream & outs, const uint16_t v );
extern Stream & operator <<( Stream & outs, const uint32_t v );
extern Stream & operator <<( Stream & outs, const int32_t v );
extern Stream & operator <<( Stream & outs, const uint8_t v );
extern Stream & operator <<( Stream & outs, const __FlashStringHelper *s );

extern Stream & operator <<( Stream & outs, const tmElements_t & t );

#endif