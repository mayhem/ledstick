#!/usr/bin/env python3
import sys
import os
import random
from PIL import Image
import struct
import board
from time import sleep, time
from struct import pack, unpack
from adafruit_dotstar import DotStar

numpixels = 144 # Number of LEDs in strip
secs_per_image = 20 

def clear(strip):
    strip.fill((0,0,0))
    strip.show();

def startup(strip):

    for repeat in range(2):
        for row in range(numpixels // 4):
            strip[row * 4] =  (255, 80, 0)
            strip[row * 4 + 1] = (255, 0, 255)
            strip[row * 4 + 2] = (255, 80, 0)
            strip[row * 4 + 3] = (255, 0, 255)

        strip.show();
        sleep(.2)

        for row in range(numpixels // 4):
            strip[row * 4] =  (255, 0, 255)
            strip[row * 4 + 1] = (255, 80, 0)
            strip[row * 4 + 2] = (255, 0, 255)
            strip[row * 4 + 3] = (255, 80, 0)

        strip.show();
        sleep(.2)

    clear(strip)

def load_files(filenames):

    images = []
    for filename in filenames:
        print("load %s" % filename)
        img = Image.open(filename).convert("RGB")
        pixels = img.load()
        width = img.size[0]
        height = img.size[1]
        assert height == 144
        images.append({ 'name' : filename, 'width' : width, 'height' : height, 'pixels' : pixels })

    return images


def main_loop(strip, images):
    while True:
        for image in images:
            timeout = time() + secs_per_image
            
            print("image %s" % image['name'])
            while True:
                for x in range(image['width']):
                    for y in range(image['height']):
                        color = list(image['pixels'][x,y])
                        color[1] = int(color[1] * .9)
                        strip[numpixels - y - 1] = color

                    strip.show()
                    sleep(.005)

                if time() > timeout:
                    break
                clear(strip)
                sleep(.05)


if len(sys.argv) < 2:
    print("Usage: %s: <png file> [png file] ..." % sys.argv[0])
    sys.exit(-1)


strip = DotStar(board.SCK, board.MOSI, numpixels, brightness=0.1, auto_write=False, baudrate=1200000)
startup(strip)

images = load_files(sys.argv[1:])
try:
    main_loop(strip, images)
except KeyboardInterrupt:
    clear(strip)
    sys.exit(0)
