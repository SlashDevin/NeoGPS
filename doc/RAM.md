RAM requirements
=======
As data is received from the device, various portions of a `fix` are 
modified.  In fact, _**no buffering RAM is required**_.  Each character 
affects the internal state machine and may also contribute to a data member 
(e.g., latitude). 

The **Minimal** configuration requires only 2 bytes, and the NMEA state 
machine requires 7 bytes, for a total of **10 bytes** (structure alignment 
may add 1 byte). 

The **DTL** configuration requires 18 bytes, for a total of **25 bytes**.

The **Nominal** configuration requires only 34 bytes, for a total of **41 
bytes**.  For comparison, TinyGPS requires about 180 bytes (120 bytes for 
members plus about 60 bytes of string data), and TinyGPS++ requires about 
240 bytes.

The **Full** configuration requires 44 bytes, and the full NMEA message set 
configuration adds 130 bytes of satellite data, for a total of **174 
bytes**.  For comparison, satellite tracking in TinyGPS++ requires over 1100 
bytes.

If your application only requires an accurate one pulse-per-second, you 
can configure it to parse *no* sentence types and retain *no* data members. 
This is the **Minimal** configuration.  Although the 
`fix().status` can be checked, no valid flags are available.  Even 
though no sentences are parsed and no data members are stored, the 
application will  still receive a `decoded` message type once per second:

```
while (uart1.available())
  if (gps.decode( uart1.getchar() )) {
    if (gps.nmeaMessage == NMEAGPS::NMEA_RMC)
      sentenceReceived();
  }
```

The `ubloxNMEA` derived class doesn't use any extra bytes of RAM.

The `ubloxGPS` derived class adds 20 bytes to handle the more-complicated protocol, 
plus 5 static bytes for converting GPS time and Time Of Week to UTC.

