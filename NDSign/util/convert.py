#!/usr/bin/env python

import binascii
import codecs
import re
import struct
import unittest


class Convert(object):
    '''
    format conversion utility methods
      - little endian byte order is always assumed. ToDo: test big endian stuff if it is ever needed
    '''

    def bytearray_to_hex_string(self, byte_array, offset=0, num_of_bytes=None, endianness='little'):

        if num_of_bytes is None:
            num_of_bytes = len(byte_array) - offset

        hex_string_list = []
        ba = byte_array[offset:offset+num_of_bytes]
        if endianness != 'big': ba = ba[::-1]

        for byte in ba:
            hex_string_list.append( "%02X" % byte )

        hex_string = "0x" + ''.join( hex_string_list ).strip()

        # remove leading zeros
        hex_string = re.sub(r'0x[0]*','0x',hex_string)

        if hex_string == "0x":
            hex_string = "0x0"

        return hex_string

    def bytearray_to_integer(self, byte_array, offset=0, num_of_bytes=None, endianness='little'):

        if len(byte_array) == 0:
            return 0

        if num_of_bytes is None:
            num_of_bytes = len(byte_array) - offset

        byte_array = byte_array[offset:offset+num_of_bytes]
        if endianness != 'big': byte_array = byte_array[::-1]

        return int(binascii.hexlify(byte_array), base=16)

    def integer_to_bytes(self, n, length, endianness='little'):
        h = '%x' % n
        s = codecs.decode(('0'*(len(h) % 2) + h).zfill(length*2),'hex')
        ba = bytearray(s)
        if endianness != 'big':
            ba = ba[::-1]
        if len(ba) > length:
            ba = ba[0:length]
        return ba

    def swap32(self,i):
        return struct.unpack("<I", struct.pack(">I", i))[0]

    def hex_string_to_bytes(self, hex_string, num_of_bytes=None, endianness='little'):
        if (hex_string[0:2] == '0x'):
            hex_string = hex_string[2:]

        hex_string = '0'*(len(hex_string)%2) + hex_string
        num_hex_bytes = len(hex_string)//2
        if num_of_bytes is None:
            num_of_bytes = num_hex_bytes
        if (num_of_bytes < num_hex_bytes):
            raise BufferError("Cannot fit hex string into specified num_of_bytes")

        hex_string = '00'*(num_of_bytes-num_hex_bytes) + hex_string

        ret = bytearray(num_of_bytes)
        for i in range(0,len(hex_string),2):
            val = int(hex_string[i:i+2],16)
            offset = i//2
            if endianness != 'big':
                offset = -(1+offset) # Move backwards instead of forwards
            ret[offset] = val

        return ret


class ConvertTest(unittest.TestCase):

    def test_bytearray_to_hex_string(self):
        cnv = Convert()
        self.assertEquals(cnv.bytearray_to_hex_string(bytearray()), "0x0")
        self.assertEquals(cnv.bytearray_to_hex_string(bytearray([0x0])), "0x0")
        self.assertEquals(cnv.bytearray_to_hex_string(bytearray([0x0,0x0])), "0x0")
        self.assertEquals(cnv.bytearray_to_hex_string(bytearray([0x0,0x0,0x0,0x0,0x0])), "0x0")
        self.assertEquals(cnv.bytearray_to_hex_string(bytearray([0x0,0x1,0x2,0x3,0x4]),0,1), "0x0")
        self.assertNotEquals(cnv.bytearray_to_hex_string(bytearray([0x1,0x0,0x0,0x0,0x4]),0,1), "0x0")
        self.assertEquals(cnv.bytearray_to_hex_string(bytearray([0x1,0x0,0x0,0x0,0x4]),1,3), "0x0")
        self.assertEquals(cnv.bytearray_to_hex_string(bytearray([0xA,0xB,0xC,0xD])), "0xD0C0B0A")
        self.assertEquals(cnv.bytearray_to_hex_string(bytearray([0xAB])), "0xAB")
        self.assertEquals(cnv.bytearray_to_hex_string(bytearray([0xAB,0xCD])), "0xCDAB")

    def test_bytearray_to_integer(self):
        cnv = Convert()
        self.assertEquals(cnv.bytearray_to_integer(bytearray()), 0x0)
        self.assertEquals(cnv.bytearray_to_integer(bytearray([0x0])), 0x0)
        self.assertEquals(cnv.bytearray_to_integer(bytearray([0x0,0x0])), 0x0)
        self.assertEquals(cnv.bytearray_to_integer(bytearray([0x0,0x0,0x0,0x0,0x0])), 0x0)
        self.assertEquals(cnv.bytearray_to_integer(bytearray([0x0,0x1,0x2,0x3,0x4]),0,1), 0x0)
        self.assertNotEquals(cnv.bytearray_to_integer(bytearray([0x1,0x0,0x0,0x0,0x4]),0,1), 0x0)
        self.assertEquals(cnv.bytearray_to_integer(bytearray([0x1,0x0,0x0,0x0,0x4]),1,3), 0x0)
        self.assertEquals(cnv.bytearray_to_integer(bytearray([0xA,0xB,0xC,0xD])), 0xD0C0B0A)
        self.assertEquals(cnv.bytearray_to_integer(bytearray([0xAB])), 0xAB)
        self.assertEquals(cnv.bytearray_to_integer(bytearray([0xAB,0xCD])), 0xCDAB)

    def test_integer_to_bytes(self):
        cnv = Convert()
        self.assertEquals(len(cnv.integer_to_bytes(0x01020304,0)),0)
        self.assertEquals(len(cnv.integer_to_bytes(0x01020304,1)),1)
        self.assertEquals(len(cnv.integer_to_bytes(0x01020304,4)),4)
        self.assertEquals(len(cnv.integer_to_bytes(0x01020304,100)),100)
        self.assertEquals(cnv.bytearray_to_hex_string(cnv.integer_to_bytes(0x01020304,5)),"0x1020304")
        self.assertEquals(cnv.bytearray_to_hex_string(cnv.integer_to_bytes(0x01020304,2)),"0x304")
        self.assertEquals(cnv.bytearray_to_hex_string(cnv.integer_to_bytes(0x01020304,1)),"0x4")
        self.assertEquals(cnv.bytearray_to_hex_string(cnv.integer_to_bytes(0xFFFFFFFF,4)),"0xFFFFFFFF")
        self.assertEquals(cnv.bytearray_to_hex_string(cnv.integer_to_bytes(0xFFFFFFF0,2)),"0xFFF0")

        self.assertEquals(hex(cnv.integer_to_bytes(0x62294895,4)[0]),'0x95')
        self.assertEquals(hex(cnv.integer_to_bytes(0x62294895,4)[3]),'0x62')

    def test_swap32(self):
        cnv = Convert()
        self.assertEquals(cnv.swap32(0x0),0x0)
        self.assertEquals(cnv.swap32(0x1234),0x34120000)
        self.assertEquals(cnv.swap32(0x12345678),0x78563412)
        self.assertEquals(cnv.swap32(0xABCDEF00),0x00EFCDAB)
        self.assertEquals(cnv.swap32(0xFFFFFFFF),0xFFFFFFFF)

    def test_hex_string_to_bytes(self):
        cnv = Convert()
        self.assertRaises(BufferError, cnv.hex_string_to_bytes, '0x01020304', 0)
        self.assertRaises(BufferError, cnv.hex_string_to_bytes, '0x01020304', 3)
        self.assertEquals(cnv.hex_string_to_bytes('0x01020304', 4), bytearray([4,3,2,1]))
        self.assertEquals(cnv.hex_string_to_bytes('0x010203', 4), bytearray([3,2,1,0]))
        self.assertEquals(cnv.hex_string_to_bytes('0x00010203', 4), bytearray([3,2,1,0]))
        self.assertEquals(cnv.hex_string_to_bytes('01020304', 4, 'big'), bytearray([1,2,3,4]))
        self.assertEquals(cnv.hex_string_to_bytes('0x000102030405060708090a0b0c0d0e0f'), bytearray(reversed(range(0,16))))
        self.assertEquals(cnv.hex_string_to_bytes('0x000102030405060708090a0b0c0d0e0f', 26), bytearray(reversed(range(0,16))) + bytearray(10))
        self.assertEquals(cnv.hex_string_to_bytes('0x000102030405060708090a0b0c0d0e0f', 26, 'big'), bytearray(10) + bytearray(range(0,16)))
        self.assertEquals(cnv.hex_string_to_bytes(cnv.bytearray_to_hex_string(bytearray([0xAB, 0xCD]))), bytearray([0xAB,0xCD]))
        self.assertEquals(cnv.hex_string_to_bytes(cnv.bytearray_to_hex_string(bytearray([0xA, 0xC]))), bytearray([0xA,0xC]))

if __name__ == '__main__':
    unittest.main()
