#ifndef UBXMSG_H
#define UBXMSG_H

#include "NMEAGPS.h"
class ubloxGPS;

namespace ublox {

    enum msg_class_t
      { UBX_NAV  = 0x01,  // Navigation results
        UBX_RXM  = 0x02,  // Receiver Manager messages
        UBX_INF  = 0x04,  // Informational messages
        UBX_ACK  = 0x05,  // ACK/NAK replies to CFG messages
        UBX_CFG  = 0x06,  // Configuration input messages
        UBX_MON  = 0x0A,  // Monitoring messages
        UBX_AID  = 0x0B,  // Assist Now aiding messages
        UBX_TIM  = 0x0D,  // Timing messages
        UBX_NMEA = 0xF0,  // NMEA Standard messages
        UBX_UNK  = 0xFF
      }  __attribute__((packed));

    enum msg_id_t
      {
        UBX_ACK_NAK     = 0x00, // Reply to CFG messages
        UBX_ACK_ACK     = 0x01, // Reply to CFG messages
        UBX_CFG_MSG     = 0x01, // Configure which messages to send
        UBX_CFG_RATE    = 0x08, // Configure message rate
        UBX_CFG_NAV5    = 0x24, // Configure navigation engine settings
        UBX_MON_VER     = 0x04, // Monitor Receiver/Software version
        UBX_NAV_POSLLH  = 0x02, // Current Position
        UBX_NAV_STATUS  = 0x03, // Receiver Navigation Status
        UBX_NAV_VELNED  = 0x12, // Current Velocity
        UBX_NAV_TIMEGPS = 0x20, // Current GPS Time
        UBX_NAV_TIMEUTC = 0x21, // Current UTC Time
        UBX_NAV_SVINFO  = 0x30, // Space Vehicle Information
        UBX_ID_UNK   = 0xFF
      }  __attribute__((packed));

      struct msg_hdr_t {
          msg_class_t msg_class;
          msg_id_t    msg_id;
          bool same_kind( const msg_hdr_t & msg ) const volatile
            { return (msg_class == msg.msg_class) && (msg_id == msg.msg_id); }
      }  __attribute__((packed));

      struct msg_t : msg_hdr_t {
          uint16_t length;  // should be sizeof(this)-sizeof(msg+hdr_t)
#define UBX_MSG_LEN(msg) (sizeof(msg) - sizeof(ublox::msg_t))

          msg_t()
          {
            length    = 0;
          };
          msg_t( enum msg_class_t m, enum msg_id_t i, uint16_t l = 0 )
          {
              msg_class = m;
              msg_id    = i;
              length    = l;
          }
      } __attribute__((packed));

    /**
      * Configure message intervals.
      */

    enum ubx_nmea_msg_t { // msg_id's for UBX_NMEA msg_class
        UBX_GPGGA = 0x00,
        UBX_GPGLL = 0x01,
        UBX_GPGSA = 0x02,
        UBX_GPGSV = 0x03,
        UBX_GPRMC = 0x04,
        UBX_GPVTG = 0x05,
        UBX_GPZDA = 0x08
    } __attribute__((packed));

    struct cfg_msg_t : msg_t {
        msg_class_t  cfg_msg_class;
        msg_id_t     cfg_msg;
        uint8_t      rate;

        cfg_msg_t( msg_class_t m, msg_id_t i, uint8_t r )
          : msg_t( UBX_CFG, UBX_CFG_MSG, UBX_MSG_LEN(*this) )
        {
          cfg_msg_class = m;
          cfg_msg       = i;
          rate          = r;
        };
    } __attribute__((packed));

    extern bool configNMEA( ubloxGPS &gps, NMEAGPS::nmea_msg_t msgType, uint8_t rate );
    

    // Configure navigation rate
    enum time_ref_t {
      UBX_TIME_REF_UTC=0,
      UBX_TIME_REF_GPS=1
    } __attribute__((packed));

    struct cfg_rate_t : msg_t {
        uint16_t        GPS_meas_rate;
        uint16_t        nav_rate;
        enum time_ref_t time_ref:16;

        cfg_rate_t( uint16_t gr, uint16_t nr, enum time_ref_t tr )
          : msg_t( UBX_CFG, UBX_CFG_RATE, UBX_MSG_LEN(*this) )
        {
          GPS_meas_rate = gr;
          nav_rate      = nr;
          time_ref      = tr;
        }
    }  __attribute__((packed));

    //  Navigation Engine Expert Settings
    enum dyn_model_t {
        UBX_DYN_MODEL_PORTABLE   = 0,
        UBX_DYN_MODEL_STATIONARY = 2,
        UBX_DYN_MODEL_PEDESTRIAN = 3,
        UBX_DYN_MODEL_AUTOMOTIVE = 4,
        UBX_DYN_MODEL_SEA        = 5,
        UBX_DYN_MODEL_AIR_1G     = 6,
        UBX_DYN_MODEL_AIR_2G     = 7,
        UBX_DYN_MODEL_AIR_4G     = 8
    } __attribute__((packed));

    enum position_fix_t {
        UBX_POS_FIX_2D_ONLY = 1,
        UBX_POS_FIX_3D_ONLY = 2,
        UBX_POS_FIX_AUTO    = 3
    } __attribute__((packed));

    struct cfg_nav5_t : msg_t {
        struct parameter_mask_t {
            bool dyn_model            :1;
            bool min_elev             :1;
            bool fix                  :1;
            bool dr_limit             :1;
            bool pos_mask             :1;
            bool time_mask            :1;
            bool static_hold_thr      :1;
            bool dgps_timeout         :1;
            int  _unused_             :8;
        } __attribute__((packed));

        union {
          struct parameter_mask_t apply;
          uint16_t                apply_word;
        } __attribute__((packed));
                
        enum dyn_model_t       dyn_model:8;
        enum position_fix_t    fix_mode:8;
        int32_t                fixed_alt;          // m MSL x0.01
        uint32_t               fixed_alt_variance; // m^2 x0.0001
        int8_t                 min_elev;           // deg
        uint8_t                dr_limit;           // s
        uint16_t               pos_dop_mask;       // x0.1
        uint16_t               time_dop_mask;      // x0.1
        uint16_t               pos_acc_mask;       // m
        uint16_t               time_acc_mask;      // m
        uint8_t                static_hold_thr;    // cm/s
        uint8_t                dgps_timeout;       // s
        uint32_t always_zero_1;
        uint32_t always_zero_2;
        uint32_t always_zero_3;

        cfg_nav5_t() : msg_t( UBX_CFG, UBX_CFG_NAV5, UBX_MSG_LEN(*this) )
          {
            apply_word = 0xFF00;
            always_zero_1 =
            always_zero_2 =
            always_zero_3 = 0;
          }

    }  __attribute__((packed));

    // Geodetic Position Solution
    struct nav_posllh_t : msg_t {
        uint32_t time_of_week; // mS
        int32_t  lon; // deg * 1e7
        int32_t  lat; // deg * 1e7
        int32_t  height_above_ellipsoid; // mm
        int32_t  height_MSL; // mm
        uint32_t horiz_acc; // mm
        uint32_t vert_acc; // mm

        nav_posllh_t() : msg_t( UBX_NAV, UBX_NAV_POSLLH, UBX_MSG_LEN(*this) ) {};
    } __attribute__((packed));

    // Receiver Navigation Status
    struct nav_status_t : msg_t {
        uint32_t time_of_week; // mS
        enum status_t {
          NAV_STAT_NONE,
          NAV_STAT_DR_ONLY,
          NAV_STAT_2D,
          NAV_STAT_3D,
          NAV_STAT_GPS_DR,
          NAV_STAT_TIME_ONLY
        } __attribute__((packed))
            status;

        struct flags_t {
          bool gps_fix:1;
          bool diff_soln:1;
          bool week:1;
          bool time_of_week:1;
        } __attribute__((packed))
          flags;
        
        static gps_fix::status_t to_status( enum status_t status, flags_t flags )
        {
          if (!flags.gps_fix)
            return gps_fix::STATUS_NONE;
          if (flags.diff_soln)
            return gps_fix::STATUS_DGPS;
          switch (status) {
            case NAV_STAT_DR_ONLY  : return gps_fix::STATUS_EST;
            case NAV_STAT_2D       :
            case NAV_STAT_3D       :
            case NAV_STAT_GPS_DR   : return gps_fix::STATUS_STD;
            case NAV_STAT_TIME_ONLY: return gps_fix::STATUS_TIME_ONLY;
            default                : return gps_fix::STATUS_NONE;
          }
        }
        
        struct {
          bool dgps_input:1;
          bool _skip_:6;
          bool map_matching:1;
        }  __attribute__((packed))
          fix_status;

        enum {
          PSM_ACQUISITION,
          PSM_TRACKING,
          PSM_POWER_OPTIMIZED_TRACKING,
          PSM_INACTIVE
        }
          power_safe:2; // FW > v7.01

        uint32_t time_to_first_fix; // ms time tag
        uint32_t uptime; // ms since startup/reset

        nav_status_t() : msg_t( UBX_NAV, UBX_NAV_STATUS, UBX_MSG_LEN(*this) ) {};
    }  __attribute__((packed));

    // Velocity Solution in North/East/Down
    struct nav_velned_t : msg_t {
        uint32_t time_of_week; // mS
        int32_t  vel_north;    // cm/s
        int32_t  vel_east;     // cm/s
        int32_t  vel_down;     // cm/s
        uint32_t speed_3D;     // cm/s
        uint32_t speed_2D;     // cm/s
        int32_t  heading;      // degrees * 1e5
        uint32_t speed_acc;    // cm/s
        uint32_t heading_acc;  // degrees * 1e5
        
        nav_velned_t() : msg_t( UBX_NAV, UBX_NAV_VELNED, UBX_MSG_LEN(*this) ) {};
    }  __attribute__((packed));

    // GPS Time Solution
    struct nav_timegps_t : msg_t {
        uint32_t time_of_week;   // mS
        int32_t  fractional_ToW; // nS
        int16_t  week;
        int8_t   leap_seconds;   // GPS-UTC
        struct valid_t {
          bool time_of_week:1;
          bool week:1;
          bool leap_seconds:1;
        } __attribute__((packed))
          valid;

        nav_timegps_t() : msg_t( UBX_NAV, UBX_NAV_TIMEGPS, UBX_MSG_LEN(*this) ) {};
    }  __attribute__((packed));

    // UTC Time Solution
    struct nav_timeutc_t : msg_t {
        uint32_t time_of_week;   // mS
        uint32_t time_accuracy;  // nS
        int32_t  fractional_ToW; // nS
        uint16_t year;           // 1999..2099
        uint8_t  month;          // 1..12
        uint8_t  day;            // 1..31
        uint8_t  hour;           // 0..23
        uint8_t  minute;         // 0..59
        uint8_t  second;         // 0..59
        struct valid_t {
          bool time_of_week:1;
          bool week_number:1;
          bool UTC:1;
        } __attribute__((packed))
          valid;

        nav_timeutc_t() : msg_t( UBX_NAV, UBX_NAV_TIMEUTC, UBX_MSG_LEN(*this) ) {};
    }  __attribute__((packed));

    // Space Vehicle Information
    struct nav_svinfo_t : msg_t {
        uint32_t time_of_week;   // mS
        uint8_t  num_channels;
        enum { ANTARIS_OR_4, UBLOX_5, UBLOX_6 } chipgen:8;
        uint16_t reserved2;
        struct sv_t {
          uint8_t channel; // 255 = no channel
          uint8_t id;      // Satellite ID
          bool    used_for_nav:1;
          bool    diff_corr   :1; // differential correction available
          bool    orbit_avail :1; // orbit info available
          bool    orbit_eph   :1; // orbit info is ephemeris
          bool    unhealthy   :1; // SV should not be used
          bool    orbit_alm   :1; // orbit info is Almanac Plus
          bool    orbit_AOP   :1; // orbit info is AssistNow Autonomous
          bool    smoothed    :1; // Carrier smoothed pseudorange used
          enum {
              IDLE, 
              SEARCHING, 
              ACQUIRED, 
              UNUSABLE, 
              CODE_LOCK, 
              CODE_AND_CARRIER_LOCK_1,
              CODE_AND_CARRIER_LOCK_2,
              CODE_AND_CARRIER_LOCK_3
            }
              quality:8;
          uint8_t  snr;           // dbHz
          uint8_t  elevation;     // degrees
          uint16_t azimuth;       // degrees
          uint32_t pr_res;        // pseudo range residual in cm
        };

        //  Calculate the number of bytes required to hold the
        //  specified number of channels.
        static uint16_t size_for( uint8_t channels )
          { return sizeof(nav_svinfo_t) + (uint16_t)channels * sizeof(sv_t); }

        // Initialze the msg_hdr for the specified number of channels
        void init( uint8_t max_channels )
        {
          msg_class = UBX_NAV;
          msg_id    = UBX_NAV_SVINFO;
          length    = size_for( max_channels ) - sizeof(ublox::msg_t);
        }

      }  __attribute__((packed));

};

#endif