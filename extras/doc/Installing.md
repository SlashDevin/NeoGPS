Installing
==========

1. [Download the library](#1-download-the-library)
2. [Choose a serial port](#2-choose-a-serial-port)
3. [Connect the GPS device](#3-connect-the-gps-device)
4. [Review `GPSport.h`](#4-review-librariesneogpssrcgpsporth)
5. [Open the example](#5--open-the-example-sketch-nmeaino)
6. [Build and upload](#6--build-and-upload-the-sketch-to-your-arduino)
<hr>

### 1. Download the library

It is easiest to use the [Ardino IDE Library Manager](https://www.arduino.cc/en/Guide/Libraries#toc3) to automatically download and install NeoGPS.  Select the menu **Sketch -> Include Library -> Manage Libraries**.  Then type "NeoGPS" in the Search box.

If you need to perform a manual installation,:

* Download the [master ZIP file](https://github.com/SlashDevin/NeoGPS/archive/master.zip).
*  Open the zip file and open the nested `NeoGPS-master` subdirectory.
*  Select and copy all files in the `NeoGPS-master` subdirectory into a new `Arduino/Libraries/NeoGPS` directory, like most libraries.  The `Arduino/Libraries/NeoGPS` directory should contain:<br>
```
extras
examples
src
library.properties
README.md
```

<hr>

### 2. Choose a serial port

**BEST**: The fastest, most reliable way to connect a GPS device is to use a HardwareSerial port.

On any Arduino board, you can connect the GPS device to the `Serial` pins (0 & 1).  You can still print debug statements, and they will show up on the Serial Monitor window.  The received GPS characters will not interfere with those prints, and you will not see those characters on the Serial Monitor window.

However, when you need to upload a new sketch to the Arduino, **you must disconnect the GPS TX from the Arduino RX pin 0.**  Otherwise, the GPS characters will interfere with the upload data.

For Mega, Due and Teensy boards, you can connect the GPS device to the `Serial1`,  `Serial2` or `Serial3` pins.

For Micro and Leo (and other 32U4-based Arduinos), you can connect the GPS device to the `Serial1` pins.

**2nd Best**:  If you can't connect the GPS device to a HardwareSerial port, you should download and install the [AltSoftSerial](https://github.com/PaulStoffregen/AltSoftSerial) or [NeoICSerial](https://github.com/SlashDevin/NeoICSerial) library.  These libraries only work on two specific pins (8 & 9 on an UNO).  This library is very efficient and reliable.  It uses one of the hardware TIMERs, so it may conflict with libraries that use TIMERs or PWM output (e.g., servo).

**3rd Best**:  If you can't use the pins required by `AltSoftSerial`, and your GPS device runs at 9600, 19200 or 38400 baud, you should download and install the [NeoSWSerial](https://github.com/SlashDevin/NeoSWSerial) library.  This library is almost as efficient.  It will help you avoid common timing problems caused by `SoftwareSerial`.  It does not need an extra TIMER, so it can be used with most other libraries.  It does use Pin Change Interrupts, but there is an option in the header file that allows you to coordinate other PCI usage with `NeoSWSerial`.

`NeoSWSerial` can be used with `AltSoftSerial` at the same time, allowing your sketch to have two extra serial ports.

**WORST**:  `SoftwareSerial` is NOT RECOMMENDED, because it disables interrupts for long periods of time.  This can interfere with other parts of your sketch, or with other libraries.  It cannot transmit and receive at the same time, and your sketch can only receive from one `SoftwareSerial` instance at time.

<hr>

### 3. Connect the GPS device

Most GPS devices are 3.3V devices, and most Arduinos are 5V devices.  Although many GPS modules are described as "3V & 5V compatible", 

<p align=center><b>YOU SHOULD NOT CONNECT A 5V ARDUINO TRANSMIT PIN TO THE 3.3V GPS RX PIN</b></p>

This can damage the device, cause overheating, system power problems or decrease the lifetime of battery-operated systems.  You must level-shift this connection with inexpensive level-shifting modules (best) or a resistor divider.

Connecting the 3.3V GPS TX pin to a 5V Arduino receive pin will not damage the GPS device, but it may not be reliable.  This is because the GPS TX voltage is slightly lower than what the Arduino requires.  It works in many situations, but if you are not able to receive GPS characters reliably, you probably need to use a level-shifting module (best) or a diode+resistor to "pull up" the GPS TX pin voltage.

<hr>

### 4. Review `Libraries/NeoGPS/src/GPSport.h`

This file chooses a default serial port for each type of Arduino.  You can either declare your own `gpsPort` variable in the .INO file, or you should confirm GPSport.h will choose the correct serial port for your GPS device.

By default, Mega Boards will use `Serial1`.  If you have installed the [NeoHWSerial](https://github.com/SlashDevin/NeoHWSerial) library and included the header before `GPSport.h`, then `NeoSerial1` will be used.

If you have included the `AltSoftSerial` header befor GPSport.h, its specific pins will be used for the `gpsPort` (8 & 9 on an UNO).

For all other Boards, a `NeoSWSerial` instance will be created on pins 3 and 4.  If your GPS is on different pins, put these `#define` lines in the INO, before the `#include "GPSport.h"` line:

    #define RX_PIN 2
    #define TX_PIN 3
    #include "GPSport.h"

All the example programs can use any of the following serial port types:

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

Modify these defaults if necessary, or if you know what serial port to use, you can declare it yourself.  Be sure to delete the line `#include "GPSport.h"`, and delete the file `GPSport.h`.

<hr>

### 5.  Open the example sketch NMEA.ino

In the Arduino IDE, select **File -> Examples -> NeoGPS -> NMEA**.

<hr>

### 6.  Build and upload the sketch to your Arduino.

**Note:**  If the sketch does not compile, please see the [Troubleshooting](Troubleshooting.md#configuration-errors) section.

When the sketch begins, you should see this:
```
NMEA.INO: started
  fix object size = 31
  gps object size = 84
Looking for GPS device on Serial1

GPS quiet time is assumed to begin after a RMC sentence is received.
  You should confirm this with NMEAorder.ino

Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,Sats,Rx ok,Rx err,Rx chars,
3,2016-05-24 01:21:29.00,472852332,85652650,,138,,,1,0,66,
3,2016-05-24 01:21:30.00,472852311,85652653,,220,24040,7,9,0,557,
3,2016-05-24 01:21:31.00,472852315,85652647,,449,24080,7,17,0,1048,
  etc.
```
The default NeoGPS configuration is **Nominal**, as described [here](Configurations.md#typical-configurations).  This output can be copy & pasted into a spreadsheet for graphing or analysis, or into a text editor for saving as a CSV file.

If you do not see this output, please review the  [Troubleshooting](Troubleshooting.md#gps-device-connection-problems) section.

<hr>

### The NMEA.ino example works!
Once you have verified the GPS device connection and build process with this first example, you should also verify your device's behavior with `NMEAorder.ino` (see [this section](Troubleshooting.md#quiet-time-interval)).  This can avoid problems later on, when you start adding/merging other functions to do your "work".

[Other examples](Examples.md) include `NMEAloc.ino`, which shows how to use just the location fields of a fix, or `NMEAtimezone.ino`, which shows how to adjust the GPS time for your local time zone.

If you are logging information to an SD card, you should next try `NMEA_isr.ino`.  It is identical to `NMEA.ino`, except that it handles the GPS characters during the RX char interrupt.  Interrupt handling will require one of the NeoXXSerial libraries to be installed (e.g. [NeoHWSerial](https://github.com/SlashDevin/NeoHWSerial)).

If you are working on a drone or other autonomous system, you should you should read about [Coherency](Coherency.md) and the interrupt-driven technique in [NMEA_isr](/examples/NMEA_isr/NMEA_isr.ino).

You can also try other configurations.  Please see [Choosing Your Configuration](Choosing.md) for more information, and then simply edit `GPSfix_cfg.h` and/or `NMEAGPS_cfg.h`, or select an [example configuration](../configs) and copy these three files into your application directory: `NeoGPS_cfg.h`, `GPSfix_cfg.h`, and `NMEAGPS_cfg.h`.

You can review and edit each of the copied configuration files to add or remove messages or fields, at any time.

**Note:**  Not all configurations will work with all example applications.  Compiler error messages are emitted for incompatible settings, or if an example requires certain configurations.

### I have a ublox GPS device
After you have tried all the standard NMEA examples, and you need the ublox-specific capabilities of NeoGPS, please see the [ublox](ublox.md) section.  Try `PUBX.ino` first, then try `ublox.ino` if you *really* need the binary protocol.
