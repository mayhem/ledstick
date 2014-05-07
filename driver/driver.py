#!/usr/bin/env python
import sys
import os
import serial
import random
import png
import struct
from time import sleep, time
from struct import pack, unpack

BAUD_RATE = 9600

def crc16_update(crc, a):
    crc ^= a
    for i in xrange(0, 8):
        if crc & 1:
            crc = (crc >> 1) ^ 0xA001
        else:
            crc = (crc >> 1)
    return crc

class Driver(object):

    def __init__(self, device, leds, delay):
        self.ser = None
        self.device = device
        self.num_leds = leds
        self.delay = delay

    def get_num_leds(self):
        return self.num_leds

    def write(self, data):
        self.ser.write(data)
#        data = data.replace("\r", "\\r")
#        data = data.replace("\n", "\\n")
#        sys.stdout.write("w: ")
#        sys.stdout.write(data)
#        sys.stdout.flush()

    def wait_for_prompt(self, prompt = ">", wakeup="\r"):
#        sys.stdout.write("r: ")
#        sys.stdout.flush()
        while True:
            ch = self.ser.read(1)
            if not ch:
#                print "<t>"
                self.ser.flush()
                if not wakeup: 
                    return False
                self.write(wakeup)  
                continue

#            sys.stdout.write(ch)
#            sys.stdout.flush()
            if ch == prompt:
#                print
                return True

    def open(self):
        '''Open the serial connection to the router'''

        try:
            self.ser = serial.Serial(self.device, 
                                     BAUD_RATE, 
                                     bytesize=serial.EIGHTBITS, 
                                     parity=serial.PARITY_NONE, 
                                     stopbits=serial.STOPBITS_ONE,
                                     timeout=10)
        except serial.serialutil.SerialException:
            raise SerialIOError

        self.ser.flushOutput()
        self.ser.flushInput()
        sleep(3)

    def send_image(self, w, h, d, pixels):
        header = chr(0xF0) + chr(0x0F) + chr(0x0F) + chr(0xF0)
        crc = 0
        packet = struct.pack("<HHH", w, h, d) + pixels
        for ch in packet:
            crc = crc16_update(crc, ord(ch))

        packet = pack("<I", len(packet)) + packet + pack("<H", crc)
        packet = chr(0) + chr(0) + header + packet

        self.ser.flushInput()

        print "Sending packet: %d" % len(packet)
        r = self.ser.write(packet)
        print "Wrote %d bytes" % r 
        while True:
            print self.ser.outWaiting()
        while self.ser.outWaiting() > 0:
            print "sleep"
            sleep(.0001)

#            while self.ser.inWaiting():
#            sys.stdout.write(self.ser.read(1))

#        for ch in packet:
#            while True:
#                r = self.ser.write(ch)
#                if r != 1:
#                    print "sleep"
#                    sleep(.0005)
#                else:
#                    while self.ser.outWaiting() > 0:
#                    sleep(.0005)
#                    break


        print "\npacket complete. trailing data:"
        while True: #self.ser.inWaiting():
            sys.stdout.write(self.ser.read(1))

#            sent = self.ser.write(packet)
#            if sent != len(packet):
#                print "Sent %d of %d bytes" % (sent, len)
#            ack = self.ser.read(1)
#            if ack: 
#                print "image sent ok"
#                break
#            if not ack:
#                print "timeout"
#            else:
#                print "Received ack: %d" % ord(ack)

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
    print "Usage: %s <serial device>" % sys.argv[0]
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
driver = Driver(sys.argv[1], num_leds, 0)
print "open serial port"
driver.open()
print "send image"
driver.send_image(width, height, 100, pixels)
print "done"
