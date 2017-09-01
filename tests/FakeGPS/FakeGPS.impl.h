#pragma once

#include "FakeGPS.header.h"

#include <unistd.h>
#include <string.h>


FakeGPS::FakeGPS(const char* fakeContent, bool repeat)
  : _fakeContent(fakeContent), _repeat(repeat), _next_char_ts(0) {}

bool FakeGPS::available() {
  return _next_char_ts > 0 && _next_char_ts <= std::time( 0 );
}

    
char FakeGPS::read() {
    static int runner = 0;
    static int len = strlen(_fakeContent);
    
    char c = _fakeContent[runner];
    runner = (runner + 1) % len;
    
    if (c == '\n') {
      if (!_repeat && runner == 0) {
        _next_char_ts = -1;
      } else {
        _next_char_ts = std::time( 0 ) + 1;
      }
    }
    
    return c;
}

void FakeGPS::print(char c) {
    (void) c;
    return;
}
