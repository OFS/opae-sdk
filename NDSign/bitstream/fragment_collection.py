#!/usr/bin/env python

import unittest

from fragment import BitstreamFragment
from magic import MagicNumber

class BitstreamFragmentCollection(BitstreamFragment):

    def __init__(self, fragments=None, max_size=None, max_fragment_size=None):
        self.fragments = None
        self.max_fragment_size = max_fragment_size
        super(BitstreamFragmentCollection,self).__init__(byte_array=None, max_size=max_size)
        BitstreamFragmentCollection.initialize(self,fragments=fragments)

    def initialize(self, fragments=None, size=0):
        if fragments is None:
            self.fragments = []
            if size > 0:
                self.append(size=size)
        elif size != 0:
                raise ValueError('size attribute to initialize method is only valid if fragments are not specified')
        else:
            self.fragments = fragments

        self.validate_size()
        return self

    def size(self):
        size = 0
        if self.fragments is not None:
            for f in self.fragments:
                size += f.size()
        return size

    def append(self, fragments=None, size=0):
        if fragments is None:
            if size <= 0:
                raise ValueError('size attribute to append method is <= 0')
            else:
                bf = BitstreamFragment(max_size=self.max_fragment_size)
                if self.max_fragment_size is not None:
                    assert(size <= self.max_fragment_size)
                bf.append(size=size)
                self.fragments.append(bf)
        elif size != 0:
                raise ValueError('size attribute to initialize method is only valid if byte_array not specified')
        else:
            self.fragments += fragments

    def validate(self):
        for f in self.fragments:
            f.validate()
        self.validate_size()
        return super(BitstreamFragmentCollection,self).validate()

    def update(self):
        for f in self.fragments:
            f.update()
        return super(BitstreamFragmentCollection, self).update()

    def number_of_fragments(self):
        return len(self.fragments)

    def fragment(self, index):
        assert(index >= 0 and index < self.number_of_fragments())
        return self.fragments[index]

    def validate_size(self):
        if self.max_size is not None:
            assert(self.size() <= self.max_size)
        if self.max_fragment_size is not None:
            for f in self.fragments:
                assert(f.size() <= self.max_fragment_size)

    def get_raw_byte_array(self):
        ba = bytearray()

        for f in self.fragments:
            ba += f.get_raw_byte_array()

        assert(len(ba) == self.size())

        return ba

    def set_raw_value(self, byte_array, offset, endianness='little'):
        sz = len(byte_array)
        assert(offset+sz <= self.size())
        for f in self.fragments:
            if offset >= f.size():
                offset = offset - f.size()
                continue
            elif (offset < f.size()) and (offset+sz <= f.size()):
                return f.set_raw_value(byte_array=byte_array, offset=offset, endianness=endianness)
            else:
                raise ValueError('Assignment Alignment Issue - Setting values across multiple bitstream fragments is not supported')
        assert(0)

    def allocate_bitstream_fragment(self, fp, max_size):
        '''
        This method allocates a new BitstreamFragment Data Structure. It is used by the read() method.
        Subclasses can overload/mutate this method if they require a different type of BitstreamFragment
        to be allocated.
        '''
        return BitstreamFragment(max_size=max_size)

    def read(self, fp):
        if fp.closed: return 0

        size_in = self.size()
        for f in self.fragments:
            f.read(fp)
            if fp.closed:
                break

        # In the case of multi root entry, max_size of the data fragment includes 3 public keys.
        # However, we can create a multi root entry with at least one public key.
        # It's possible that self.size() < self.max_size, hence fp.closed can be False here
        # Therefore, we want to skip the while loop if this BitstreamFragmentCollection is the data in SignatureChainMultiRootEntry
        skip_allocation = self.number_of_fragments() is 4 and self.get_value(8) in MagicNumber.PUBLIC_KEY_MAGICS

        while not skip_allocation and not fp.closed:
            max_fragment_size = self.max_fragment_size
            if self.max_size is not None:
                if max_fragment_size is None:
                    max_fragment_size = self.max_size - self.size()
                else:
                    max_fragment_size = min(self.max_size-self.size(),max_fragment_size)

            b = self.allocate_bitstream_fragment(fp, max_size=max_fragment_size)
            b.read(fp)

            if b.size() > 0:
                self.append(fragments=[b])

            if self.size() == self.max_size:
                break

        read_size = self.size() - size_in
        return read_size

    def update_crc(self):
        for f in self.fragments:
            f.update_crc()
        return super(BitstreamFragmentCollection,self).update_crc()

    def __str__(self):
        return_string = "-------------------------------------\n"
        return_string +=  self.__class__.__name__ + " [Size=" + str(self.size()) + "] " + "\n"
        return_string += self.fragment_properties_str()
        for f in self.fragments:
            if f.size() == 0:
                continue
            return_string += " ----------------\n"
            return_string += str(f)
        return_string += "-------------------------------------\n"
        return return_string

class BitstreamFragmentCollectionTest(unittest.TestCase):

    def test_the_basics(self):
        bfc = BitstreamFragmentCollection()
        bfc.max_fragment_size = 4096

        bfc.validate()
        self.assertEquals(bfc.size(), 0)

        bfc.append(size=128)
        self.assertEquals(bfc.size(), 128)
        bfc.validate()

        ba = bfc.get_raw_byte_array()
        self.assertNotEqual(ba, None)
        self.assertEquals(len(ba), 128)
        self.assertEquals(bfc.size(), 128)

        bfc.set_value(value=0xdeadbeef, offset=16, size=4)
        self.assertEquals(bfc.get_value(offset=16), 0xdeadbeef)

        bfc.append(size=5)
        bfc.set_value(value=0x12345678, offset=129)
        self.assertEquals(bfc.get_value(offset=129), 0x12345678)
        self.assertEquals(bfc.size(), 133)

        bfc.max_size = 1000000

        bfc.load("../test/files/example.cmf")
        self.assertEquals(bfc.size(), 110725)
        self.assertEquals(bfc.number_of_fragments(),28)

        bfc.update_crc()
        bfc.validate()


if __name__ == '__main__':
    unittest.main()

