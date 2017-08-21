#include "GpsPort.h"
#include <unistd.h>
#include <string.h>

GpsPort::GpsPort(const char* usbDev) {
    (void) usbDev;
    _device = 0;
}

bool GpsPort::available() {
    if (_device == 0) {
      return true;
    }
    _device = 1;
    return false;
}

static const char * NMEA_STRING = 
    "$GPRMC,162254.00,A,3723.02837,N,12159.39853,W,0.820,188.36,110706,,,A*74\r\n"
    "$GPVTG,188.36,T,,M,0.820,N,1.519,K,A*3F\r\n"
    "$GPGGA,162254.00,3723.02837,N,12159.39853,W,1,03,2.36,525.6,M,-25.6,M,,*65\r\n"
    "$GPGSA,A,2,25,01,22,,,,,,,,,,2.56,2.36,1.00*02\r\n"
    "$GPGSV,4,1,14,25,15,175,30,14,80,041,,19,38,259,14,01,52,223,18*76\r\n"
    "$GPGSV,4,2,14,18,16,079,,11,19,312,,14,80,041,,21,04,135,25*7D\r\n"
    "$GPGSV,4,3,14,15,27,134,18,03,25,222,,22,51,057,16,09,07,036,*79\r\n"
    "$GPGSV,4,4,14,07,01,181,,15,25,135,*76\r\n"
    "$GPGLL,3723.02837,N,12159.39853,W,162254.00,A,A*7C\r\n"
    "$GPZDA,162254.00,11,07,2006,00,00*63\r\n";
    
char GpsPort::read() {
    static int runner = 0;
    static int len = strlen(NMEA_STRING);
    char c = NMEA_STRING[runner];
    if (c == '$') {
      sleep(1);
    }
    runner = (runner + 1) % len;
    
    char nextC = NMEA_STRING[runner];
    if (nextC == '$') {
      // Signal available to say false;
      _device = 1;
    }
    
    return c;
}

void GpsPort::print(char c) {
    (void) c;
    return;
}
