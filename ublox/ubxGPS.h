#ifndef _UBXGPS_H_
#define _UBXGPS_H_

/**
 * @file UBXGPS.h
 * @version 2.1
 *
 * @section License
 * Copyright (C) 2014, SlashDevin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "ubxNMEA.h"
#include "ubxmsg.h"
#include "GPSTime.h"

#include <Stream.h>
#include <stddef.h>

// NOTE: millis() is used for ACK timing

/**
 * Enable/disable the parsing of specific UBX messages.
 *
 * Configuring out a message prevents its fields from being parsed.
 * However, the message type will still be recognized by /decode/ and 
 * stored in member /rx_msg/.  No valid flags would be available.
 */

#define UBLOX_PARSE_STATUS
#define UBLOX_PARSE_TIMEGPS
#define UBLOX_PARSE_TIMEUTC
#define UBLOX_PARSE_POSLLH
#define UBLOX_PARSE_VELNED
#define UBLOX_PARSE_SVINFO
//#define UBLOX_PARSE_CFGNAV5
//#define UBLOX_PARSE_MONVER
 


class ubloxGPS : public ubloxNMEA
{
    ubloxGPS( const ubloxGPS & );

public:

    // Constructor needs to know the device to handle the UBX binary protocol
    ubloxGPS( Stream *device )
      :
        storage( (ublox::msg_t *) NULL ),
        reply( (ublox::msg_t *) NULL ),
        reply_expected( false ),
        ack_expected( false ),
        m_device( device )
      {};

    /**
     * Process one character of ublox message.  The internal state 
     * machine tracks what part of the sentence has been received.  As the
     * tracks what part of the sentence has been received so far.  As the
     * sentence is received, members of the /fix/ structure are updated.  
     * @return DECODE_COMPLETED when a sentence has been completely received.
     */
    decode_t decode( char c );

    /**
     * Received message header.  Payload is only stored if /storage/ is 
     * overridden for that message type.
     */
    ublox::msg_t & rx() { return m_rx_msg; }

    bool enable_msg( ublox::msg_class_t msg_class, ublox::msg_id_t msg_id )
    {
      return send( ublox::cfg_msg_t( msg_class, msg_id, 1 ) );
    }
    bool disable_msg( ublox::msg_class_t msg_class, ublox::msg_id_t msg_id )
    {
      return send( ublox::cfg_msg_t( msg_class, msg_id, 0 ) );
    }
    
    /**
     * Send a message (non-blocking).
     *    Although multiple /send_request/s can be issued,
     *      replies will cause multiple dispatches to /on_event/
     */
    bool send_request( const ublox::msg_t & msg )
    {
      write( msg );
      return true;
    };
    bool send_request_P( const ublox::msg_t & msg )
    {
      write_P( msg );
      return true;
    };

    /**
     * Send a message and wait for a reply (blocking).
     *    No event will be generated for the reply.
     *    If /msg/ is a UBX_CFG, this will wait for a UBX_CFG_ACK/NAK
     *      and return true if ACKed.
     *    If /msg/ is a poll, this will wait for the reply.
     *    If /msg/ is neither, this will return true immediately.
     *    If /msg/ is both, this will wait for both the reply and the ACK/NAK.
     *    If /storage_for/ is implemented, those messages will continue
     *      to be saved while waiting for this reply.
     */
    bool send( const ublox::msg_t & msg, ublox::msg_t *reply_msg = (ublox::msg_t *) NULL );
    bool send_P( const ublox::msg_t & msg, ublox::msg_t *reply_msg = (ublox::msg_t *) NULL );

    //  Ask for a specific message (non-blocking).
    //     /on_event/ will receive the header later.
    //  See also /send_request/.
    bool poll_request( const ublox::msg_t & msg )
    {
//trace << '?' << msg.msg_class << '/' << msg.msg_id << ' ';
      ublox::msg_t poll_msg( msg.msg_class, msg.msg_id, 0 );
      return send_request( poll_msg );
    };
    bool poll_request_P( const ublox::msg_t & msg )
    {
      ublox::msg_t poll_msg( (ublox::msg_class_t) pgm_read_byte( &msg.msg_class ),
                      (ublox::msg_id_t) pgm_read_byte( &msg.msg_id ), 0 );
      return send_request( poll_msg );
    };

    //  Ask for a specific message (blocking).
    //    See also /send/.
    bool poll( ublox::msg_t & msg )
    {
      ublox::msg_t poll_msg( msg.msg_class, msg.msg_id, 0 );
      return send( poll_msg, &msg );
    };
    bool poll_P( const ublox::msg_t & msg, ublox::msg_t *reply_msg = (ublox::msg_t *) NULL )
    {
      ublox::msg_t poll_msg( (ublox::msg_class_t) pgm_read_byte( &msg.msg_class ),
                      (ublox::msg_id_t) pgm_read_byte( &msg.msg_id ), 0 );
      return send( poll_msg, reply_msg );
    };

    //  Return the Stream that was passed into the constructor.
    Stream *Device() const { return (Stream *)m_device; };

protected:

    #if defined( GPS_FIX_LAT_ERR ) | defined( GPS_FIX_LON_ERR ) | \
        defined( GPS_FIX_ALT_ERR )
      // Well, crud.  The NAV_POSLLH message has 4-byte received errors in mm.
      // These must be converted to the 2-byte gps_fix errors in cm.
      // There's no easy way to perform this conversion as the bytes are
      // being received, especially when the LSB is received first.
      // Hold them here, then divide.
      union {
        uint32_t U4;
        uint16_t U2[2];
        uint8_t  U1[4];
      };
    #endif

    /*
     * Some UBX messages can be larger than 256 bytes, so
     * hide the 8-bit NMEAGPS::chrCount with this 16-bit version.
     */
    uint16_t chrCount;

    bool parseField( char chr );

    enum ubxState_t {
        UBX_IDLE  = NMEA_IDLE,
        UBX_SYNC2 = NMEA_LAST_STATE+1,
        UBX_HEAD,
        UBX_RECEIVING_DATA,
        UBX_CRC_A,
        UBX_CRC_B
    };
    static const ubxState_t UBX_FIRST_STATE = UBX_SYNC2;
    static const ubxState_t UBX_LAST_STATE  = UBX_CRC_B;

    inline void write
      ( uint8_t c, uint8_t & crc_a, uint8_t & crc_b ) const
    {
        m_device->print( (char) c );
        crc_a += c;
        crc_b += crc_a;
    };
    void write( const ublox::msg_t & msg );
    void write_P( const ublox::msg_t & msg );

    void wait_for_idle();
    bool wait_for_ack();
    bool waiting() const
    {
      return (ack_expected && (!ack_received && !nak_received)) ||
             (reply_expected && !reply_received);
    }
    bool receiving() const
    {
      return (rxState != (rxState_t)UBX_IDLE) || (m_device && m_device->available());
    }

    //  /run/ is called from inside /wait_for_idle/.
    //
    //  If a derived class processes incoming chars in the background
    //  (e.g., a derived IOStream::Device::/putchar/ called from the IRQ),
    //  this default method is sufficient.
    //
    //  If /this/ instance processes characters in the foreground,
    //  /run/ must be provided to continue decoding the input stream while 
    //  waiting for UBX replies.  For example,
    //
    //            while (uart.available())
    //              decode( uart.getchar() );

    virtual void run() { };

    // Override this if the contents of a particular message need to be saved.
    // This may execute in an interrupt context, so be quick!
    //  NOTE: the ublox::msg_t.length will get stepped on, so you may need to
    //  set it every time if you are using a union for your storage.
    virtual ublox::msg_t *storage_for( const ublox::msg_t & rx_msg )
      { return (ublox::msg_t *)NULL; };

private:
    ublox::msg_t   *storage;   // cached ptr to hold a received msg.

    // Storage for a specific received message.
    //   Used internally by send & poll variants.
    //   Checked and used before /storage_for/ is called.
    ublox::msg_t   *reply;

    struct {
      bool     reply_expected NEOGPS_BF(1);
      bool     reply_received NEOGPS_BF(1);
      bool     ack_expected NEOGPS_BF(1);
      bool     ack_received NEOGPS_BF(1);
      bool     nak_received NEOGPS_BF(1);
      bool     ack_same_as_sent NEOGPS_BF(1);
    } NEOGPS_PACKED;
    struct ublox::msg_hdr_t sent;

    struct rx_msg_t : ublox::msg_t
    {
      uint8_t  crc_a;   // accumulated as packet received
      uint8_t  crc_b;   // accumulated as packet received

      rx_msg_t()
      {
        init();
      }

      void init()
      {
        msg_class = ublox::UBX_UNK;
        msg_id    = ublox::UBX_ID_UNK;
        length    = 0;
        crc_a = 0;
        crc_b = 0;
      }

    } NEOGPS_PACKED;

    rx_msg_t m_rx_msg;

    void rxBegin();
    bool rxEnd();

    static const uint8_t SYNC_1 = 0xB5;
    static const uint8_t SYNC_2 = 0x62;

    Stream *m_device;

    bool parseFix( uint8_t c );

    bool parseTOW( uint8_t chr )
    {
      #if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE)
        if (chrCount == 0) {
          m_fix.valid.date =
          m_fix.valid.time = false;
        }

        ((uint8_t *) &m_fix.dateTime)[ chrCount ] = chr;

        if (chrCount == 3) {
          uint32_t tow = *((uint32_t *) &m_fix.dateTime);
          //trace << PSTR("@ ") << tow;

          uint16_t ms;
          if (GPSTime::from_TOWms( tow, m_fix.dateTime, ms )) {
            m_fix.dateTime_cs = ms / 10;
            m_fix.valid.time = true;
            m_fix.valid.date = true;
          } else
            m_fix.dateTime.init();
          //trace << PSTR(".") << m_fix.dateTime_cs;
        }
      #endif

      return true;
    }

} NEOGPS_PACKED;

#endif
