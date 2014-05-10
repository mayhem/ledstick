#!/usr/bin/env python
import sys
import os
import smbus
import random
import png
import struct
from time import sleep, time
from struct import pack, unpack

MASTER_ADDRESS = 69
DUE_ADDRESS = 45
DUE_BUS = 1

RECEIVE_OK               = 0
RECEIVE_ABORT_PACKET     = 1
RECEIVE_ABORT_PACKET_CRC = 2
RECEIVE_PACKET_COMPLETE  = 3
RECEIVE_NO_STATUS        = 4
RECEIVE_TIMEOUT          = 5

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

        while True:
            for ch in packet:
                while True:
                    try:
                        self.due.write_byte(DUE_ADDRESS, ord(ch))
                        break
                    except IOError:
                        sleep(.001)

            while True:
                try:
                    ret = self.due.read_byte(DUE_ADDRESS)
                    break
                except IOError, err:
                    sleep(.001)
            if ret == RECEIVE_PACKET_COMPLETE:
                break

def read_image(image_file):
    r=png.Reader(file=open(image_file))
    data = r.read()
    x = data[0]
    y = data[1]

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
