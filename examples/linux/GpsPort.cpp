#include "GpsPort.h"

GpsPort::GpsPort(const char* usbDev) {
    _device = ::init(usbDev);
}

bool GpsPort::available() {
    return true;
    //bool available = ::data_available(_device);
    //std::cout << "Availabe: " << available << std::endl;
    //return available;
}

char GpsPort::read() {
    return ::read(_device);
}

void GpsPort::print(char c) {
    char s[2];
    s[0] = c;
    s[1] = '\0';
    ::write(_device, s);
}
