#!/usr/bin/env python
import sys
import os
import serial
import random
import png
import struct
from time import sleep, time
from struct import pack, unpack

if len(sys.argv) < 1:
    print "Usage: %s <png file>" % sys.argv[0]
    sys.exit(1)

r=png.Reader(file=open(sys.argv[1]))
data = r.read()
width = data[0]
height = data[1]

pixels = ""
for row in data[2]:
    pixels += row.tostring()

print "const uint16_t width = %d;" % width
print "const uint16_t height = %d;" % height
print "const prog_uchar pixels[%d] PROGMEM = {" % (width * height)
for x in xrange(width):
    line = "  "
    for y in xrange(height):
        line += "%3d," % ord(pixels[y * width + x])
    if y == width - 1:
        line = line[:len(line)-1]
    print line

print "};"

palette = r.palette()
print "const prog_char palette[%d] PROGMEM = {" % (3 * len(palette))
for i, color in enumerate(palette):
    print "   %3d, %3d, %3d" % (color[0], color[1], color[2]),
    if i != len(palette) - 1:
        print ","
    else:
        print
print "};"

