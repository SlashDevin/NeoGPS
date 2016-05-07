#include <Arduino.h>
#include "NMEAGPS.h"

//======================================================================
//  Program: NMEAloc.ino
//
//  Description:  This program only parses an RMC sentence for the lat/lon.
//
//  Prerequisites:
//     1) NMEA.ino works with your device (correct TX/RX pins and baud rate)
//     2) The RMC sentence has been enabled.
//     3) Your device sends an RMC sentence (e.g., $GPRMC).
//
//  Serial is for trace output to the Serial Monitor window.
//
//======================================================================

#if defined( UBRR1H )
  // Default is to use Serial1 when available.  You could also
  // use NeoHWSerial, especially if you want to handle GPS characters
  // in an Interrupt Service Routine.
  //#include <NeoHWSerial.h>
#else  
  // Only one serial port is available, uncomment one of the following:
  //#include <NeoICSerial.h>
  #include <NeoSWSerial.h>
  //#include <SoftwareSerial.h> /* NOT RECOMMENDED */
#endif
#include "GPSport.h"

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_RMC )

  #error You must uncomment NMEAGPS_PARSE_RMC in NMEAGPS_cfg.h!

#endif

#if !defined( GPS_FIX_LOCATION )

  #error You must uncomment GPS_FIX_LOCATION in GPSfix_cfg.h!

#endif

//------------------------------------------------------------

static NMEAGPS  gps; // This parses the GPS characters
static gps_fix  fix; // This holds on to the parsed fields, like lat/lon

//----------------------------------------------------------------

static void printL( Stream & stream, int32_t degE7 );

static void printL( Stream & stream, int32_t degE7 )
{
  // Extract and print negative sign
  if (degE7 < 0) {
    degE7 = -degE7;
    stream.print( '-' );
  }

  // Whole degrees
  int32_t deg = degE7 / 10000000L;
  stream.print( deg );
  stream.print( '.' );

  // Get fractional degrees
  degE7 -= deg*10000000L;

  // Print leading zeroes, if needed
  int32_t factor = 1000000L;
  while ((degE7 < factor) && (factor > 1L)){
    stream.print( '0' );
    factor /= 10L;
  }
  
  // Print fractional degrees
  stream.print( degE7 );
}

static void doSomeWork();
static void doSomeWork()
{
  //  This is the best place to do your time-consuming work, right after
  //     the RMC sentence was received.  If you do anything in "loop()",
  //     you could cause GPS characters to be lost, and you will not
  //     get a good lat/lon.
  //  For this example, we just print the lat/lon.  If you print too much,
  //     this routine will not get back to "loop()" in time to process
  //     the next set of GPS data.

  if (fix.valid.location) {

    // Serial.print( fix.latitude(), 6 ); // floating-point display
    // Serial.print( fix.latitudeL() ); // integer display
    printL( Serial, fix.latitudeL() );
    Serial.print( ',' );
    // Serial.print( fix.longitude(), 6 ); // floating-point display
    // Serial.print( fix.longitudeL() );  // integer display
    printL( Serial, fix.longitudeL() );

  } else {
    // No valid location data yet!
    Serial.print( '?' );
  }

  Serial.println();

} // doSomeWork

//------------------------------------

static void GPSloop();
static void GPSloop()
{  
  while (gps_port.available()) {

    if (gps.decode( gps_port.read() ) == NMEAGPS::DECODE_COMPLETED) {

      if (gps.nmeaMessage == NMEAGPS::NMEA_RMC) {
        fix = gps.fix(); // fetch the latest fix data...
        doSomeWork();    // ... and do some serious work with it.
      }
    }
  }
} // GPSloop
  
//--------------------------

void setup()
{
  Serial.begin(9600);

  Serial.print( F("NMEAloc.INO: started\n") );
  Serial.println( F("Looking for GPS device on " USING_GPS_PORT) );
  Serial.flush();
  
  // Start the UART for the GPS device
  gps_port.begin(9600);
}

//--------------------------

void loop()
{
  GPSloop();
  
  // If the GPS has been sending data, then the "fix" structure may have
  //   valid data.  Remember, you must check the valid flags before you
  //   use any of the data inside "fix".  See "doSomeWork" for an example
  //   of checking whether any lat/lon data has been received yet.
}
