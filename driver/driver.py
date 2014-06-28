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
TIMEOUT   = .1

RECEIVE_OK               = 0
RECEIVE_ABORT_PACKET     = 1
RECEIVE_ABORT_PACKET_CRC = 2
RECEIVE_PACKET_COMPLETE  = 3
RECEIVE_NO_STATUS        = 4
RECEIVE_TIMEOUT          = 5

PACKET_OFF            =  0
PACKET_SHOW_IMAGE_0   =  1
PACKET_SHOW_IMAGE_1   =  2
PACKET_LOAD_IMAGE_0   =  3
PACKET_LOAD_IMAGE_1   =  4
PACKET_IMAGE_BLOB     =  5

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
                                     timeout=TIMEOUT)
        except serial.serialutil.SerialException:
            raise IOError("Cannot open serial port %s" % self.device)

        self.ser.flushOutput()
        self.ser.flushInput()

        # A connection to the due causes the Due to reboot. Thus we wait.
        sleep(3)

    def set_timeout(self, timeout = TIMEOUT):
        self.ser.timeout = timeout

    def read_char(self):
        while True:
            try:
                ret = self.ser.read()
                if num == 1:
                    return ret
                print "read fail, retry"
            except serial.SerialTimeoutException:
                print "read timeout"
                sleep(.001)

    def console(self):
        self.set_timeout(0)
        ret = self.ser.read()
        self.set_timeout()
        if len(ret) > 0:
           print ret, 

    def send_cmd(self, cmd):
        packet = struct.pack("<c", chr(cmd))
        return self.send_packet(packet)

    def send_image(self, w, h, pixels):
        if not self.send_cmd(PACKET_LOAD_IMAGE_1):
            print "Failed to send load command"
            return False

        max_blob_size = 10240
        packet = struct.pack("<HH", w, h) + pixels
        num_blobs = (len(packet) // max_blob_size) + 1
        for i in xrange(num_blobs):
            print "Send blob: [%d : %d]" % (i * max_blob_size, (i+1) * max_blob_size)
            if not self.send_packet(packet[i * max_blob_size : (i+1) * max_blob_size]):
                print "Failed to send image blob"
                return False

    def send_packet(self, packet):
        header = chr(0xF0) + chr(0x0F) + chr(0x0F) + chr(0xF0)
        crc = 0
        for ch in packet:
            crc = crc16_update(crc, ord(ch))

        print "packet len: %d %X" % (len(packet), len(packet))
        packet = pack("<H", len(packet)) + packet + pack("<H", crc)
        packet = chr(0) + chr(0) + header + packet

        for i, ch in enumerate(packet):
            while True:
                try:
                    num = self.ser.write(ch)
                    if num == 1:
                        break
                    print "write fail, retry"
                except serial.SerialTimeoutException:
                    print "write timeout"
                    sleep(.001)

        self.ser.timeout = 1
        ch = self.ser.read(1)
        if ch and ord(ch) == RECEIVE_PACKET_COMPLETE:
            print "Packet sent ok."
            return True
        if not ch:
            print "No response!"
            return False
        else:
            print "Packet response: %d" % ord(ch)
            return False

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

def dump_image(pixels):
    t_pixels = ""
    for i, p in enumerate(pixels):
        if i % 8 == 0:
            print "\n%08X " % i,
        print "%02X " % ord(p),
    print
    print "%d bytes" % len(pixels)

if len(sys.argv) < 2:
    print "Usage: driver.py <png file>"
    sys.exit(1)

width, height, pixels = read_image(sys.argv[1]);
#width, height, pixels = make_test_image()

pixels = rotate_image(width, height, pixels)
dump_image(struct.pack("<HH", width, height) + pixels)

driver = Driver("/dev/ttyAMA0", 0)
print "open port"
driver.open()
print "send image"
driver.send_image(width, height, pixels)
print "done"
