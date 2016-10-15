#Choosing your configuration
There are only a few configurations provided by examples.  If your application needs something slightly different, here is a general configuration process.

##What number do you want?
First, decide which data members of `gps_fix` and `NMEAGPS` you need (see [Data Model](Data Model.md) for member descriptions).  Those members **must** be enabled in `GPSfix_cfg.h`.

Next, figure out what messages can fill out those members, because those messages **must** be enabled in `NMEAGPS_cfg.h`.    Here is a table of the NMEA messages parsed by NeoGPS, and which data members they affect:

<table>
  <tr>
    <td><p align="right"><b>Message</b></p><p><b>Data Member</b></p></td>
    <td><p>GGA</p><p><br></p></td>
    <td><p>GLL</p><p><br></p></td>
    <td><p>GSA</p><p><br></p></td>
    <td><p>GST</p><p><br></p></td>
    <td><p>GSV</p><p><br></p></td>
    <td><p>RMC</p><p><br></p></td>
    <td><p>VTG</p><p><br></p></td>
    <td><p>ZDA</p><p><br></p></td>
    <td><p align="center">PUBX<br>00<sup>1<sup></p><p><br></p></td>
    <td><p align="center">PUBX<br>04<sup>1<sup></p><p><br></p></td>
  </tr>
  <tr><td><p><b>class gps_fix</b></p></td></tr>
  <tr>
    <td><p align="right">status</p></td>
    <td>*</td>
    <td>*</td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">date<sup>2</sup></p></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td>*</td>
</tr>
  <tr>
    <td><p align="right">time<sup>2</sup></p></td>
    <td>*</td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td>*</td>
    <td>*</td>
  </tr>
  <tr>
    <td><p align="right">lat/lon</p></td>
    <td>*</td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">altitude</p></td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">speed</p></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">heading</p></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">lat, lon, alt error</p></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">satellites</p></td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">HDOP</p></td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">VDOP</p></td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">PDOP</p></td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">TDOP</p></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr><td><p><b>class NMEAGPS</b></p></td></tr>
  <tr>
    <td><p align="right">satellite IDs</p></td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><p align="right">satellite azimuth,<br>&nbsp;&nbsp;elevation and<br>&nbsp;&nbsp;signal strength</p></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
</table>

This table illustrates the poor design of the NMEA message set: it requires multiple messages to deliver a complete fix (i.e., all members of `gps_fix`).  This also explains why many manufacturers provide proprietary messages that *are* more complete.  Above, you can see that the `$PUBX,00`<sup>1</sup> message contains all members except `date`.

While the manufacturer's specification will document all sentences supported for your device, you can also find general descriptions of many NMEA sentences [here](http://www.gpsinformation.org/dale/nmea.htm), [here](http://aprs.gids.nl/nmea/) or [here](http://www.catb.org/gpsd/NMEA.txt).

<hr>
<sub><sup>1</sup>  The NMEA proprietary messages "PUBX" are only availble in the `ubloxNMEA` class.  See [ublox-specific](ublox.md) instructions for adding this class to your configuration.</sub>

<sub><sup>2</sup>  Date and time are both stored in one member of `gps_fix`, called `dateTime`.  The `fix.dateTime` member is a C++ class that has both date-oriented members (Date, Month and Year) and time-oriented members (Hours, Minutes and Seconds). See [Time.h](/Time.h) for the complete description and capabilities of the `dateTime` member, such as date/time arithmetic and conversion to/from seconds since the epoch.  Hundredths of a second are stored in a separate member of `gps_fix`, called `dateTime_cs`.</sub>
