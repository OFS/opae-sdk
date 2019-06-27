#!/usr/bin/env python
import unittest

from fragment import BitstreamFragment
from magic import MagicNumber

class Signature(BitstreamFragment):

    def __init__(self, enable_multi=False):
        super(Signature,self).__init__(byte_array=None, max_size=112)
        self.expected_magic.append((MagicNumber.SIGNATURE,0x0))
        
        # case:607626 TODO: Set the enable_multi argument properly
        # self.enable_multi = enable_multi
        self.enable_multi = False

    def initialize(self):
        super(Signature, self).initialize(size=self.max_size)

        if self.enable_multi:
            # In FM, these fields should be 0.
            # set r_size
            self.set_value(value=0, offset=4, size=4)
            # set s_size
            self.set_value(value=0, offset=8, size=4)
        else:
            # set r_size
            self.set_value(value=48, offset=4, size=4)
            # set s_size
            self.set_value(value=48, offset=8, size=4)

        # set signature hash type
        self.set_value(value=MagicNumber.SIGNATURE_HASH_SHA_384, offset=0xC, size=4)
        self.update_magic()
        return self

    def r_size(self):
        return self.get_value(offset=0x4,size=4)

    def r(self):
        return self.get_value(offset=0x10, size=self.r_size(), endianness='big')

    def s_size(self):
        return self.get_value(offset=0x8,size=4)

    def s(self):
        return self.get_value(offset=0x10+self.r_size(), size=self.x_size(), endianness='big')

    def sha_type(self):
        sha_type = self.get_value(0xC)
        if sha_type == MagicNumber.SIGNATURE_HASH_SHA_256:
            return "SHA-256"
        elif sha_type == MagicNumber.SIGNATURE_HASH_SHA_384:
            return "SHA-384"
        else:
            return "unknown"

    def validate(self):
        assert(self.size() == 112)

        # case:607626 TODO: Re-enable these checks
        # if self.enable_multi:
        #     assert(self.s_size() == 0)
        #     assert(self.r_size() == 0)
        # else:
        #     assert(self.s_size() == 48)
        #     assert(self.r_size() == 48)

        if self.sha_type() == "unknown":
            raise ValueError('signature hash sha type is unknown')

        # CSS only supports SHA-384, so let's restrict this to only support SHA-384
        assert(self.sha_type() == "SHA-384")

        return super(Signature,self).validate()

class SignatureTest(unittest.TestCase):

    def test_constructor(self):
        s = Signature()
        s.initialize()
        s.validate()

    def test_constructor_with_multi_format(self):
        s = Signature(enable_multi=True)
        s.initialize()
        s.validate()

if __name__ == '__main__':
    unittest.main()
