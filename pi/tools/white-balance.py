#!/usr/bin/env python
import sys
import os
import serial
import random
import png
import struct
from time import sleep, time
from struct import pack, unpack
from dotstar import Adafruit_DotStar

numpixels = 144
gamma_value = 1.1

gamma = bytearray(256)
for i in range(256):
    gamma[i] = int(pow(float(i) / 255.0, gamma_value) * 255.0 + 0.5)

strip   = Adafruit_DotStar(numpixels, order='bgr')

strip.begin()           # Initialize pins for output
strip.setBrightness(128) # Limit brightness to ~1/4 duty cycle

for row in xrange(144):
    strip.setPixelColor(row, 0)
strip.show()

# FF FF F0

for row in xrange(50):
    strip.setPixelColor(row, 0xaaaaa0)
strip.show()
