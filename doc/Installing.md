Installing
==========
**1.**  Download the [master ZIP file](https://github.com/SlashDevin/NeoGPS/archive/master.zip).  For most non-Mega boards, like UNOs, you should also download and install the [NeoSWSerial](https://github.com/SlashDevin/NeoSWSerial) library.


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

By default, Mega Boards will use `Serial1`.  If you have installed the [NeoHWSerial](https://github.com/SlashDevin/NeoHWSerial) library and included the header before `GPSport.h`, then `NeoSerial1` will be used.

For all other Boards, a software serial instance will be created on pins 3 and 4.   The examples can use any of the following:

* [NeoSWSerial](https://github.com/SlashDevin/NeoSWSerial) (default, works on most pins)
* [NeoICSerial](https://github.com/SlashDevin/NeoICSerial) (only works on one specific Input Capture pin)
* [AltSoftSerial](https://github.com/PaulStoffregen/AltSoftSerial) (only works on one specific Input Capture pin)
* SoftwareSerial (built-in, not recommended)

To select one of the non-default types, simply include their header before including `GPSport.h`:

    //#include <NeoHWSerial.h>
    #include <NeoICSerial.h>
    //#include <NeoSWSerial.h>
    //#include <SoftwareSerial.h> /* NOT RECOMMENDED */
    #include "GPSport.h"

The above will cause `GPSport.h` to declare `gps_port` using the class `NeoICSerial`.

Modify these defaults if necessary, or if you know what serial port to use, you can declare it in `NMEA.ino`.  Be sure to delete the line `#include "GPSport.h"`, and delete the file `GPSport.h`.


**5.**  Start the IDE by double-clicking on the `NMEA.INO` file and upload the example sketch.

**Note:**  If the sketch does not compile, please see the [Troubleshooting](Troubleshooting.md#configuration-errors) section.

When the sketch begins, you should see a few lines of startup text:
```
NMEA.ino: started
fix object size = 34
NMEAGPS object size = 53
Looking for GPS device on Serial1
GPS quiet time begins after a GLL sentence is received.
You should confirm this with NMEAorder.ino
```
If the GPS device is correctly wired but running at the wrong baud rate, you may see:

    Invalid data received.  Use NMEAdiagnostic.INO to verify baud rate.

If it is running at the specified baud rate, you should see the header for the GPS data fields, and a few messages that report which NMEA sentences are being received.  Depending on when the Arduino resets within the GPS reporting interval, and your specific GPS device, you may see different sentence reports:
```
Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,HDOP,Sats,Rx ok,Rx err,Rx chars,
Received RMC...
Received VTG...
Received GGA...
Received GSA...
Received GSV...
Received GSV...
Received GSV...
```
When the LAST_SENTENCE_IN_INTERVAL is received (default is a GLL), it begins printing the GPS data fields in a CSV format.  If the GPS device has a fix on several satellites, you will see something like this:
```
3,2015-09-14 19:07:30.00,472852332,85652650,,1262,,,,8,0,501,
3,2015-09-14 19:07:31.00,472852338,85652646,,1678,,,,16,0,1004,
  etc.
```
If your device does not send the expected LAST_SENTENCE_IN_INTERVAL, you will receive a warning:
```
Warning: GLL sentence was never received and is not the LAST_SENTENCE_IN_INTERVAL.
  Please use NMEAorder.ino to determine which sentences your GPS device sends, and then
  use the last one for the definition above.
```
As described in the [Troubleshooting](Troubleshooting.md#gps-device-connection-problems) section, you should run the NMEAorder.ino sketch to determine which sentence is last.  You may be able to see a slight pause in the "Received XXX" messages above, which would also identify the last sentence.  Edit NMEA.ino and change this line:
```
static const NMEAGPS::nmea_msg_t LAST_SENTENCE_IN_INTERVAL = NMEAGPS::NMEA_GLL;
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
