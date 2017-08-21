#include "GpsPort.h"

GpsPort::GpsPort(const char* usbDev) {
    _device = ::init(usbDev);
}

bool GpsPort::available() {
    return ::data_available(_device);
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
