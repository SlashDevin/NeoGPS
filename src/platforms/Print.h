#pragma once

#include <stdint.h>
class Print {
public:
  void print(uint8_t);
  void print(uint16_t);
  void print(uint32_t);
  void write(char);
};
