#!/usr/bin/env python

import os
import unittest

from cmf_descriptor import CmfDescriptor
from section import BitstreamSection
from signature_descriptor import SignatureDescriptor


class Cmf(BitstreamSection):

    def __init__(self, enable_multi=False):
        super(Cmf,self).__init__(max_fragment_size=4096)
        self.expected_magic = []
        self.filename = None

    def create(self, fragments=None, size=0, cmf_descriptor=CmfDescriptor(), signature_descriptor=SignatureDescriptor()):
        if size != 0:
            raise ValueError('size attribute to initialize method of cmf must be 0')
        assert(isinstance(cmf_descriptor, CmfDescriptor))
        assert(isinstance(signature_descriptor, SignatureDescriptor))

        self.fragments = []
        self.fragments.append(cmf_descriptor)
        self.fragments.append(signature_descriptor)
        assert(len(self.fragments) == 2)

        if fragments is not None:
            self.fragments += fragments
            assert(self.number_of_fragments() == 2 + len(fragments))
        else:
            assert(self.number_of_fragments() == 2)

        self.validate_size()

        assert(isinstance(self.cmf_descriptor(), CmfDescriptor))
        assert(isinstance(self.signature_descriptor(), SignatureDescriptor))
        return self

    def initialize(self):
        self.create()
        self.cmf_descriptor().initialize()
        self.signature_descriptor().initialize()
        self.signature_descriptor().set_raw_value(byte_array=self.cmf_descriptor().sha384(), offset=0)
        self.update()
        return self

    def read(self, fp):
        cmf_descriptor = CmfDescriptor()
        cmf_descriptor.read(fp)

        signature_descriptor = SignatureDescriptor(enable_multi=cmf_descriptor.is_fm_format())
        signature_descriptor.read(fp)

        self.create(cmf_descriptor=cmf_descriptor, signature_descriptor=signature_descriptor)

        self.max_size = self.get_value(offset=0x408, size=4)
        if self.max_size == 0:
            self.max_size = None
        else:
            assert(self.max_size > 0)

        return super(Cmf, self).read(fp)

    def load(self, filename=None):
        if filename is not None:
            self.filename = filename
        return super(Cmf,self).load(filename=self.filename)

    def save(self, filename=None):
        if filename is not None:
            self.filename = filename
        return super(Cmf,self).save(filename=self.filename)

    def number_of_blocks(self):
        return self.number_of_fragments()

    def block(self, block_num):
        return self.fragment(block_num)

    def cmf_descriptor(self):
        return self.block(0)

    def signature_descriptor(self):
        return self.block(1)

    def validate(self):
        if self.size() == 0:
            raise ValueError('CMF is not valid - size is zero')

        if not isinstance(self.cmf_descriptor(), CmfDescriptor):
            raise ValueError('CMF is not valid - cmf descriptor is not the correct type')

        if not isinstance(self.signature_descriptor(), SignatureDescriptor):
            raise ValueError('CMF is not valid - signature descriptor is not the correct type')

        if self.cmf_descriptor().sha384() != self.signature_descriptor().block0_hash():
            raise ValueError('CMF is not valid - signature descriptor block0 hash does not match block0 calculated sha384 hash')

        assert(self.section_type() == 'CMF')

        return super(Cmf,self).validate()

    def fragment_properties_str(self):
        return_string = super(Cmf, self).fragment_properties_str()
        return_string += "  Filename: " + str(self.filename) + "\n"
        return_string += "  [Size: " + str(self.size()/1024) + " KBytes]\n"
        return_string += "  SHA-256: " + self.sha256sum() + "\n"
        return_string += "\n"
        return return_string

class CmfTest(unittest.TestCase):

    def setUp(self):
        if not os.path.isfile("../test/files/example.cmf"):
            raise ValueError("../test/files/example.cmf not found")
        if not os.path.isfile("../test/files/example_fm.cmf"):
            raise ValueError("../test/files/example_fm.cmf not found")

    def testConsturctor(self):
        cmf = Cmf()
        self.assertNotEqual(cmf, None)
        cmf.load("../test/files/example.cmf")
        cmf.validate()

        self.assertNotEquals(cmf.cmf_descriptor().descriptor_format(),"unknown")

        #ToDo: This value is 16 for sdm full and 0 for sdm lite
        #self.assertEquals(cmf.cmf_descriptor().descriptor_format_version(),16)

        # ToDo: date of creation is not zero -- we need a way to make it zero
        #self.assertEquals(cmf.cmf_descriptor().date_of_creation(),0)

        # cmf's intended for signing must have tool_information set to zero
        self.assertEquals(cmf.cmf_descriptor().tool_information(),0)

        self.assertEqual(cmf.cmf_descriptor().crc(),cmf.cmf_descriptor().crc(calculate=True))
        self.assertEqual(cmf.signature_descriptor().crc(),cmf.signature_descriptor().crc(calculate=True))

        cmf.save("../work_dir/main_out.cmf")
        self.assertEqual(cmf.size(), 110592)
        cmf_size_pre_load = cmf.size()


        cmf.load("../work_dir/main_out.cmf")
        self.assertEqual(cmf.size(), cmf_size_pre_load)

        self.assertEqual(cmf.sha256sum(),"e514b4d4ea95ab9518fbe9f7479e8b9a541133027bd3223106ed095607e85f94")

    def testConsturctor(self):
        cmf = Cmf()
        self.assertNotEqual(cmf, None)
        cmf.load("../test/files/example_fm.cmf")
        cmf.validate()

        self.assertNotEquals(cmf.cmf_descriptor().descriptor_format(),"unknown")

        #This value is 0x20 for sdm fm
        self.assertEquals(cmf.cmf_descriptor().descriptor_format_version(),0x20)

        # cmf's intended for signing must have tool_information set to zero
        self.assertEquals(cmf.cmf_descriptor().tool_information(),0)

        self.assertEqual(cmf.cmf_descriptor().crc(),cmf.cmf_descriptor().crc(calculate=True))
        self.assertEqual(cmf.signature_descriptor().crc(),cmf.signature_descriptor().crc(calculate=True))

        cmf.save("../work_dir/main_fm_out.cmf")
        self.assertEqual(cmf.size(), 143360)
        cmf_size_pre_load = cmf.size()


        cmf.load("../work_dir/main_fm_out.cmf")
        self.assertEqual(cmf.size(), cmf_size_pre_load)

        self.assertEqual(cmf.sha256sum(),"04a398e4f98efd27a6670647e7230671d250739efaa8b7db2077090dfc0ef11c")


    def testInitialize(self):
        cmf = Cmf().initialize()
        cmf.validate()


if __name__ == '__main__':
    unittest.main()
