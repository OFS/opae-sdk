#!/usr/bin/env python

import unittest

from fragment import BitstreamFragment
from fragment_collection import BitstreamFragmentCollection
from padding import Padding
from signature_chain import SignatureChain


class SignatureChainSection(BitstreamFragmentCollection):
    '''
    A collection of SignatureChain Objects (with a header and padding)
    '''

    # In Signature Descriptor Fields in Nadder, we save 4 signature chains.
    _ND_NUM_OF_SIGNATURE_CHAINS = 4
    # In Signature Descriptor Fields in Falcon Mesa, we save 8 signature chains.
    # In this format, we only write "offset to first entry" and don't write "number of entries" in each sig chain
    _FM_NUM_OF_SIGNATURE_CHAINS = 8

    # Header size
    # ND header: num entries + offset to first entry of Signature Chain 1-4
    # FM header: offset to first entry of Signature Chain 1-8
    _HEADER_SIZE = 32

    # max_size default: 4096 (signature descriptor) - 64 (SHA hash) - 252 (main image pointer) - 4 (CRC32) = 3776
    def __init__(self, number_of_signature_chains=_ND_NUM_OF_SIGNATURE_CHAINS, max_size=(32+3744)):
        assert(max_size is not None)
        assert(number_of_signature_chains in [SignatureChainSection._ND_NUM_OF_SIGNATURE_CHAINS, SignatureChainSection._FM_NUM_OF_SIGNATURE_CHAINS])
        super(SignatureChainSection,self).__init__(fragments=None, max_size=max_size)

        header = BitstreamFragment(max_size=SignatureChainSection._HEADER_SIZE)
        header.description = "Signature Chain Section Header"

        self.offset_within_parent = 0

        self.append([header])
        max_signature_chain_size = max_size-header.max_size

        for i in range(0,number_of_signature_chains):
            sigchain = SignatureChain(max_size=max_signature_chain_size)
            sigchain.description = "Signature Chain #" + str(i)
            self.append([sigchain])

        self.append([Padding(parent=self, max_size=max_signature_chain_size)])

    def initialize(self):
        assert(self.number_of_signature_chains() > 0)
        assert(self.number_of_signature_chains() in [SignatureChainSection._ND_NUM_OF_SIGNATURE_CHAINS, SignatureChainSection._FM_NUM_OF_SIGNATURE_CHAINS])

        self.header().initialize(size=(SignatureChainSection._HEADER_SIZE))
        for i in range(1,self.number_of_fragments()):
            self.fragment(i).initialize()
        return self

    def validate(self):
        assert(self.number_of_signature_chains() in [SignatureChainSection._ND_NUM_OF_SIGNATURE_CHAINS, SignatureChainSection._FM_NUM_OF_SIGNATURE_CHAINS])

        for i in range(0,self.number_of_signature_chains()):
            sigchain = self.signature_chain(i)
            num_of_entries = sigchain.number_of_signature_entries()

            if not self.is_multi_root_format():
                assert(self.header().get_value(offset=(i*8),size=4) == num_of_entries)

            if num_of_entries != 0:
                assert(sigchain.size() > 0)

                # In FM, the first byte is the offset to first entry
                if num_of_entries == SignatureChainSection._FM_NUM_OF_SIGNATURE_CHAINS:
                    sigchain_offset = self.header().get_value(offset=(i*8),size=4) - self.offset_within_parent
                else:
                    sigchain_offset = self.header().get_value(offset=(i*8)+4,size=4) - self.offset_within_parent

                assert(self.get_value(offset=sigchain_offset, size=sigchain.size())) == sigchain.get_value(offset=0, size=sigchain.size())

        return super(SignatureChainSection, self).validate()

    def read(self, fp):
        if fp.closed: return 0

        if self.size() == self.max_size:
            return 0

        assert(self.size() == 0)

        self.header().read(fp)
        if fp.closed: assert(0)

        for i in range(0,self.number_of_signature_chains()):
            # entries_pending_read is set here so that the subsequent read() call knows when to stop
            self.signature_chain(i).entries_pending_read = self.signature_chain_number_of_entries(i)
            self.signature_chain(i).read(fp)
            if fp.closed: assert(0)

        self.fragment(self.number_of_fragments()-1).read(fp)
        if fp.closed: assert(0)

        assert(self.size() == self.max_size)
        return self.size()

    def is_multi_root_format(self): 
        return self.number_of_signature_chains() == SignatureChainSection._FM_NUM_OF_SIGNATURE_CHAINS

    def header(self):
        return self.fragment(0)

    def number_of_signature_chains(self):
        return self.number_of_fragments() - 2

    def signature_chain(self,index):
        assert(index >= 0 and index < self.number_of_signature_chains())
        return self.fragment(index+1)

    def signature_chain_number_of_entries(self,index):
        assert(index >= 0 and index < self.number_of_signature_chains())
        if self.is_multi_root_format():
            # Section 7.6.1: signature verifiers should stop processing when they reach a chain with a zero offset and/or length.
            if self.header().get_value(offset=((index*4)),size=4) == 0:
                return 0
            # This gives entries_pending_read None so that it can iterates through sig chain by looking at "length"
            return None
        return self.header().get_value(offset=(index*8),size=4)

    def signature_chain_offset(self,index):
        assert(index >= 0 and index < self.number_of_signature_chains())
        if self.is_multi_root_format():
            return self.header().get_value(offset=((index*4)),size=4)
        return self.header().get_value(offset=((index*8)+4),size=4)

    def append_signature(self, signature_chain_index, signature_chain_entry):
        assert(signature_chain_index >= 0 and signature_chain_index < self.number_of_signature_chains())
        self.signature_chain(signature_chain_index).append(signature_chain_entry=signature_chain_entry)
        self.update_header()

    def update_header(self):
        sigchain_offset = self.offset_within_parent + self.header().size()

        if not self.is_multi_root_format():
            # For ND, there are 4 possible signature chains. 
            # For each chain, we store number of entries and then the offset to first entry.
            for i in range(0,self.number_of_signature_chains()):
                sigchain = self.signature_chain(i)
                num_of_entries = sigchain.number_of_signature_entries()
                self.header().set_value(value=num_of_entries,offset=(i*8),size=4)
                if num_of_entries == 0:
                    self.header().set_value(value=0,offset=(i*8)+4,size=4)
                else:
                    self.header().set_value(value=sigchain_offset,offset=(i*8)+4,size=4)
                sigchain_offset += sigchain.size()
        else:
            # For FM, there are 8 possible signature chains. 
            # For each chain, we store the offset to first entry.
            assert(self.is_multi_root_format())

            for i in range(0,self.number_of_signature_chains()):
                sigchain = self.signature_chain(i)
                if sigchain.size() == 0:
                    self.header().set_value(value=0,offset=(i*4),size=4)
                else:
                    self.header().set_value(value=sigchain_offset,offset=(i*4),size=4)
                    sigchain_offset += sigchain.size()

    def update(self):
        self.update_header()
        return super(SignatureChainSection, self).update()

    def fragment_properties_str(self):
        return_string = super(SignatureChainSection, self).fragment_properties_str()
        return_string += "  [Number of Signature Chains: " + str(self.number_of_signature_chains()) + "]\n"

        if not self.is_multi_root_format(): 
            for i in range(0,self.number_of_signature_chains()):
                return_string += "  [Number of Entries in Signature Chain #" + str(i) + ": " + str(self.signature_chain_number_of_entries(i)) + "]\n"

        return return_string


class SignatureChainSectionTest(unittest.TestCase):

    def test_constructor(self):
        scs = SignatureChainSection()
        self.assertEquals(scs.size(),0)
        scs.initialize()
        self.assertEquals(scs.size(),scs.max_size)
        self.assertNotEqual(scs, None)
        scs.validate()

    def test_constructor_with_multi_format(self):
        scs = SignatureChainSection(number_of_signature_chains=SignatureChainSection._FM_NUM_OF_SIGNATURE_CHAINS)
        self.assertEquals(scs.size(),0)
        scs.initialize()
        self.assertEquals(scs.size(),scs.max_size)
        self.assertNotEqual(scs, None)
        scs.validate()

    def test_class_constants(self):
        self.assertEquals(SignatureChainSection._ND_NUM_OF_SIGNATURE_CHAINS, 4)
        self.assertEquals(SignatureChainSection._FM_NUM_OF_SIGNATURE_CHAINS, 8)
        self.assertEquals(SignatureChainSection._HEADER_SIZE, 32)

if __name__ == '__main__':
    unittest.main()
