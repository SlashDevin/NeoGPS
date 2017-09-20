#pragma once

#include <termios.h>

class GpsPort {
private:
  int _device;
    
public:
  GpsPort(const char* usbDev = "/dev/ttyUSB0", const char* speed = "9600");
  bool available();
  char read();
  void print(char);
    
private:
  int set_interface_attribs(int fd, int speed);
};

