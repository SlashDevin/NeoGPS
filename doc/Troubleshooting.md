####Troubleshooting

###Configurations
Because there are so many configurable items, it is possible that your configuration prevents acquiring the desired GPS information.

The compiler **cannot** catch message set dependencies: the enum 
`nmea_msg_t` is always available.  So even though a `fix` member is enabled, 
you may have disabled all messages that would have set its value.  
NMEAtest.ino can be used to check some configurations.

For example, if your application needs altitude, you **must** enable the GGA sentence.  No other sentence provides the altitude member.  If `NMEA_PARSE_GGA` is not defined,  `gps.decode()` will return COMPLETED after a GGA is received, but no parts of the GGA sentence will have been parsed, and altitude will never be valid.  NeoGPS will _recognize_ the GGA sentence, but it will not be parsed.


The compiler will catch any attempt to use parts of a `fix` that have been 
configured out: you will see something like `gps_fix does not have member 
xxx`.