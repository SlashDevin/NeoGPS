/*
  Serial should be connected to the GPS device.
  This will toggle the LED once per second, when a GPRMC message is received.
*/

#include <Arduino.h>

#include "NMEAGPS.h"

static NMEAGPS gps;

static const int led = 13;

void setup()
{
  // Start the UART for the GPS device
  Serial.begin(9600);
  pinMode(led, OUTPUT);
}

void loop()
{
  while (Serial.available()) {
    if (gps.decode( Serial.read() ) == NMEAGPS::DECODE_COMPLETED) {
      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC)
        digitalWrite(led, !digitalRead(led));
    }
  }
}
