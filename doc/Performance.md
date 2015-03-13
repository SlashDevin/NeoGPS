Performance
===========

####**NeoGPS** is **35% faster _or more_**.

For comparison, the following sentences were parsed by various [Configurations](/doc/Configurations.md) of **NeoGPS** and **TinyGPS** on a 16MHz Arduino Mega2560.

```
$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,8,1.01,499.6,M,48.0,M,,0*5B
$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*57
```

<table>
<tr><td>Configuration</td><td>Sentence</td><td>NeoGPS</td><td>TinyGPS</td><td>Performance<br>Improvement</td></tr>
<tr><td>Minimal</td><td>GGA<br>RMC</td><td>436us<br>485us</td><td>-</td><td>70%<br>66%</td></tr>
<tr><td>DTL</td><td>GGA<br>RMC</td><td>839us<br>859us</td><td>-</td><td>42%<BR>40%</td></tr>
<tr><td>Nominal</td><td>GGA<br>RMC</td><td>913us<br>936us</td><td>1448us<br>1435us</td><td>37%<BR>35%</td></tr>
<tr><td>Full<sup>1</sup></td><td>GGA<br>RMC</td><td>1094us<br>1075us</td><td>-<br>-</td><td>25%<BR>25%</td></tr>
</table>

####Why is **NeoGPS** faster?

Most libraries use extra buffers to accumulate parts of the sentence so they 
can be parsed all at once.  For example, an extra field buffer may hold on 
to all the characters between commas.  That buffer is then parsed into a 
single data item, like `heading`.  Some libraries even hold on to the 
*entire* sentence before attempting to parse it.  In addition to increasing 
the RAM requirements, this requires **extra CPU time** to copy the bytes and 
index through them... again.
 
**NeoGPS** parses each character immediately into the data item.  When the 
delimiting comma is received, the data item has been fully computed *in 
place* and is marked as valid.

Most libraries parse all fields of their selected sentences.  Although most 
people use GPS for obtaining lat/long, some need only time, or even just one 
pulse-per-second.

**NeoGPS** configures each item separately.  Disabled items are 
conditionally compiled, which means they will not use any RAM, program space 
or CPU time.  The characters from those fields are simply skipped; they are 
never copied into a buffer or processed.

While it is significantly faster and smaller than all NMEA parsers, these same improvements also make 
NeoGPS faster and smaller than _binary_ parsers.
______________
<sup>1</sup> &nbsp;&nbsp;While "only" 25% faster than TinyGPS, the **Full** configuration of **NeoGPS** handles more messages 
and fields than TinyGPS.  It should probably be compared with TinyGPS++, which would probably yield a 
higher % improvement and significantly smaller RAM footprint.