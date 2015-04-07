#include "GPSTime.h"

uint8_t         GPSTime::leap_seconds    = 0;
NeoGPS::clock_t GPSTime::s_start_of_week = 0;
