Installing
==========
**1.**  Download the [master ZIP file](https://github.com/SlashDevin/NeoGPS/archive/master.zip).


**2.**  Starting with the first example program, `NMEA.ino`, copy this subdirectory into your main Arduino directory:
```
NeoGPS-master/examples/NMEA  ->  Arduino
```
This creates an `Arduino/NMEA` sketch subdirectory containing `NMEA.INO`.


**3.**  Copy all the .H and .CPP files from the top directory of NeoGPS-master into `Arduino/NMEA`.  Your application subdirectory should now contain these files:
```
    CosaCompat.h
    GPSfix.h
    GPSfix_cfg.h
    GPSport.h
    NMEA.ino
    NMEAGPS.cpp
    NMEAGPS.h
    NMEAGPS_cfg.h
    NeoGPS_cfg.h
    Streamers.cpp
    Streamers.h
    Time.cpp
    Time.h
```
You do not need the files from any other subdirectories, like **ublox**.  Most of the example programs only use these generic NMEA files.

This is different from most libraries, which are usually copied to the `Arduino/Libraries` subdirectory.  Unfortunately, the sad state of Arduino library management requires you to copy the NeoGPS files into *each* example subdirectory that you would like to try.  :(  Fortunately, this will allow you to have a different configuration for each example. :)

**4.** Review the example `GPSport.h` to confirm that the correct serial port will be used for your GPS device.

By default, Mega Boards will use `Serial1`.  For all other Boards, a `SoftwareSerial` instance will be created on pins 3 and 4.   Modify these defaults if necessary, or if you know what serial port to use, you can declare it in `NMEA.ino`.  Be sure to delete the line `#include "GPSport.h"`, and delete the file `GPSport.h`.


**5.**  Start the IDE by double-clicking on the `NMEA.INO` file and upload the example sketch.

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
The default NeoGPS configuration is **Nominal**, as described [here](Configurations.md#typical-configurations).  This output can be copy & pasted into a spreadsheet for graphing or analysis, or into a text editor for saving as a CSV file.

If you do not see this output, please see the [Troubleshooting](Troubleshooting.md#gps-device-connection-problems) section.

#The NMEA.ino example works!
Once you have verified the GPS device connection and build process with this first example, you should also verify your device's behavior with `NMEAorder.ino` (see [this section](Troubleshooting.md#quiet-time-interval)).  This can avoid problems later on, when you start adding/merging other functions to do your "work".

Next, you should try `NMEAfused.ino`.

If you are working on a drone or other autonomous system, you should then try `NMEAcoherent.ino`.

You can also try other configurations.  Please see [Choosing Your Configuration](Choosing.md) for more information, and then simply edit `GPSfix_cfg.h` and/or `NMEAGPS_cfg.h`, or select an [example configuration](../configs) and copy these three files into your application directory: `NeoGPS_cfg.h`, `GPSfix_cfg.h`, and `NMEAGPS_cfg.h`.

You can review and edit each of the copied configuration files to add or remove messages or fields, at any time.

**Note:**  Not all configurations will work with all example applications.

#I have a ublox GPS device
After you have tried all the standard NMEA examples, and you need the ublox-specific capabilities of NeoGPS, please see the [ublox](ublox.md) section.  Try `PUBX.ino` first, then try `ublox.ino` if you *really* need the binary protocol.
