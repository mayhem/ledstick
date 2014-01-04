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
                                     timeout=1)
        except serial.serialutil.SerialException:
            raise SerialIOError

        sleep(4)

    def send_image(self, w, h, d, pixels):
        print "write image: %d" % len(pixels)
        self.ser.write(struct.pack("<BBH", w, h, d))
        for i, p in enumerate(pixels):
            print "%d" % i
            self.ser.write(p)
            sleep(.01)

r=png.Reader(file=open('tv-test-pattern-tiny.png'))
data = r.read()
x = data[0]
y = data[1]
bits = data[2]

pixels = ""
for row in data[2]:
    pixels += row.tostring()

num_leds = 72
driver = Driver("/dev/tty.usbmodemfd121", num_leds, 0)
driver.open()
driver.send_image(x, y, 100, pixels)
