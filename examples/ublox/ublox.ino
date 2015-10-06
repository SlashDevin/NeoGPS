#include <Arduino.h>
#include "ubxGPS.h"

//======================================================================
//  Program: ublox.ino
//
//  Prerequisites:
//     1) You have a ublox GPS device
//     2) PUBX.ino works with your device
//     3) You have installed the ubxGPS.* and ubxmsg.* files.
//     4) At least one UBX message has been enabled in ubxGPS.h.
//     5) Implicit Merging is disabled in NMEAGPS_cfg.h.
//
//  Description:  This program parses UBX binary protocal messages from
//     ublox devices.  It is an extension of NMEAfused.ino.
//
//  Serial is for trace output to the Serial Monitor window.
//
//======================================================================

#include "GPSport.h"
#include "Streamers.h"
Stream & trace = Serial;

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined(UBLOX_PARSE_STATUS) & !defined(UBLOX_PARSE_TIMEGPS) & \
    !defined(UBLOX_PARSE_TIMEUTC) & !defined(UBLOX_PARSE_POSLLH) & \
    !defined(UBLOX_PARSE_VELNED) & !defined(UBLOX_PARSE_SVINFO)

  #error No UBX binary messages enabled: no fix data available for fusing.

#endif

//-----------------------------------------------------------------
//  Derive a class to add the state machine for starting up:
//    1) The status must change to something other than NONE.
//    2) The GPS leap seconds must be received
//    3) The UTC time must be received
//    4) All configured messages are "requested"
//         (i.e., "enabled" in the ublox device)
//  Then, all configured messages are parsed and explicitly merged.

class MyGPS : public ubloxGPS
{
public:

    gps_fix merged;

    enum
      {
        GETTING_STATUS, 
        GETTING_LEAP_SECONDS, 
        GETTING_UTC, 
        RUNNING
      }
        state NEOGPS_BF(8);

    uint32_t last_rx;
    uint32_t last_sentence;

    // Prevent recursive sentence processing while waiting for ACKs
    bool ok_to_process;
    
    MyGPS( Stream *device ) : ubloxGPS( device )
    {
      state = GETTING_STATUS;
      last_rx = 0L;
      ok_to_process = false;
    }

    //--------------------------

    void begin()
    {
      last_rx = millis();
    }

    //--------------------------

    void run()
    {
      bool rx = false;

      while (Device()->available()) {
        rx = true;
        if (decode( Device()->read() ) == DECODE_COMPLETED) {
          if (ok_to_process)
            processSentence();
        }
      }

      uint32_t ms = millis();

      if (rx)
        last_rx = ms;

      else {
        if (ok_to_process && ((ms - last_rx) > 2000L)) {
          last_rx = ms;
          trace << F("RESTART!\n");
          if (state != GETTING_STATUS) {
            state = GETTING_STATUS;
            enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_STATUS );
          }
        }
      }
    } // run

    //--------------------------

    void get_status()
    {
      static bool acquiring = false;

      if (fix().status == gps_fix::STATUS_NONE) {
        if (!acquiring) {
          acquiring = true;
          trace << F("Acquiring...");
        } else
          trace << '.';

      } else {
        if (acquiring)
          trace << '\n';
        trace << F("Acquired status: ") << (uint8_t) fix().status << '\n';

        #if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE) & \
            defined(UBLOX_PARSE_TIMEGPS)

          if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEGPS ))
            trace << F("enable TIMEGPS failed!\n");

          state = GETTING_LEAP_SECONDS;
        #else
          start_running();
          state = RUNNING;
        #endif
      }
    } // get_status

    //--------------------------

    void get_leap_seconds()
    {
      #if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE) & \
          defined(UBLOX_PARSE_TIMEGPS)

        if (GPSTime::leap_seconds != 0) {
          trace << F("Acquired leap seconds: ") << GPSTime::leap_seconds << '\n';

          if (!disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEGPS ))
            trace << F("disable TIMEGPS failed!\n");

          #if defined(UBLOX_PARSE_TIMEUTC)
            if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC ))
              trace << F("enable TIMEUTC failed!\n");
            state = GETTING_UTC;
          #else
            start_running();
          #endif
        }
      #endif

    } // get_leap_seconds

    //--------------------------

    void get_utc()
    {
      #if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE) & \
          defined(UBLOX_PARSE_TIMEUTC)
        if (GPSTime::start_of_week() != 0) {
          trace << F("Acquired UTC: ") << fix().dateTime << '\n';
          trace << F("Acquired Start-of-Week: ") << GPSTime::start_of_week() << '\n';

          start_running();
        }
      #endif

    } // get_utc

    //--------------------------

    void start_running()
    {
      bool enabled_msg_with_time = false;

      #if (defined(GPS_FIX_LOCATION) | defined(GPS_FIX_ALTITUDE)) & \
          defined(UBLOX_PARSE_POSLLH)
        if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_POSLLH ))
          trace << F("enable POSLLH failed!\n");

        enabled_msg_with_time = true;
      #endif

      #if (defined(GPS_FIX_SPEED) | defined(GPS_FIX_HEADING)) & \
          defined(UBLOX_PARSE_VELNED)
        if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_VELNED ))
          trace << F("enable VELNED failed!\n");

        enabled_msg_with_time = true;
      #endif

      #if (defined(GPS_FIX_SATELLITES) | defined(NMEAGPS_PARSE_SATELLITES)) & \
          defined(UBLOX_PARSE_SVINFO)
        if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_SVINFO ))
          trace << PSTR("enable SVINFO failed!\n");
        
        enabled_msg_with_time = true;
      #endif

      #if defined(UBLOX_PARSE_TIMEUTC)

        #if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE)
          if (enabled_msg_with_time &&
              !disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC ))
            trace << F("disable TIMEUTC failed!\n");

        #elif defined(GPS_FIX_TIME) | defined(GPS_FIX_DATE)
          // If both aren't defined, we can't convert TOW to UTC,
          // so ask for the separate UTC message.
          if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC ))
            trace << F("enable TIMEUTC failed!\n");
        #endif

      #endif

      state = RUNNING;
      trace_header();
      
    } // start_running

    //--------------------------

    bool is_new_interval()
    {
      // See if we stepped into a different time interval,
      //   or if it has finally become valid after a cold start.

      bool new_interval;

      #if defined(GPS_FIX_TIME)
        new_interval = (fix().valid.time &&
                      (!merged.valid.time ||
                       (merged.dateTime.seconds != fix().dateTime.seconds) ||
                       (merged.dateTime.minutes != fix().dateTime.minutes) ||
                       (merged.dateTime.hours   != fix().dateTime.hours)));

      #elif defined(GPS_FIX_DATE) && defined(PULSE_PER_DAY)
        new_interval = (fix().valid.date &&
                      (!merged.valid.date ||
                       (merged.dateTime.date  != fix().dateTime.date) ||
                       (merged.dateTime.month != fix().dateTime.month) ||
                       (merged.dateTime.year  != fix().dateTime.year)));
      
      #else
        //  Time is not configured, so let's assume it's a new interval
        //    if we just received a particular sentence.
        //  Different GPS devices will send sentences in different orders.

        new_interval = (rx().msg_class == ublox::UBX_NAV) &&
                       (rx().msg_id    == ublox::UBX_NAV_STATUS);
      
      #endif
      
      return new_interval;

    } // is_new_interval

    //--------------------------

    bool processSentence()
    {
      bool old_otp = ok_to_process;
      ok_to_process = false;

      bool ok = false;

      if (!ok && (nmeaMessage >= (nmea_msg_t)ubloxGPS::PUBX_00))
        ok = true;

      if (!ok && (rx().msg_class != ublox::UBX_UNK))
        ok = true;

      if (ok) {

        switch (state) {
          case GETTING_STATUS      : get_status      (); break;

          case GETTING_LEAP_SECONDS: get_leap_seconds(); break;

          case GETTING_UTC         : get_utc         (); break;

          case RUNNING:
            if (is_new_interval()) {

              //  Since we're into the next time interval, we throw away
              //     all of the previous fix and start with what we
              //     just received.
              merged = fix();

            } else {
              // Accumulate all the reports in this time interval
              merged |= fix();
            }
            break;
        }
      }

      ok_to_process = old_otp;

      return ok;
      
    } // processSentence

    //--------------------------

    void traceIt()
    {
      if (state == RUNNING)
        trace_all( *this, merged );

    } // traceIt


} NEOGPS_PACKED;

// Construct the GPS object and hook it to the appropriate serial device
static MyGPS gps( &gps_port );

//----------------------------------------------------------------
//  Determine whether the GPS quiet time has started, using the
//    current time, the last time a character was received,
//    and the last time a GPS quiet time started.

static bool quietTimeStarted()
{
  // As was shown in NMEAfused.ino, this could be replaced by a 
  //   test for the last UBX binary message that is sent in each
  //   one-second interval: NAV_VELNED.
  // For now, the timing technique is used.

  uint32_t current_ms       = millis();
  uint32_t ms_since_last_rx = current_ms - gps.last_rx;

  if (ms_since_last_rx > 5) {

    // The GPS device has not sent any characters for at least 5ms.
    //   See if we've been getting chars sometime during the last second.
    //   If not, the GPS may not be working or connected properly.

    bool getting_chars = (ms_since_last_rx < 1000UL);

    static uint32_t last_quiet_time = 0UL;

    bool just_went_quiet = (((int32_t) (gps.last_rx - last_quiet_time)) > 0L);
    bool next_quiet_time = ((current_ms - last_quiet_time) >= 1000UL);

    if ((getting_chars && just_went_quiet)
          ||
        (!getting_chars && next_quiet_time)) {

      if (!getting_chars) {
        trace.println( F("Check GPS device and/or connections.  No data received.\n") );
      }

      last_quiet_time = current_ms;  // Remember for next loop

      return true;
    }
  }

  return false;

} // quietTimeStarted

//--------------------------

void setup()
{
  // Start the normal trace output
  Serial.begin(9600);  // change this to match 'trace'.  Can't do 'trace.begin'

  trace << F("ublox binary protocol example started.\n");
  trace << F("fix object size = ") << sizeof(gps.fix()) << '\n';
  trace << F("ubloxGPS object size = ") << sizeof(ubloxGPS) << '\n';
  trace << F("MyGPS object size = ") << sizeof(gps) << '\n';
  trace.println( F("Looking for GPS device on " USING_GPS_PORT) );
  trace.flush();

  // Start the UART for the GPS device
  Serial1.begin(9600);

  gps.begin();

  // Turn off the preconfigured NMEA standard messages
  for (uint8_t i=NMEAGPS::NMEA_FIRST_MSG; i<=NMEAGPS::NMEA_LAST_MSG; i++) {
    ublox::configNMEA( gps, (NMEAGPS::nmea_msg_t) i, 0 );
  }

  // Turn on the UBX status message
  gps.enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_STATUS );

  // Turn off things that may be left on by a previous build
  gps.disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEGPS );
  gps.disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC );
  gps.disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_VELNED );
  gps.disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_POSLLH );

  #if 0
    // Test a Neo M8 message -- should be rejected by Neo-6 and Neo7
    ublox::cfg_nmea_v1_t test;

    test.always_output_pos  = false; // invalid or failed
    test.output_invalid_pos = false;
    test.output_invalid_time= false;
    test.output_invalid_date= false;
    test.use_GPS_only       = false;
    test.output_heading     = false; // even if frozen
    test.__not_used__       = false;

    test.nmea_version = ublox::cfg_nmea_v1_t::NMEA_V_4_0;
    test.num_sats_per_talker_id = ublox::cfg_nmea_v1_t::SV_PER_TALKERID_UNLIMITED;

    test.compatibility_mode = false;
    test.considering_mode   = true;
    test.max_line_length_82 = false;
    test.__not_used_1__     = 0;

    test.filter_gps    = false;
    test.filter_sbas   = false;
    test.__not_used_2__= 0;
    test.filter_qzss   = false;
    test.filter_glonass= false;
    test.filter_beidou = false;
    test.__not_used_3__= 0;

    test.proprietary_sat_numbering = false;
    test.main_talker_id = ublox::cfg_nmea_v1_t::MAIN_TALKER_ID_GP;
    test.gsv_uses_main_talker_id = true;
    test.beidou_talker_id[0] = 'G';
    test.beidou_talker_id[1] = 'P';

    trace << F("CFG_NMEA result = ") << gps.send( test );
  #endif

  gps.ok_to_process = true;
}

//--------------------------

void loop()
{
  gps.run();

  if (quietTimeStarted())
    gps.traceIt();
}
