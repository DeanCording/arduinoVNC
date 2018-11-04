VNC Client for Arduino/ESP8266
===========================================

A VNC Client for Arduino/ESP8266 based on rfbproto.

Video:

[![VNC Client on ESP8266 + ILI9341](https://img.youtube.com/vi/MA47npOtApc/0.jpg)](https://www.youtube.com/watch?v=MA47npOtApc)
[![VNC Client on ESP8266 + ILI9341 + Touch](https://img.youtube.com/vi/8Ix-HomwQvw/0.jpg)](https://www.youtube.com/watch?v=8Ix-HomwQvw)

##### Supported features #####
 - Bell
 - CutText (clipboard)
 
##### Supported encodings #####
 - RAW
 - RRE
 - CORRE
 - HEXTILE
 - COPYRECT
  
##### Not supported encodings #####
 - TIGHT
 - ZLIB
    
##### Supported Hardware #####
 - ESP8266 
 
 may run on Arduino DUE too.

##### Supported Displays #####
All displays supported by [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) library.

 - ILI9341
 - ILI9163
 - ST7735
 - S6D02A1
 - ILI9481
 - ILI9486
 - ILI9488
 - HX8357D
 - ST7789
 
 Configuration of TFT driver and connections is in libraries/TFT_eSPI/User_Setup.h
 
#### Supported Touch Controllers #####
 - XPT2046
 
 
#### Configuration
Script uses [WiFiManager](https://github.com/tzapu/WiFiManager) to configure Wifi and VNC server connection
details.  If the ESP8266 cannot connect to a Wifi network, it will bring up and Wifi access point with a 
configuration web portal.

Pressing the _Flash_ (Pin 0) during operation will trigger the Wifi/VNC config portal.

Touching the touch screen during boot up will trigger calibration of the touch screen.

 
 
### Issues ###
Submit issues to: https://github.com/DeanCording/arduinoVNC/issues

### License and credits ###

The library is licensed under [GPLv2](https://github.com/DeanCording/arduinoVNC/blob/master/LICENSE)

D3DES by Richard Outerbridge (public domain)

VNC code base (GPLv2)
Thanks to all that worked on the original VNC implementation
```
Copyright (C) 2009-2010, 2012 D. R. Commander. All Rights Reserved.
Copyright (C) 2004-2008 Sun Microsystems, Inc. All Rights Reserved.
Copyright (C) 2004 Landmark Graphics Corporation. All Rights Reserved.
Copyright (C) 2000-2006 Constantin Kaplinsky. All Rights Reserved.
Copyright (C) 2000 Tridia Corporation. All Rights Reserved.
Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
```


