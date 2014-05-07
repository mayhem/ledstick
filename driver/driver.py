#!/usr/bin/env python
import sys
import os
import smbus
import random
import png
import struct
from time import sleep, time
from struct import pack, unpack

DUE_ADDRESS = 45
DUE_BUS = 1

def crc16_update(crc, a):
    crc ^= a
    for i in xrange(0, 8):
        if crc & 1:
            crc = (crc >> 1) ^ 0xA001
        else:
            crc = (crc >> 1)
    return crc

class Driver(object):

    def __init__(self, leds, delay):
        self.num_leds = leds
        self.delay = delay
        self.due = None

    def get_num_leds(self):
        return self.num_leds

    def open(self):
        self.due = smbus.SMBus(DUE_BUS)

    def send_image(self, w, h, d, pixels):
        header = chr(0xF0) + chr(0x0F) + chr(0x0F) + chr(0xF0)
        crc = 0
        packet = struct.pack("<HHH", w, h, d) + pixels
        for ch in packet:
            crc = crc16_update(crc, ord(ch))

        packet = pack("<I", len(packet)) + packet + pack("<H", crc)
        packet = chr(0) + chr(0) + header + packet

        print "Sending packet: %d" % len(packet)
        for ch in packet:
            while True:
                try:
                    self.due.write_byte(DUE_ADDRESS, ord(ch))
                    break
                except IOError:
                    sleep(.001)


def read_image(image_file):
    r=png.Reader(file=open(image_file))
    data = r.read()
    x = data[0]
    y = data[1]

    pixels = ""
    for row in data[2]:
        pixels += row.tostring()

    return (x, y, pixels)

if len(sys.argv) < 1:
    sys.exit(1)

width = 20
height = 20
pixels = ""
for y in xrange(height):
    row = ""
    for x in xrange(width):
        if ((x + y) % 2 == 0):
            row += chr(255) + chr(255) + chr(0)
        else:
            row += chr(255) + chr(0) + chr(255)
    pixels += row

num_leds = 72
driver = Driver(num_leds, 0)
print "open i2c port"
driver.open()
print "send image"
driver.send_image(width, height, 100, pixels)
print "done"
