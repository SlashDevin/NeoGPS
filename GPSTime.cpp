#include "GPSTime.h"

uint8_t GPSTime::leap_seconds = 0;
clock_t GPSTime::s_start_of_week = 0;
