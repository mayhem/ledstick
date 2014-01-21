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

        list.append({ 'width' : width, 'height' : height, 'pixels' : pixels })
    return list

def print_image_file(img, index):
    width = img['width']
    height = img['height']
    pixels = img['pixels']

    print "const uint16_t width_%d = %d;" % (index, width)
    print "const uint16_t height_%d = %d;" % (index, height)
    print "const prog_uchar image_%d[%d] PROGMEM = {" % (index, width * height * 3)
    for x in xrange(width):
        line = "  "
        for y in xrange(height):
            line += "%3d," % ord(pixels[y * width * 3 + (x * 3)])
            line += "%3d," % ord(pixels[y * width * 3 + (x * 3) + 1])
            line += "%3d," % ord(pixels[y * width * 3 + (x * 3) + 2])
        if y == width - 1:
            line = line[:len(line)-1]
        print line

    print "};"

def print_lookup_table(list):
    print "const int num_bitmaps = %d;" % len(list)
    print "bitmap_t bitmaps[num_bitmaps] = "
    print "{"
    for i, img in enumerate(list):
        print "   { width_%d, height_%d, 5, image_%d }," % (i, i, i)
    print "};"

if len(sys.argv) < 1:
    print "Usage: %s <png file>" % sys.argv[0]
    sys.exit(1)

list = load_files(sys.argv[1:])
for i, filename in enumerate(list):
    print_image_file(filename, i)

print_lookup_table(list)
