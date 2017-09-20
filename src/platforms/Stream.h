#pragma once

#include <stdint.h>
// If there is not already an implementation for your platform,
// implement these functions.
class Stream {
public:
  bool available();
  uint8_t read();
  void print(uint8_t);
};
