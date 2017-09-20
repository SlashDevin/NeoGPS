#pragma once

#include <stdint.h>
// If there is not already an implementation for your platform,
// implement these functions.
class Stream {
public:
  bool available();
  char read();
  void print(char);
};
