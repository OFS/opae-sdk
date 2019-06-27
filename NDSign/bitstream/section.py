#!/usr/bin/env python

import unittest

from magic import MagicNumber
from fragment_collection import BitstreamFragmentCollection


class BitstreamSection(BitstreamFragmentCollection):

    def __init__(self, fragments=None, max_size=None, max_fragment_size=None):
        super(BitstreamSection,self).__init__(fragments=fragments, max_size=max_size, max_fragment_size=max_fragment_size)
        self.expected_magic.append((MagicNumber.SECTION_DESCRIPTOR,0x0))

    def initialize(self):
        super(BitstreamSection,self).initialize(size=self.max_size)
        self.update_magic()
        self.set_value(value=MagicNumber.CORE_SECTION_DESCRIPTOR, offset=0x00C, size=4)

    def section_type(self):
        magic = self.get_value(offset=0x0,size=4)
        if magic == MagicNumber.CMF_DESCRIPTOR:
            section_type_offset = 0x40C
        else:
            assert(self.magic_number() == MagicNumber.SECTION_DESCRIPTOR)
            section_type_offset = 0x00C

        section_type_magic = self.get_value(offset=section_type_offset ,size=4)

        if section_type_magic == MagicNumber.CMF_SECTION_DESCRIPTOR:
            return 'CMF'
        elif section_type_magic == MagicNumber.IO_SECTION_DESCRIPTOR:
            return 'IO'
        elif section_type_magic == MagicNumber.CORE_SECTION_DESCRIPTOR:
            return 'CORE'
        elif section_type_magic == MagicNumber.HPS_SECTION_DESCRIPTOR:
            return 'HPS'
        elif section_type_magic == MagicNumber.PR_SECTION_DESCRIPTOR:
            return 'PR'
        elif section_type_magic == MagicNumber.TEST_SECTION_DESCRIPTOR:
            return 'TEST'
        elif section_type_magic == MagicNumber.CERT_SECTION_DESCRIPTOR:
            return 'CERT'
        elif section_type_magic == MagicNumber.PUF_SECTION_DESCRIPTOR:
            return 'PUF'
        elif section_type_magic == MagicNumber.HPIO_SECTION_DESCRIPTOR:
            return 'HPIO'
        else:
            print hex(section_type_magic)
            return 'unknown'

    def validate(self):
        assert(self.section_type() != "unknown")
        return super(BitstreamSection,self).validate()

    def fragment_properties_str(self):
        return_string = super(BitstreamSection, self).fragment_properties_str()
        return_string += "  SectionType: " + self.section_type() + "\n"
        return return_string

class BitstreamSectionTest(unittest.TestCase):

    def test_nothing_really(self):
        section = BitstreamSection(max_size=4096*100)
        section.initialize()
        section.validate()
