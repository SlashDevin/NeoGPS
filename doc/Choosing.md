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
    <td><p>PUBX,00<sup>1<sup></p><p><br></p></td>
    <td><p>PUBX,04<sup>1<sup></p><p><br></p></td>
  </tr>
  <tr>
    <td>status</td>
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
    <td>date</td>
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
    <td>time</td>
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
    <td>lat/lon</td>
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
    <td>altitude</td>
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
    <td>speed</td>
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
    <td>heading</td>
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
    <td>lat, lon and<br>alt error</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td>number of<br>satellites</td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td>*</td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td>*</td>
    <td> </td>
  </tr>
  <tr>
    <td>satellite IDs</td>
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
    <td>satellite azimuth,<br>&nbsp;&nbsp;elevation and<br>&nbsp;&nbsp;signal strength</td>
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
  <tr>
    <td>HDOP</td>
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
    <td>VDOP</td>
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
    <td>PDOP</td>
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
</table>

This table illustrates the poor design of the NMEA message set: it requires multiple messages to deliver a complete fix (i.e., all members of `gps_fix`).  This also explains why many manufacturers provide proprietary messages that *are* more complete.  Above, you can see that the `$PUBX,00`<sup>1</sup> message contains all members except `date`.

While the manufacturer's specification will document all sentences supported for your device, you can also find general descriptions of many NMEA sentences [here](http://www.gpsinformation.org/dale/nmea.htm), [here](http://aprs.gids.nl/nmea/) or [here](http://www.catb.org/gpsd/NMEA.txt).

<hr>
<sub><sup>1</sup> The NMEA proprietary messages "PUBX" are only availble in the `ubloxNMEA` class.  See [ublox-specific](ublox.md) instructions for adding this class to your configuration.</sub>
