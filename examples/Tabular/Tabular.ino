#include <NeoSWSerial.h>
#include <NMEAGPS.h>

NMEAGPS     gps;
//NeoSWSerial gpsPort(4, 3);
#define gpsPort Serial1

static const NeoGPS::Location_t London( 51.508131, -0.128002 );

#ifndef GPS_FIX_HDOP
  #error You must uncomment GPS_FIX_HDOP in GPSfix_cfg.h!
#endif

void setup()
{
  Serial.begin(9600);
  
  Serial.println
    (
      F( "Testing NeoGPS library\n\n"
         "Sats HDOP Latitude  Longitude  Date       Time     Alt    Speed  Heading    -- To London --    Chars Sentences Errors\n"
         "          (deg)     (deg)                          (m)                      Dist    Dir\n" )
    );

  repeat( '-', 133 );

  gpsPort.begin(9600);
}

void loop()
{
  if (gps.available( gpsPort )) {
    gps_fix fix = gps.read();

    float bearingToLondon = fix.location.BearingToDegrees( London );
    bool  validDT         = fix.valid.date & fix.valid.time;

    print(             fix.satellites       , fix.valid.satellites, 3             );
    print(             fix.hdop/1000.0      , fix.valid.hdop      , 6, 2          );
    print(             fix.latitude ()      , fix.valid.location  , 10, 6         );
    print(             fix.longitude()      , fix.valid.location  , 11, 6         );
    print(             fix.dateTime         , validDT             , 20            );
    print(             fix.altitude ()      , fix.valid.altitude  , 7, 2          );
    print(             fix.speed_kph()      , fix.valid.speed     , 7, 2          );
    print(             fix.heading  ()      , fix.valid.heading   , 7, 2          );
    print( compassDir( fix.heading  () )    , fix.valid.heading   , 4             );
    print( fix.location.DistanceKm( London ), fix.valid.location  , 5             );
    print(             bearingToLondon      , fix.valid.location  , 7, 2          );
    print( compassDir( bearingToLondon )    , fix.valid.location  , 4             );

    print( gps.statistics.chars , true, 10 );
    print( gps.statistics.ok    , true,  6 );
    print( gps.statistics.errors, true,  6 );

    Serial.println();
  }
}

//  Print utilities

static void repeat( char c, int8_t len )
{
  for (int8_t i=0; i<len; i++)
    Serial.write( c );
}

static void printInvalid( int8_t len )
{
  Serial.write( ' ' );
  repeat( '*', abs(len)-1 );
}

static void print( float val, bool valid, int8_t len, int8_t prec )
{
  if (!valid) {
    printInvalid( len );
  } else {
    char s[16];
    dtostrf( val, len, prec, s );
    Serial.print( s );
  }
}

static void print( int32_t val, bool valid, int8_t len )
{
  if (!valid) {
    printInvalid( len );
  } else {
    char s[16];
    ltoa( val, s, 10 );
    repeat( ' ', len - strlen(s) );
    Serial.print( s );
  }
}

static void print( const __FlashStringHelper *str, bool valid, int8_t len )
{
  if (!valid) {
    printInvalid( len );
  } else {
    int slen = strlen_P( (const char *) str );
    repeat( ' ', len-slen );
    Serial.print( str );
  }
}

static void print( const NeoGPS::time_t & dt, bool valid, int8_t len )
{
  if (!valid) {
    printInvalid( len );
  } else {
    Serial.write( ' ' );
    Serial << dt;
  }
}

//------------------------------------------------------------
//  This snippet is from NMEAaverage.  It keeps all the
//    compass direction strings in FLASH memory, saving RAM.

const char nCD  [] PROGMEM = "N";
const char nneCD[] PROGMEM = "NNE";
const char neCD [] PROGMEM = "NE";
const char eneCD[] PROGMEM = "ENE";
const char eCD  [] PROGMEM = "E";
const char eseCD[] PROGMEM = "ESE";
const char seCD [] PROGMEM = "SE";
const char sseCD[] PROGMEM = "SSE";
const char sCD  [] PROGMEM = "S";
const char sswCD[] PROGMEM = "SSW";
const char swCD [] PROGMEM = "SW";
const char wswCD[] PROGMEM = "WSW";
const char wCD  [] PROGMEM = "W";
const char wnwCD[] PROGMEM = "WNW";
const char nwCD [] PROGMEM = "NW";
const char nnwCD[] PROGMEM = "NNW";

const char * const dirStrings[] PROGMEM =
  { nCD, nneCD, neCD, eneCD, eCD, eseCD, seCD, sseCD, 
    sCD, sswCD, swCD, wswCD, wCD, wnwCD, nwCD, nnwCD };

const __FlashStringHelper *compassDir( uint16_t bearing ) // degrees CW from N
{
  const int16_t directions    = sizeof(dirStrings)/sizeof(dirStrings[0]);
  const int16_t degreesPerDir = 360 / directions;
        int8_t  dir           = (bearing + degreesPerDir/2) / degreesPerDir;

  while (dir < 0)
    dir += directions;
  while (dir >= directions)
    dir -= directions;

  return (const __FlashStringHelper *) pgm_read_ptr( &dirStrings[ dir ] );

} // compassDir
