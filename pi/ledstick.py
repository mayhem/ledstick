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

numpixels = 144 # Number of LEDs in strip

def load_file(filename):

    r=png.Reader(file=open(filename))
    data = r.read()
    width = data[0]
    height = data[1]

    pixels = ""
    if data[3]['alpha']:
        for row in data[2]:
            for i in xrange(width):
                pixels += chr(row[i * 4])
                pixels += chr(row[i * 4 + 1])
                pixels += chr(row[i * 4 + 2])
    else:
        for row in data[2]:
            pixels += row.tostring()

    return { 'width' : width, 'height' : height, 'pixels' : pixels }

if len(sys.argv) < 2:
    print "Usage: %s: <png file>" % sysargv[0]
    sys.exit(-1)

image = load_file(sys.argv[1])

strip   = Adafruit_DotStar(numpixels, order='bgr')

strip.begin()           # Initialize pins for output
strip.setBrightness(64) # Limit brightness to ~1/4 duty cycle

while True:
    for col in xrange(image['width']):
        for row in xrange(image['height']):
            color = ord(image['pixels'][(row * 3 * image['width']) + (col * 3)]) << 16 |     \
                    ord(image['pixels'][(row * 3 * image['width']) + (col * 3) + 1]) << 8 | \
                    ord(image['pixels'][(row * 3 * image['width']) + (col * 3) + 2])
            strip.setPixelColor(numpixels - row, color)

	strip.show()
	sleep(.0002)

    sleep(.05)
