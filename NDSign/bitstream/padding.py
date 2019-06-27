#!/usr/bin/env python
import unittest

from fragment import BitstreamFragment
from fragment_collection import BitstreamFragmentCollection


class Padding(BitstreamFragment):

    def __init__(self, parent, pad_value=0, max_size=None):
        self.__cycle_detect = False
        self.initialized = False
        self.parent = parent
        self.pad_value = pad_value
        assert(self.parent is not None)
        assert(isinstance(self.parent, BitstreamFragmentCollection))
        super(Padding,self).__init__(byte_array=None, max_size=max_size)

    def initialize(self):
        self.initialized = True
        return self

    def size(self):
        if not self.initialized:
            return 0

        assert(self.__cycle_detect == False)
        self.__cycle_detect = True

        used_space = 0
        for f in self.parent.fragments:
            if f == self:
                continue
            elif isinstance(f, Padding):
                raise ValueError('Parent of this padding fragment has more than one padding fragment. This is not allowed')
            used_space += f.size()

        assert(self.parent.max_size != None)
        pad_size = self.parent.max_size - used_space
        if self.max_size != None and pad_size > self.max_size:
            pad_size = self.max_size
        assert(pad_size >= 0)

        self.__cycle_detect = False
        return pad_size

    def get_raw_byte_array(self):
        return bytearray([self.pad_value] * self.size())

    def read(self, fp):
        if fp.closed: return 0

        self.initialized = True

        read_size = self.size()
        if read_size > 0:
            ba = bytearray(fp.read(read_size))
            if len(ba) != read_size:
                fp.close()
                # I don't think this should ever happen
                assert(0)

            for byte_i in ba:
                assert(byte_i == self.pad_value)

            return len(ba)
        return 0

    def set_raw_value(self, byte_array, offset, endianness='little'):
        raise ValueError('Cannot set value in a padded area')


class PaddingFragmentTest(unittest.TestCase):

    def test_simple(self):
        bfc = BitstreamFragmentCollection(max_fragment_size=128, max_size=1000)
        bfc.append(size=99)
        bfc.append(size=101)
        self.assertEquals(bfc.size(),200)
        pad = Padding(parent=bfc)
        bfc.append(fragments=[pad])
        self.assertEquals(bfc.size(),200)
        pad.initialize()
        self.assertEquals(bfc.size(),1000)
        bfc.append(size=128)
        self.assertEquals(bfc.size(),1000)
        bfc.append(size=128)
        self.assertEquals(bfc.size(),1000)
        self.assertEquals(bfc.fragment(2).size(),544)
        bfc.fragment(2).max_size=128
        self.assertEquals(bfc.fragment(2).size(),128)
        self.assertEquals(bfc.size(),584)
        bfc.validate()

if __name__ == '__main__':
    unittest.main()
