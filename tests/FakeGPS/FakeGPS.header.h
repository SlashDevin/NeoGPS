#pragma once

#include <ctime>

class FakeGPS {
private:
  const char* _fakeContent;
  bool _repeat;
  struct timeval _next_char_tv;

public:
  FakeGPS(const char* fakeOutput, bool repeat);
  bool available();
  char read();
  void print(char);
};

