#!/usr/bin/env python3
import sys
import os
import random
import png
import struct
import board
from time import sleep, time
from struct import pack, unpack
from adafruit_dotstar import DotStar

leds = 144 # Number of LEDs in strip

def checkerboard(strip):
    for i in range(2):
        for k in range(leds):
            if k // 4 % 2 == (i % 2):
                color = (255, 0, 0)
            else:
                color = (255, 0, 255)
            strip[k] = color
        strip.show()
    

strip = DotStar(board.SCK, board.MOSI, leds, brightness=0.2, auto_write=False, baudrate=1200000)
strip.fill((255,0,0))
strip.show()

try:
    while True:
        checkerboard(strip)
except KeyboardInterrupt:
    strip.fill((0,0,0))
    strip.show()
