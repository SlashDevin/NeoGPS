#ifndef NMEAGPS_CFG_H
#define NMEAGPS_CFG_H

//------------------------------------------------------
// Enable/disable the parsing of specific sentences.
//
// Configuring out a sentence prevents its fields from being parsed.
// However, the sentence type will still be recognized by /decode/ and 
// stored in member /nmeaMessage/.  No valid flags would be available.
//
// Only RMC and ZDA contain date information.  Other
// sentences contain time information.  Both date and time are 
// required if you will be doing time_t-to-clock_t operations.

//#define NMEAGPS_PARSE_GGA
//#define NMEAGPS_PARSE_GLL
//#define NMEAGPS_PARSE_GSA
//#define NMEAGPS_PARSE_GSV
//#define NMEAGPS_PARSE_GST
//#define NMEAGPS_PARSE_RMC
//#define NMEAGPS_PARSE_VTG
//#define NMEAGPS_PARSE_ZDA

//------------------------------------------------------
// Enable/disable the talker ID and manufacturer ID processing.
//
// First, some background information.  There are two kinds of NMEA sentences:
//
// 1. Standard NMEA sentences begin with "$ttccc", where
//      "tt" is the talker ID, and
//      "ccc" is the variable-length sentence type (i.e., command).
//
//    For example, "$GPGLL,..." is a GLL sentence (Geographic Lat/Long) 
//    transmitted by talker "GP".  This is the most common talker ID.  Some
//    devices may report "$GNGLL,..." when a mix of GPS and non-GPS
//    satellites have been used to determine the GLL data.
//
// 2. Proprietary NMEA sentences (i.e., those unique to a particular
//    manufacturer) begin with "$Pmmmccc", where
//      "P" is the NMEA-defined prefix indicator for proprietary messages,
//      "mmm" is the 3-character manufacturer ID, and
//      "ccc" is the variable-length sentence type (it can be empty).
//
// No validation of manufacturer ID and talker ID is performed in this
// base class.  For example, although "GP" is a common talker ID, it is not
// guaranteed to be transmitted by your particular device, and it IS NOT
// REQUIRED.  If you need validation of these IDs, or you need to use the
// extra information provided by some devices, you have two independent
// options:
//
// 1. Enable SAVING the ID: When /decode/ returns DECODE_COMPLETED, the
// /talker_id/ and/or /mfr_id/ members will contain ID bytes.  The entire
// sentence will be parsed, perhaps modifying members of /fix/.  You should
// enable one or both IDs if you want the information in all sentences *and*
// you also want to know the ID bytes.  This add two bytes of RAM for the
// talker ID, and 3 bytes of RAM for the manufacturer ID.
//
// 2. Enable PARSING the ID:  The virtual /parse_talker_id/ and
// /parse_mfr_id/ will receive each ID character as it is received.  If it
// is not a valid ID, return /false/ to abort processing the rest of the
// sentence.  No CPU time will be wasted on the invalid sentence, and no
// /fix/ members will be modified.  You should enable this if you want to
// ignore some IDs.  You must override /parse_talker_id/ and/or
// /parse_mfr_id/ in a derived class.
//

//#define NMEAGPS_SAVE_TALKER_ID
//#define NMEAGPS_PARSE_TALKER_ID

//#define NMEAGPS_SAVE_MFR_ID
#define NMEAGPS_PARSE_MFR_ID

//------------------------------------------------------
// Enable/disable tracking the current satellite array and,
// optionally, all the info for each satellite.
//

//#define NMEAGPS_PARSE_SATELLITES
//#define NMEAGPS_PARSE_SATELLITE_INFO
//#define NMEAGPS_MAX_SATELLITES (40)

#ifdef NMEAGPS_PARSE_SATELLITES
#ifndef GPS_FIX_SATELLITES
#error GPS_FIX_SATELLITES must be defined in GPSfix.h!
#endif
#endif

#if defined(NMEAGPS_PARSE_SATELLITE_INFO) & \
    !defined(NMEAGPS_PARSE_SATELLITES)
#error NMEAGPS_PARSE_SATELLITES must be defined!
#endif

//------------------------------------------------------
// Enable/disable accumulating fix data across sentences.
//
// If not defined, the fix will contain data from only the last decoded sentence.
//
// If defined, the fix will contain data from all received sentences.  Each
// fix member will contain the last value received from any sentence that
// contains that information.  This means that fix members may contain
// information from different time intervals (i.e., they are not coherent).
//
// ALSO NOTE:  If a received sentence is rejected for any reason (e.g., CRC
//   error), all the values are suspect.  The fix will be cleared; no members
//   will be valid until new sentences are received and accepted.
//
//   This is an application tradeoff between keeping a merged copy of received
//   fix data (more RAM) vs. accommodating "gaps" in fix data (more code).
//
// SEE ALSO: NMEAfused.ino and NMEAcoherent.ino

//#define NMEAGPS_ACCUMULATE_FIX

#ifdef NMEAGPS_ACCUMULATE_FIX

  // When accumulating, nothing is done to the fix at the 
  // beginning of every sentence
  #define NMEAGPS_INIT_FIX(m)

  // ...but we invalidate one part when it starts to get parsed.  It *may* get
  // validated when the parsing is finished.
  #define NMEAGPS_INVALIDATE(m) m_fix.valid.m = false

#else

  // When NOT accumulating, invalidate the entire fix at the 
  // beginning of every sentence
  #define NMEAGPS_INIT_FIX(m) m.valid.init()

  // ...so the individual parts do not need to be invalidated as they are parsed
  #define NMEAGPS_INVALIDATE(m)

#endif


//------------------------------------------------------
// Enable/disable gathering interface statistics:
// CRC errors and number of sentences received

#define NMEAGPS_STATS

//------------------------------------------------------
// Configuration item for allowing derived types of NMEAGPS.
// If you derive classes from NMEAGPS, you *must* define NMEAGPS_DERIVED_TYPES.
// If not defined, virtuals are not used, with a slight size (2 bytes) and 
// execution time savings.

#define NMEAGPS_DERIVED_TYPES

#ifdef NMEAGPS_DERIVED_TYPES
  #define NMEAGPS_VIRTUAL virtual
#else
  #define NMEAGPS_VIRTUAL
#endif

//-----------------------------------
// See if DERIVED_TYPES is required
#if (defined(NMEAGPS_PARSE_TALKER_ID) | defined(NMEAGPS_PARSE_MFR_ID)) &  \
           !defined(NMEAGPS_DERIVED_TYPES)
  #error You must define NMEAGPS_DERIVED_TYPES in NMEAGPS.h in order to parse Talker and/or Mfr IDs!
#endif

#endif