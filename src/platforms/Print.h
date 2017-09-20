#pragma once

#include <stdint.h>
class Print {
public:
  void print(char) const;
  void print(uint8_t) const;
  void print(uint16_t) const;
  void print(uint32_t) const;
  void print(const char *) const;
  void write(char) const;
};
