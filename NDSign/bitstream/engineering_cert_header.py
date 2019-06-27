#!/usr/bin/env python

import unittest

from fragment import BitstreamFragment
from magic import MagicNumber
from util.convert import Convert
import re

class EngineeringCertHeader(BitstreamFragment):
    ENG_CERT_SIZE=8192

    def __init__(self, byte_array=None):
        super(EngineeringCertHeader,self).__init__(byte_array=byte_array,max_size=4096)
        self.crc_addresses.append(0xFFC)
        self.expected_magic.append( (MagicNumber.CERTIFICATE_ENGINEERING,0x0) )
        self.expected_magic.append( (MagicNumber.CERT_SECTION_DESCRIPTOR,0xC) )

    # uid, hmac, pub_key_hash are all hex strings (strings of the format '^0x[0-9a-fA-F]+$')
    def initialize(self, uid, hmac, pub_key_hash, temp_pub_version=0):
        super(EngineeringCertHeader,self).initialize(size=self.max_size)
        assert(self.size() == 4096)
        self.validate_hex_strings([uid, hmac, pub_key_hash])

        self.set_value(offset=0x00, value=MagicNumber.CERTIFICATE_ENGINEERING)
        self.set_value(offset=0x04, value=temp_pub_version) # For now, allow this to be overridden (See comments in gen_cert() in flows/CreateEngCertCommand.py)
        self.set_value(offset=0x08, value=self.ENG_CERT_SIZE)
        self.set_value(offset=0x0C, value=MagicNumber.CERT_SECTION_DESCRIPTOR)
        self.set_value(offset=0x24, value=0x100) # Allow engineering root key

        convert = Convert()
        self.set_raw_value(convert.hex_string_to_bytes(uid, 8), 0x40)
        self.set_raw_value(convert.hex_string_to_bytes(hmac, 32), 0x60)
        self.set_raw_value(convert.hex_string_to_bytes(pub_key_hash, 64), 0x80)

        self.update_crc()

    # Ensures all strings in the provided iterable are hex format
    def validate_hex_strings(self, strings):
        for s in strings:
            if (re.match('0x[0-9a-fA-F]+$', s) == None):
                raise ValueError("'" + str(s) + "' is not a hex string")

    def hmac(self):
        return self.get_raw_value(offset=0x60, size=32)

    def uid(self):
        return self.get_raw_value(offset=0x40, size=8)

    def pub_key_hash(self):
        return self.get_raw_value(offset=0x80, size=64)

    def validate(self):
        if self.size() != 4096:
            raise ValueError('engineering cert header is not valid - size is ' + str(self.size()))

        return super(EngineeringCertHeader,self).validate()

    def fragment_properties_str(self):
        convert = Convert()
        return_string = super(EngineeringCertHeader, self).fragment_properties_str()
        return_string += "  [Size: " + str(self.size()//1024) + " KBytes]\n"
        return_string += "  UID:              " + convert.bytearray_to_hex_string(self.uid()) + "\n"
        return_string += "  HMAC:             " + convert.bytearray_to_hex_string(self.hmac()) + "\n"
        return_string += "  Public Key Hash:  " + convert.bytearray_to_hex_string(byte_array=self.pub_key_hash(), endianness='big').rstrip('0') + " (Big-Endian)\n"
        return_string += "  Public version:   " + str(self.get_value(offset=0x4)) + "\n"
        return_string += "\n"
        return return_string

class EngineeringCertHeaderTest(unittest.TestCase):

    def test_initialize(self):
        hdr = EngineeringCertHeader()
        hdr.initialize('0x1', '0x2', '0x3')
        hdr.validate()
        self.assertEqual(hdr.uid(), bytearray([1]+[0 for i in range(0,7)]))
        self.assertEqual(hdr.hmac(), bytearray([2]+[0 for i in range(0,31)]))
        self.assertEqual(hdr.pub_key_hash(), bytearray([3]+[0 for i in range(0,63)]))

    def testInvalidInput(self):
        hdr = EngineeringCertHeader()
        # For each invalid attempt, try it in each position. '0x1', '0x2', '0x3' is known to be valid due to the above test.
        # Each invalid attempt is a tuple of exception type and data.
        invalid_attempts = [ '0x01g', 'x1', '1', '0x1234567890abcdefg', 1, bytearray([1]), dict(), 1.2, '0x1.2' ]
        for a in invalid_attempts:
            self.assertRaises(Exception, hdr.initialize, a, '0x2', '0x3')
            self.assertRaises(Exception, hdr.initialize, '0x1', a, '0x3')
            self.assertRaises(Exception, hdr.initialize, '0x1', '0x2', a)

    # Mostly the same as testInvalidInput, but make sure validate_hex_strings itself is functioning
    def testValidateHexStrings(self):
        hdr = EngineeringCertHeader()
        hdr.validate_hex_strings(['0x123abc', '0x1', '0x2', '0x3'])
        self.assertRaises(Exception, hdr.validate_hex_strings, ['0x123abcg', '0x1', '0x2', '0x3'])

        # These should never happen since validate_hex_strings is only called internally, but check anyway.
        self.assertRaises(Exception, hdr.validate_hex_strings, bytearray([1,2,3]))
        self.assertRaises(Exception, hdr.validate_hex_strings, {'abc': '0x123'})


if __name__ == '__main__':
    unittest.main()
