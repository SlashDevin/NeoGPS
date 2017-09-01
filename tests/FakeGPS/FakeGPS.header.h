#pragma once

#include <ctime>

class FakeGPS {
private:
  const char* _fakeContent;
  bool _repeat;
  std::time_t _next_char_ts;

public:
  FakeGPS(const char* fakeOutput, bool repeat);
  bool available();
  char read();
  void print(char);
};

