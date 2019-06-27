#!/usr/bin/env python

import unittest


class Crc(object):
    '''
    Normal python crc32 libs don't appear to work with s10's algorithm
    I found the bootrom CRC algorithm here:
    //acds/main/ip/sopc/app/com.altera.nadderdump/src/com/altera/nadderdump/CRC.java
    '''

    POLYNOMIAL = 0x04C11DB7;
    INITIAL_VALUE = 0xffffffff;
    FINAL_XOR = 0xffffffff;
    TABLE = None

    @staticmethod
    def make_table():
        table = {}

        for dividend in range(0,256):
            # Start with the dividend followed by zeros.
            remainder = (dividend << 24) & 0xffffffff

            # Perform modulo-2 division, a bit at a time.
            for _ in range(0,8):
                # Try to divide the current data bit.
                if (remainder & 0x80000000) != 0:
                    remainder = ((remainder << 1) ^ Crc.POLYNOMIAL) & 0xffffffff
                else:
                    remainder = (remainder << 1) & 0xffffffff

            table[dividend] = remainder;
        return table

    def __init__(self):
        if Crc.TABLE is None:
            Crc.TABLE = Crc.make_table()

    def generate(self, data):
        start = 0
        num = len(data)

        reg = Crc.INITIAL_VALUE

        for i in range(start,start+num):
            reg = ((reg << 8) ^ Crc.TABLE[((reg >> 24) ^ data[i]) & 0xff]) & 0xffffffff

        return (reg ^ Crc.FINAL_XOR)


class CrcTest(unittest.TestCase):

    def testCrc(self):
        crc = Crc()
        self.assertEquals(crc.generate(bytearray()), 0)
        self.assertEquals(crc.generate(bytearray([0])), 2985771083)
        self.assertEquals(crc.generate(bytearray([0,0])), 4282948482)
        self.assertEquals(crc.generate(bytearray([0,0,0])), 1218151167)
        self.assertEquals(crc.generate(bytearray([0,0,0,0])), 955982468)
        self.assertEquals(crc.generate(bytearray([0,1,2,3,4])), 115125973)

if __name__ == '__main__':
    unittest.main()
