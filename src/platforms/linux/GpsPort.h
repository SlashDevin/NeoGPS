#pragma once
#include "serial.h"

class GpsPort {
private:
    serial_dev_t _device;
    
public:
    GpsPort(const char*);
    bool available();
    char read();
    void print(char);
};

