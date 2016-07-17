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

def clear(strip):
    for row in xrange(numpixels):
        strip.setPixelColor(row, 0)

    strip.show();

def startup(strip):

    for repeat in xrange(5):
        for row in xrange(numpixels / 4):
            strip.setPixelColor(row * 4, 0xFFAA00)
            strip.setPixelColor(row * 4 + 1, 0xFFAA00)
            strip.setPixelColor(row * 4 + 2, 0xFF00FF)
            strip.setPixelColor(row * 4 + 3, 0xFF00FF)

        strip.show();
        sleep(.1)

        for row in xrange(numpixels / 4):
            strip.setPixelColor(row * 4, 0xFF00FF)
            strip.setPixelColor(row * 4 + 1, 0xFF00FF)
            strip.setPixelColor(row * 4 + 2, 0xFFAA00)
            strip.setPixelColor(row * 4 + 3, 0xFFAA00)

        strip.show();
        sleep(.1)

    clear(strip)

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


strip   = Adafruit_DotStar(numpixels, order='bgr')

strip.begin()           # Initialize pins for output
strip.setBrightness(128) # Limit brightness to ~1/4 duty cycle

startup(strip)

image = load_file(sys.argv[1])

while True:
    for col in xrange(image['width']):
        for row in xrange(image['height']):
            red = ord(image['pixels'][(row * 3 * image['width']) + (col * 3)])
            green = ord(image['pixels'][(row * 3 * image['width']) + (col * 3) + 1])
            blue = ord(image['pixels'][(row * 3 * image['width']) + (col * 3) + 2])

            blue = int(blue * .9)

            color = (red << 16 | green << 8 | blue)
            strip.setPixelColor(numpixels - row, color)

	strip.show()
	sleep(.0002)

    clear(strip)
    sleep(.05)
