#include <platforms/Print.h>

#include <iostream>

template <typename T>
static void _print(T x) {
  std::cout << x;
}

void Print::print(char c) const { _print(c); }
void Print::print(uint8_t n) const { _print(n); }
void Print::print(uint16_t n) const { _print(n); }
void Print::print(uint32_t n) const { _print(n); }
void Print::print(int n) const { _print(n); }
void Print::print(const char *s) const { _print(s); }
void Print::write(char c) const { _print(c); }
