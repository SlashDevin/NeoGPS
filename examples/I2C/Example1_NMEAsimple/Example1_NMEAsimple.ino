/*
  Program: Example1_NMEAsimple.ino

  Description:  

  Prerequisites:
     1) NEO-M8N with SDA/SCL connected. Use pullups as necessary.
     2) LAST_SENTENCE_IN_INTERVAL has been set to GLL in NMEAGPS_cfg.h
*/

#include <NMEAGPS.h>

NMEAGPS gps; // This parses the GPS characters
gps_fix fix; // This holds on to the latest values

void setup()
{
  Serial.begin(9600);
  while (!Serial) ; //Wait for user to open terminal
  
  Serial.println("Example using Wire/I2C");

  Wire.begin();
}

void loop()
{
  while (gps.available(Wire)) {
    fix = gps.read();

    Serial.print("Location: ");
    if (fix.valid.location) {
      Serial.print( fix.latitude(), 6 );
      Serial.print( ',' );
      Serial.print( fix.longitude(), 6 );
    }

    Serial.print(", Altitude: ");
    if (fix.valid.altitude)
      Serial.print( fix.altitude() );

    Serial.println();
  }
}
