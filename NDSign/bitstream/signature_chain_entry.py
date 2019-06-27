#!/usr/bin/env python

import unittest

from fragment import BitstreamFragment
from fragment_collection import BitstreamFragmentCollection
from magic import MagicNumber


class SignatureChainEntry(BitstreamFragmentCollection):

    def __init__(self, header=None, data=None, signature=None, max_size=None):

        if header is None: header = BitstreamFragment(max_size=0x18)
        if data is None: data = BitstreamFragment()
        if signature is None: signature = BitstreamFragment()

        if max_size is None and header.max_size is not None and data.max_size is not None and signature.max_size is not None:
            max_size = header.max_size + data.max_size + signature.max_size

        super(SignatureChainEntry,self).__init__(fragments=[header,data,signature],max_size=max_size)

    def initialize(self):
        self.header().initialize(size=self.header().max_size)
        self.data().initialize()
        self.signature().initialize()

        self.update_magic()
        self.update();
        return self

    def header(self):
        return self.fragment(0)

    def data(self):
        return self.fragment(1)

    def signature(self):
        return self.fragment(2)

    def set_signature(self, signature):
        self.fragments[2] = signature
        self.update()

    def length(self):
        return self.get_value(offset=0x4,size=4)

    def set_length(self,value=None):
        if value is None:
            value = self.size()
        self.set_value(value=value, offset=0x4,size=4)

    def data_length(self):
        return self.get_value(offset=0x8,size=4)

    def set_data_length(self, value=None):
        if value is None:
            value = self.data().size()
        self.set_value(value=value, offset=0x8, size=4)

    def signature_length(self):
        return self.get_value(offset=0xC, size=4)

    def set_signature_length(self, value=None):
        if value is None:
            value = self.signature().size()
        self.set_value(value=value, offset=0xC, size=4)

    def sha_legnth(self):
        return self.get_value(offset=0x10, size=4)

    def set_hash_sel(self, value=0):
        # Root hash selection values: 0=intel, 1=user, 2=Engineering, 3=Manufacturing, 4=Upgrade, 5=Slot
        if value not in [0, 1, 2, 3, 4, 5]:
            raise ValueError('Root hash selection is invalid. Valid inputs are: 0, 1, 2, 3, 4, or 5. Refer to Nadder_Config_Data document for definitions.')
        self.set_value(value=value, offset=0x14, size=4)

    def hash_sel(self):
        return self.get_value(offset=0x14, size=4)

    def update_lengths(self):
        self.set_data_length()
        self.set_signature_length()
        self.set_length()

    def update(self):
        self.update_lengths()
        return super(SignatureChainEntry, self).update()

    def validate_lengths(self):
        if self.length() != self.size():
            raise ValueError('Length entry does not match signature chain entry data structure size')

        if self.data_length() != self.data().size():
            raise ValueError('Data length entry does not match data to be signed size')

        if self.signature_length() != self.signature().size():
            raise ValueError('Signature length entry does not match data to be signature size')

        if self.sha_legnth() != 0:
            raise ValueError('SHA length is reserved and should be set to zero')

        if self.length() % 8 != 0:
            raise ValueError('Signature chain entry length is not a multiple of 8')

    def validate(self):
        self.validate_lengths()
        assert(self.number_of_fragments() == 3)

        return super(SignatureChainEntry,self).validate()


class SignatureChainEntryTest(unittest.TestCase):

    def test_constructor(self):
        sce = SignatureChainEntry()
        sce.initialize()
        sce.validate()
        self.assertGreater(sce.length(), 0)

if __name__ == '__main__':
    unittest.main()
