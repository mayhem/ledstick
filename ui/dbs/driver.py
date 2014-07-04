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
MAX_WIDTH = 100
MIN_WIDTH = 10
MAX_HEIGHT = 144
MIN_HEIGHT = 10

RECEIVE_OK               = 0
RECEIVE_ABORT_PACKET     = 1
RECEIVE_ABORT_PACKET_CRC = 2
RECEIVE_PACKET_COMPLETE  = 3
RECEIVE_NO_STATUS        = 4
RECEIVE_TIMEOUT          = 5

PACKET_OFF            =  0
PACKET_SHOW_IMAGE_0   =  1
PACKET_SHOW_IMAGE_1   =  2
PACKET_LOAD_IMAGE     =  3
PACKET_IMAGE_BLOB     =  4

MAX_BLOB_SIZE = 4096

gamma_corr_table = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11,
    11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18,
    19, 19, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28,
    29, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
    40, 41, 42, 43, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
    71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
    90, 91, 93, 94, 95, 96, 98, 99,100,102,103,104,106,107,109,110,
    111,113,114,116,117,119,120,121,123,124,126,128,129,131,132,134,
    135,137,138,140,142,143,145,146,148,150,151,153,155,157,158,160,
    162,163,165,167,169,170,172,174,176,178,179,181,183,185,187,189,
    191,193,194,196,198,200,202,204,206,208,210,212,214,216,218,220,
    222,224,227,229,231,233,235,237,239,241,244,246,248,250,252,255
]

def crc16_update(crc, a):
    crc ^= a
    for i in xrange(0, 8):
        if crc & 1:
            crc = (crc >> 1) ^ 0xA001
        else:
            crc = (crc >> 1)
    return crc

class Driver(object):

    def __init__(self, device):
        self.device = device
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
        cmd = PACKET_LOAD_IMAGE
        if not self.send_cmd(cmd):
            print "Failed to send load command"
            return False

        packet = struct.pack("<HH", w, h) + pixels
        num_blobs = (len(packet) // MAX_BLOB_SIZE) + 1
        for i in xrange(num_blobs):
            print "Send blob: [%d : %d]" % (i * MAX_BLOB_SIZE, (i+1) * MAX_BLOB_SIZE)
            if not self.send_packet(packet[i * MAX_BLOB_SIZE : (i+1) * MAX_BLOB_SIZE]):
                print "Failed to send image blob"
                return False

        return True

    def send_packet(self, packet):
        header = chr(0xF0) + chr(0x0F) + chr(0x0F) + chr(0xF0)
        crc = 0
        for ch in packet:
            crc = crc16_update(crc, ord(ch))

        print "packet len: %d %X" % (len(packet), len(packet))
        packet = pack("<H", len(packet)) + packet + pack("<H", crc)
        packet = header + packet

        self.ser.write(chr(0))
        sleep(.1)

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

    def read_image(self, image_file):
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

    def rotate_image(self, width, height, pixels):

        new_pixels = ""
        for x in xrange(width):
            line = ""
            for y in xrange(height):
                line += chr(gamma_corr_table[ord(pixels[y * width * 3 + (x * 3)])])
                line += chr(gamma_corr_table[ord(pixels[y * width * 3 + (x * 3) + 1])])
                line += chr(gamma_corr_table[ord(pixels[y * width * 3 + (x * 3) + 2])])

            new_pixels += line

        return new_pixels
