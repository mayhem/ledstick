#!/usr/bin/env python
import sys
import os
import serial
import random
import png
import struct
from time import sleep, time
from struct import pack, unpack

BAUD_RATE = 115200

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

    def __init__(self, device, delay):
        self.device = device
        self.delay = delay
        self.ser = None

    def open(self):
        '''Open the serial connection to the router'''
        try:
            self.ser = serial.Serial(self.device, 
                                     BAUD_RATE, 
                                     bytesize=serial.EIGHTBITS, 
                                     parity=serial.PARITY_NONE, 
                                     stopbits=serial.STOPBITS_ONE,
                                     timeout=.1)
        except serial.serialutil.SerialException:
            raise SerialIOError

        self.ser.flushOutput()
        self.ser.flushInput()

        # A connection to the due causes the Due to reboot. Thus we wait.
        sleep(3)

    def send_image(self, w, h, d, pixels):
        header = chr(0xF0) + chr(0x0F) + chr(0x0F) + chr(0xF0)
        crc = 0
        packet = struct.pack("<HHH", w, h, d) + pixels
        for ch in packet:
            crc = crc16_update(crc, ord(ch))

        print "packet len: %d %X" % (len(packet), len(packet))
        packet = pack("<I", len(packet)) + packet + pack("<H", crc)
        packet = chr(0) + chr(0) + header + packet

        while True:
            for i, ch in enumerate(packet):
                while True:
                    try:
                        print "%d: %X" % (i, ord(ch))
                        num = self.ser.write(ord(ch))
                        if num == 1:
                            break
                        print "write fail, retry"
                    except SerialTimeoutException:
                        print "write timeout"
                        sleep(.001)

            while True:
                try:
                    ret = self.ser.read()
                    if num == 1:
                        break
                    print "read fail, retry"
                except SerialTimeoutException:
                    print "read timeout"
                    sleep(.001)

            if ret == RECEIVE_PACKET_COMPLETE:
                break

            print "Received char %X. WTF?" % ch

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

def make_test_image():
    width = 100 
    height = 144
    pixels = ""
    for y in xrange(height):
        row = ""
        for x in xrange(width):
            if x % 2 == 0:
                row += chr(255) + chr(0) + chr(0)
            else:
                row += chr(0) + chr(0) + chr(255)
        pixels += row

    return (width, height, pixels)

def rotate_image(width, height, pixels):

    new_pixels = ""
    for x in xrange(width):
        line = ""
        for y in xrange(height):
            line += pixels[y * width * 3 + (x * 3)]
            line += pixels[y * width * 3 + (x * 3) + 1]
            line += pixels[y * width * 3 + (x * 3) + 2]

        new_pixels += line

    return new_pixels

def dump_image(width, height, pixels):
    t_pixels = ""
    for x in xrange(width):
        txt = ""
        for y in xrange(height):
            txt += "%03d " % ord(pixels[y * width * 3 + (x * 3)]) 
            txt += "%03d " % ord(pixels[y * width * 3 + (x * 3) + 1]) 
            txt += "%03d " % ord(pixels[y * width * 3 + (x * 3) + 2]) 

        t_pixels += txt + "\n"
    print t_pixels

if len(sys.argv) < 2:
    print "Usage: driver.py <png file>"
    sys.exit(1)

width, height, pixels = read_image(sys.argv[1]);
#width, height, pixels = make_test_image()

#pixels = rotate_image(width, height, pixels)

driver = Driver("/dev/ttyACM0", 0)
print "open port"
driver.open()
print "send image"
driver.send_image(width, height, 100, pixels)
print "done"
