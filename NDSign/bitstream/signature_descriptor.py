#!/usr/bin/env python
import unittest

from fragment import BitstreamFragment
from fragment_collection import BitstreamFragmentCollection
from signature_chain_section import SignatureChainSection


class SignatureDescriptor(BitstreamFragmentCollection):

    def __init__(self, enable_multi=False):
        super(SignatureDescriptor,self).__init__(fragments=None,max_size=4096)
        self.crc_addresses.append(0xFFC)
        block0_hash = BitstreamFragment(max_size=64)
        block0_hash.description = "SHA384 hash over Block0"

        # In Falcon Mesa, the default number of sign chains is 8. In ND, it was 4.
        number_of_signature_chains = 8 if enable_multi else 4

        # 32Bytes header plus space for Signature Chains (upto 3744 Bytes) 3744 = 4096-64-32-252-4
        signatures = SignatureChainSection(number_of_signature_chains=number_of_signature_chains, max_size=(32+3744))
        signatures.offset_within_parent = block0_hash.max_size

        main_image_pointer_plus_crc =  BitstreamFragment(max_size=(252+4))
        fragments = [block0_hash, signatures, main_image_pointer_plus_crc]
        super(SignatureDescriptor,self).initialize(fragments=fragments)

    def initialize(self):
        self.fragment(0).initialize(size=64)
        self.fragment(1).initialize()
        self.fragment(2).initialize(size=(252+4))
        self.update_crc()

    def block0_hash(self):
        assert(self.get_value(offset=48, size=(64-48)) == 0)
        return self.get_raw_value(offset=0x0, size=48)

    def signatures(self):
        return self.fragment(1)

    def image_pointer_area(self):
        return self.get_raw_value(offset=0xF00, size=252)

    def fragment_properties_str(self):
        return_string = super(SignatureDescriptor, self).fragment_properties_str()
        return_string += "  [Size: " + str(self.size()//1024) + " KBytes]\n"
        return return_string

    def validate(self):
        assert(self.size() == 4096)
        return BitstreamFragment.validate(self)

class SignatureDescriptorTest(unittest.TestCase):

    def test_constructor(self):
        sig_desc = SignatureDescriptor()
        sig_desc.initialize()
        self.assertNotEquals(sig_desc, None)
        self.assertEquals(sig_desc.validate(), True)
        self.assertEquals(sig_desc.size(), 4096)

        print sig_desc
        print sig_desc.crc()
        self.assertEquals(sig_desc.crc(),2434849798)
        sig_desc.validate()

    def test_constructor_with_multi_format(self):
        sig_desc = SignatureDescriptor(enable_multi=True)

if __name__ == '__main__':
    unittest.main()
