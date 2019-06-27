#!/usr/bin/env python

import unittest

from fragment import BitstreamFragment
from magic import MagicNumber
from signature import Signature
from signature_chain_entry import SignatureChainEntry


class SignatureChainBlock0Entry(SignatureChainEntry):

    def __init__(self, max_size=None, enable_multi=False):
        data = BitstreamFragment(max_size=0)
        signature = Signature(enable_multi=enable_multi)
        super(SignatureChainBlock0Entry,self).__init__(data=data, signature=signature, max_size=None)
        self.expected_magic.append((MagicNumber.SIGNATURE_BLOCK0_ENTRY,0x0))

    def initialize(self):
        self.header().initialize(size=self.header().max_size)
        assert(self.data().size() == 0)
        self.signature().initialize()
        self.update_magic()
        self.update()
        return self

    def validate(self):

        assert(self.data().size() == 0)

        if self.signature_length() != 112:
            raise ValueError('Signature length for block0 signature chain entry is not 112 Bytes as expected')

        assert(self.number_of_fragments() == 3)
        return super(SignatureChainBlock0Entry,self).validate()

class SignatureChainBlock0EntryTest(unittest.TestCase):

    def test_constructor(self):
        blk0 = SignatureChainBlock0Entry()
        blk0.initialize()
        blk0.validate()

        blk1 = SignatureChainBlock0Entry(enable_multi=True)
        blk1.initialize()
        blk1.validate()

if __name__ == '__main__':
    unittest.main()
