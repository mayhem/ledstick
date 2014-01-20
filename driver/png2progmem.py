#!/usr/bin/env python
import sys
import os
import serial
import random
import png
import struct
from time import sleep, time
from struct import pack, unpack

def load_files(file_list):

    list = []
    for filename in file_list:
        r=png.Reader(file=open(filename))
        data = r.read()
        width = data[0]
        height = data[1]

        pixels = ""
        for row in data[2]:
            pixels += row.tostring()

        list.append({ 'width' : width, 'height' : height, 'pixels' : pixels, 'palette' : r.palette() })
    return list

def print_image_file(img, index):
    width = img['width']
    height = img['height']
    pixels = img['pixels']
    palette = img['palette']

    print "const uint16_t width_%d = %d;" % (index, width)
    print "const uint16_t height_%d = %d;" % (index, height)
    print "const prog_uchar image_%d[%d] PROGMEM = {" % (index, width * height)
    for x in xrange(width):
        line = "  "
        for y in xrange(height):
            line += "%3d," % ord(pixels[y * width + x])
        if y == width - 1:
            line = line[:len(line)-1]
        print line

    print "};"

    print "const prog_uchar palette_%d[%d] PROGMEM = {" % (index, 3 * len(palette))
    for i, color in enumerate(palette):
        print "   %3d, %3d, %3d" % (color[0], color[1], color[2]),
        if i != len(palette) - 1:
            print ","
        else:
            print
    print "};"

def print_lookup_table(list):
    print "const int num_bitmaps = %d;" % len(list)
    print "bitmap_t bitmaps[num_bitmaps] = "
    print "{"
    for i, img in enumerate(list):
        print "   { width_%d, height_%d, 5, image_%d, palette_%d }," % (i, i, i, i)
    print "};"

if len(sys.argv) < 1:
    print "Usage: %s <png file>" % sys.argv[0]
    sys.exit(1)

list = load_files(sys.argv[1:])
for i, filename in enumerate(list):
    print_image_file(filename, i)

print_lookup_table(list)
