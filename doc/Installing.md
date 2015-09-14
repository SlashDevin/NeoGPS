Installing
==========
**1.**  Download the [master ZIP file](https://github.com/SlashDevin/NeoGPS/archive/master.zip).

**2.**  Copy any of the example subdirectories into your main Arduino directory:
```
NeoGPS-master/examples/NMEA  ->  Arduino
```
This creates an `Arduino/NMEA` sketch subdirectory containing `NMEA.INO`.

**3.**  Unfortunately, the sad state of Arduino library management requires you to copy copy all the .H and .CPP files from the top directory of NeoGPS-master into *each* example subdirectory that you would like to try.  :(  Fortunately, this will allow you to have a different configuration for each example. :)

You do not need the files from any other subdirectories, like **ublox**.  Most of the example programs only use these generic NMEA files:
```
    CosaCompat.h
    GPSfix.h
    GPSfix_cfg.h
    NMEAGPS.cpp
    NMEAGPS.h
    NMEAGPS_cfg.h
    NeoGPS_cfg.h
    Streamers.cpp
    Streamers.h
    Time.cpp
    Time.h
```
Your application subdirectory should now contain these 11 files, plus `NMEA.INO`.

**4.**  The default NeoGPS configuration is **Nominal**, as described [here](Configurations.md#typical-configurations).

If you would like to try a different configuration, select an [example configuration](../configs) and copy these three files into your application directory: `NeoGPS_cfg.h`, `GPSfix_cfg.h`, and `NMEAGPS_cfg.h`.

You can also review and edit each of the copied configuration files to add or remove messages or fields.

**Note:**  Not all configurations will work with all example applications.

**5.** Review the example `NMEA.INO` to confirm that the correct serial port will be used for your GPS device.

By default, Mega Boards will use `Serial1`.  For all other Boards, a `SoftwareSerial` instance will be created on pins 3 and 4.  Modify these defaults if necessary.

**6.**  Start the IDE by double-clicking on the `NMEA.INO` file and upload the example sketch.

**Note:**  If the sketch does not compile, please see the [Troubleshooting](Troubleshooting.md#configuration-errors) section.

When the sketch begins, you should see a few lines of startup text:
```
NMEA.ino: started
fix object size = 44
NMEAGPS object size = 269
Looking for GPS device on Serial1
```
If the GPS device has a fix on several satellites, you will see the received GPS data as comma-separated values:
```
Local time,Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,
2015-09-14 15:07:30,3,2015-09-14 19:07:30.00,472852332,85652650,,1262,,
2015-09-14 15:07:31,3,2015-09-14 19:07:31.00,472852338,85652646,,1678,,
  etc.
```
This output can be copy & pasted into a spreadsheet for graphing or analysis, or into a text editor for saving as a CSV file.

Please see the [Troubleshooting](Troubleshooting.md#gps-device-connection-problems) section if you do not see this output.

#ublox-specific capabilities
Once you have verified the GPS device connection and build process with the generic NMEA examples, you may want to try the  ublox-specific capabilities of NeoGPS.  These are only valid for ublox GPS devices: Neo-6, Neo-7 and Neo8 have been tried, and other ublox variant may work.  Please compare the ublox protocol documents to verify compatibility.

##ublox-specific NMEA messages
If you want to handle $PUBX messages from a ublox Neo GPS device, you must copy the above files *and* also copy the ublox/ubxNMEA.* files into your application directory.  (This is required if you are trying the example/PUBX/PUBX.ino application.)

##ublox-specific binary protocol
If you want to handle the UBX binary protocol from a ublox Neo GPS device, you must copy the above files *and* also copy the ublox/ubxGPS.* and ublox/ubxmsg.* into your application directory.  (This is required if you are trying the example/ublox/ublox.ino application.)

To use ublox-specific messages, you must enable the following:
```
#define NMEAGPS_DERIVED_TYPES
```

You may also want to change the configured PUBX messages in ubxNMEA.h, or the UBX binary messages in ubxGPS.h.  They are currently configured to work with the example applications PUBX.ino and ublox.ino, respectively.
