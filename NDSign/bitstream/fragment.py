#!/usr/bin/env python

import hashlib
import os
import unittest

from bitstream import IBitstream
from util.convert import Convert
from util.crc import Crc
from util.fileutil import FileUtils
from util.mkdir import Mkdir


class BitstreamFragment(IBitstream):

    def __init__(self, byte_array=None, max_size=None):
        super(BitstreamFragment,self).__init__()
        self.max_size = max_size
        self.crc_addresses = []
        self.expected_magic = []
        self.description = None
        BitstreamFragment.initialize(self, byte_array=byte_array)

    def initialize(self, byte_array=None, size=0):
        if byte_array is None:
            self.__raw = bytearray([0]*size)
        elif size != 0:
                raise ValueError('size attribute to initialize method if only valid if byte_array not specified')
        else:
            self.__raw = byte_array

        if self.max_size is not None:
            assert(self.size() <= self.max_size)

        self.update_magic()
        return self

    def size(self):
        return len(self.__raw)

    def append(self, byte_array=None, size=0):
        if byte_array is None:
            if size <= 0:
                raise ValueError('size attribute to append method is <= 0')
            else:
                self.__raw += bytearray([0]*size)
        elif size != 0:
                raise ValueError('size attribute to initialize method if only valid if byte_array not specified')
        else:
            self.__raw += byte_array

        if self.max_size is not None:
            assert(self.size() <= self.max_size)

    def validate(self):
        if self.max_size is not None and self.size() > self.max_size:
            raise ValueError('IBitstream block size exceeds maximum')
        self.validate_magic()
        self.validate_crc()
        return True

    def update(self):
        self.update_crc()
        return True

    def get_raw_byte_array(self):
        return self.__raw

    def get_raw_value(self, offset, size, endianness='little'):
        assert(offset+size <= self.size())
        raw_byte_array = self.get_raw_byte_array()

        if endianness == 'big':
            return raw_byte_array[offset:offset+size][::-1]
        else:
            return raw_byte_array[offset:offset+size]

    def get_value(self, offset, size=4, endianness='little'):
        return Convert().bytearray_to_integer(self.get_raw_value(offset=offset, size=size, endianness=endianness))

    def set_raw_value(self, byte_array, offset, endianness='little'):
        sz = len(byte_array)
        assert(offset+sz <= self.size())

        if endianness == 'big':
            self.__raw[offset:offset+sz] = byte_array[0:sz][::-1]
        else:
            self.__raw[offset:offset+sz] = byte_array[0:sz]

    def set_value(self, value, offset, size=4, endianness='little'):
        ba = Convert().integer_to_bytes(n=value,length=size)
        assert(len(ba) == size)
        self.set_raw_value(byte_array=ba, offset=offset, endianness=endianness)

    def read(self, fp):
        if fp.closed: return

        if self.max_size is None:
            self.append(byte_array=bytearray(fp.read()))
            fp.close()
        else:
            read_size = self.max_size - self.size()
            if read_size > 0:
                self.append(bytearray(fp.read(read_size)))
                if self.size() != self.max_size:
                    fp.close()

        self.validate()

    def write(self, fp):
        self.validate()
        fp.write(self.get_raw_byte_array())

    def save(self, filename):
        self.validate()
        filepathdir = os.path.dirname(filename)
        Mkdir().mkdir_p(filepathdir)
        with FileUtils().write_binary_file_open(filename) as fp:
            self.write(fp)
        FileUtils().close_binary_file(fp, filename)

    def load(self, filename):
        with FileUtils().read_binary_file_open(filename=filename) as fp:
            self.read(fp)
        FileUtils().close_binary_file(fp, filename)
        self.validate()
        return self

    def crc(self, crc_addr=None, calculate=False):
        assert(len(self.crc_addresses) > 0)

        if crc_addr is None:
            crc_addr = sorted(self.crc_addresses)[-1]

        if crc_addr not in self.crc_addresses:
            raise ValueError('Provided crc address of ' + str(hex(crc_addr)) + ' is not valid')
        if not calculate:
            return self.get_value(crc_addr)
        return Crc().generate(self.get_raw_value(offset=0x0,size=crc_addr))

    def magic_number(self):
        if len(self.expected_magic) == 0:
            raise ValueError('Bitstream fragment does not have any magic numbers')
        return self.get_value(self.expected_magic[0][1], size=4)

    def update_magic(self):
        for magic in self.expected_magic:
            self.set_value(value=magic[0], offset=magic[1], size=4)

    def validate_magic(self):
        for magic in self.expected_magic:
            if self.get_value(offset=magic[1], size=4) != magic[0]:
                raise ValueError('Magic Number at offset ' + hex(magic[1]) + ' is not correct! ' + \
                                 'Expected: ' + hex(magic[0]) + ', ' + \
                                 'Actual: ' + hex(self.get_value(offset=magic[1], size=4)))

    def update_crc(self):
        for crc_addr in sorted(self.crc_addresses):
            self.set_value(value=self.crc(crc_addr=crc_addr,calculate=True), offset=crc_addr, size=4)
        return self.validate_crc()

    def validate_crc(self):
        for crc_addr in self.crc_addresses:
            if self.crc(crc_addr=crc_addr) != self.crc(calculate=True, crc_addr=crc_addr):
                raise ValueError('Block is not valid - crc at offset ' + str(crc_addr) + ' is not correct! ' + \
                                 'Expected: ' + str(self.crc(calculate=True, crc_addr=crc_addr)) + ', ' + \
                                 'Actual: ' + str(self.crc(crc_addr=crc_addr)))
        return True

    def sha256sum(self):
        return Convert().bytearray_to_hex_string(self.sha256(), endianness='big').lower().lstrip("0x").zfill(64)

    def sha256(self, offset=0, size=None):
        if size is None:
            size = self.size() - offset
        return bytearray(hashlib.sha256(self.get_raw_value(offset=offset, size=size)).digest())

    def sha384(self, offset=0, size=None):
        if size is None:
            size = self.size() - offset
        return bytearray(hashlib.sha384(self.get_raw_value(offset=offset, size=size)).digest())

    def fragment_properties_str(self):
        return_string = ""
        if self.description is not None:
            return_string += "  [Description: " + self.description + "]\n"
        return return_string

    def __str__(self):
        return_string = " " + self.__class__.__name__ + " [Size=" + str(self.size()) + "] " + "\n"
        return_string += self.fragment_properties_str()

        max_bytes_to_print = 4096
        count_bytes = 0

        for i in range(0,self.size()//4):
            count_bytes += 4
            return_string += "   0x" + hex(i*4)[2:].zfill(4).rstrip('L') + ":"

            if count_bytes > max_bytes_to_print:
                return_string += "   ..." + "\n\n"
                break

            return_string += "   0x" + hex(self.get_value(offset=i*4, size=4))[2:].zfill(8).rstrip('L') + "\n"

        return return_string

class BitstreamFragmentTest(unittest.TestCase):

    def test_constructor(self):
        ba = bytearray()
        b = BitstreamFragment(ba)
        self.assertNotEquals(b, None)
        self.assertEquals(b.validate(), True)
        self.assertEquals(b.size(), 0)

        ba.append(0x0)
        self.assertEquals(b.size(), 1)

        ba += bytearray.fromhex('ABCD')
        self.assertEquals(b.size(), 3)
        self.assertEquals(b.get_raw_value(1,1)[0], 0xAB)
        self.assertEquals(b.get_raw_value(2,1)[0], 0xCD)

        ba[2] = 0x0
        self.assertEquals(b.size(), 3)
        self.assertEquals(b.get_raw_value(1,1)[0], 0xAB)
        self.assertEquals(b.get_raw_value(2,1)[0], 0x00)

    def test_get_set(self):
        ba0 = bytearray([0]*8096)
        b = BitstreamFragment(ba0)
        self.assertNotEquals(b, None)

        self.assertEquals(b.get_value(0, 1), 0)
        self.assertEquals(b.get_value(0, 4), 0)

        ba1 = bytearray([1]*16)
        b.set_raw_value(ba1, 0x1)

        self.assertEquals(b.get_value(0, 1), 0)
        self.assertEquals(b.get_value(1, 1), 1)
        self.assertEquals(b.get_value(1, 2), 257)
        self.assertEquals(b.get_value(17, 1), 0)

        b.set_value(0,0x1,16)
        self.assertEquals(b.get_value(1, 1), 0)
        self.assertEquals(b.get_value(16, 1), 0)

    def test_crc(self):
        ba0 = bytearray([0]*8096)
        b = BitstreamFragment(ba0)
        b.update_crc()
        b.crc_addresses.append(4096)
        self.assertEqual(b.get_value(4096), 0)
        b.update_crc()
        self.assertEqual(b.get_value(4096), 2281715939)
        b.validate()

        self.assertEqual(b.sha256sum(),"0caad27a797719d71dfe1164a69fed898bc334e449462d7e57a5f7ecc0fbe07a")

    def test_big_endian(self):
        b = BitstreamFragment(bytearray([0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF]))
        self.assertNotEqual(b, None)

        self.assertEquals(b.get_value(offset=0x0, endianness='little'),0x33221100)
        self.assertEquals(b.get_value(offset=0x0, endianness='big'),0x00112233)
        self.assertEquals(b.get_value(offset=0x5, endianness='little'),0x88776655)
        self.assertEquals(b.get_value(offset=0x5, endianness='big'),0x55667788)

if __name__ == '__main__':
    unittest.main()

