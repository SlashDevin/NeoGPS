#pragma once

typedef int serial_dev_t;

// Open portname (if nullptr /dev/ttyUSB0).
serial_dev_t init(const char *portname);

// Write to previously initialized port.
void write(serial_dev_t, const char* out);

// Reads one character from previously initialized port.
char read(serial_dev_t);

// Returns true if read would not block on initialized port.
bool data_available(serial_dev_t);
