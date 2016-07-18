#!/usr/bin/env python
import sys
import os
from dotstar import Adafruit_DotStar

numpixels = 144 # Number of LEDs in strip

def clear(strip):
    for row in xrange(numpixels):
        strip.setPixelColor(row, 0)

    strip.show();

strip = Adafruit_DotStar(numpixels, order='bgr')

strip.begin()           
clear(strip)
