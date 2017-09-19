#pragma once

#include "FakeGPS.header.h"

#include <string.h>
#include <sys/time.h>


FakeGPS::FakeGPS(const char* fakeContent, bool repeat)
  : _fakeContent(fakeContent), _repeat(repeat) {
    _next_char_tv.tv_sec = 1; // 0 is our stop condition
    _next_char_tv.tv_usec = 0;
  }

bool FakeGPS::available() {
  struct timeval now;
  gettimeofday(&now, NULL);
  return _next_char_tv.tv_sec > 0 && 
    _next_char_tv.tv_sec <= now.tv_sec &&
    _next_char_tv.tv_usec <= now.tv_usec;
}

    
char FakeGPS::read() {
    static int runner = 0;
    static int len = strlen(_fakeContent);
    
    char c = _fakeContent[runner];
    runner = (runner + 1) % len;
    
    if (c == '\n') {
      if (!_repeat && runner == 0) {
        _next_char_tv.tv_sec = 0;
      } else {
        _next_char_tv.tv_usec += 100000;
        if (_next_char_tv.tv_usec > 1000000) {
        _next_char_tv.tv_usec -= 1000000;
          ++_next_char_tv.tv_sec;
        }
      }
    }
    
    return c;
}

void FakeGPS::print(char c) {
    (void) c;
    return;
}
