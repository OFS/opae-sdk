#!/usr/bin/env python

import os
import unittest

from engineering_cert_header import EngineeringCertHeader
from section import BitstreamSection
from signature_descriptor import SignatureDescriptor


class EngineeringCert(BitstreamSection):
    def __init__(self):
        super(EngineeringCert,self).__init__(max_fragment_size=EngineeringCertHeader.ENG_CERT_SIZE)
        self.filename = None
        self.fragments = []
        self.fragments.append(EngineeringCertHeader())
        self.fragments.append(SignatureDescriptor())
        self.expected_magic = self.engineering_cert_header().expected_magic
        self.validate_size()

    def initialize(self, uid='0x0', hmac='0x0', pub_key_hash='0x0', temp_pub_version=0):
        self.engineering_cert_header().initialize(uid, hmac, pub_key_hash, temp_pub_version)
        self.signature_descriptor().initialize()
        self.signature_descriptor().set_raw_value(byte_array=self.engineering_cert_header().sha384(), offset=0)
        self.update()
        return self

    def engineering_cert_header(self):
        return self.fragment(0)

    def signature_descriptor(self):
        return self.fragment(1)

    def validate(self):
        if self.size() != EngineeringCertHeader.ENG_CERT_SIZE:
            raise ValueError('EngineeringCert is not valid - size is not %d bytes' % EngineeringCertHeader.ENG_CERT_SIZE)

        if self.engineering_cert_header().sha384() != self.signature_descriptor().block0_hash():
            raise ValueError('EngineeringCert is not valid - signature descriptor block0 hash does not match block0 calculated sha384 hash')

        assert(self.section_type() == 'CERT')

        return super(EngineeringCert,self).validate()

    def fragment_properties_str(self):
        return_string = super(EngineeringCert, self).fragment_properties_str()
        return_string += "  Filename: " + str(self.filename) + "\n"
        return_string += "  [Size: " + str(self.size()//1024) + " KBytes]\n"
        return_string += "  SHA-256: " + self.sha256sum() + "\n"
        return_string += "\n"
        return_string += self.engineering_cert_header().fragment_properties_str()
        return_string += self.signature_descriptor().fragment_properties_str()
        return return_string

class EngineeringCertTest(unittest.TestCase):

    def testInitialize(self):
        cert = EngineeringCert().initialize()
        cert.validate()

    def testLoad(self):
        from util.convert import Convert
        cert = EngineeringCert()
        cert.load("../test/files/example_eng_cert.cert")
        cert.validate()
        self.assertEqual(cert.engineering_cert_header().uid(),
                        Convert().hex_string_to_bytes('0x1102030405060708', 8))

        self.assertEqual(cert.engineering_cert_header().hmac(),
                         Convert().hex_string_to_bytes('0x1102030405060708091011121314151617181920212223242526272829303132', 32))

        self.assertEqual(cert.engineering_cert_header().pub_key_hash(),
                         Convert().hex_string_to_bytes('0x090AD42333A93B2D2C9A2306957DB3987B1EC010C2E6131FB5C78A8D20B881D0CB6EC2AB84A6EEB0CB21F964BF8C6318', 64))

    def testFields(self):
        cert = EngineeringCert().initialize()
        cert.validate()
        self.assertEqual(True, isinstance(cert.engineering_cert_header(), EngineeringCertHeader))
        self.assertEqual(True, isinstance(cert.signature_descriptor(), SignatureDescriptor))

    # Verify this container directly reflects (references) its contents properly
    def testReferences(self):
        cert = EngineeringCert().initialize()
        ba = bytearray([0,0,0,0,0,0,0,0])
        self.assertEqual(cert.engineering_cert_header().uid(), ba)
        ba[0] = 1
        cert.set_raw_value(ba, offset=0x40)
        self.assertEqual(cert.engineering_cert_header().uid(), ba)
        self.assertEqual(cert.get_raw_value(offset=0x40, size=8), cert.engineering_cert_header().uid())

    def testValidate(self):
        cert = EngineeringCert().initialize()
        cert.validate()
        cert.fragments.append( EngineeringCertHeader() )
        self.assertRaises(Exception, cert.validate)
        cert = EngineeringCert().initialize()
        self.assertRaises(Exception, cert.engineering_cert_header().append, bytearray([1,2]))
        cert.engineering_cert_header().get_raw_byte_array()[0:1] = bytearray([1])
        self.assertRaises(Exception, cert.validate)


if __name__ == '__main__':
    unittest.main()
