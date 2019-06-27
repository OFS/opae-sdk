#!/usr/bin/env python

import unittest

from fragment import BitstreamFragment
from magic import MagicNumber


class CmfDescriptor(BitstreamFragment):

    def __init__(self, byte_array=None):
        super(CmfDescriptor,self).__init__(byte_array=byte_array,max_size=4096)
        self.crc_addresses.append(0xFFC)
        self.crc_addresses.append(0x3B4)
        self.expected_magic.append((MagicNumber.CMF_DESCRIPTOR,0x0))
        self.expected_magic.append((MagicNumber.CMF_DESCRIPTOR_CMF_DESCRIPTION,0x400))

    def initialize(self):
        super(CmfDescriptor,self).initialize(size=self.max_size)

        # assume sdm full
        self.set_value(offset=0x4, value=0x10)
        self.set_value(offset=0x13C, value=MagicNumber.HASH_SHA_512)

        self.set_value(value=0x400, offset=0x8)
        self.set_value(value=MagicNumber.CMF_SECTION_DESCRIPTOR, offset=0x40C)
        self.update_crc()

    def descriptor_format_version(self):
        return self.get_value(offset=4)

    def date_of_creation(self):
        return self.get_value(4,8)

    def physjtag(self):
        return "0x{0:7X}".format(self.get_value(0x434))

    def jtagid(self):
        note = ""
        id_raw = self.get_value(0x180)
        id_mask_raw = self.get_value(0x184)
        masked_id = id_raw & 0x0FF00FFF
        if self.get_value(0x4) == 0X0:
            if masked_id == 0x032000DD:
                note += "(raw)"
            else:
                note += "(cooked)"
                masked_id |= 0x032000DD
        sb = []
        show_mask = False
        for i in range(28, -4, -4):
            m = (id_mask_raw >> i) & 0xF
            d = (id_raw >> i) & 0xF
            if m == 0:
                if d == 0:
                    sb.append('X')
                else:
                    sb.append('&')
            else:
                sb.append("{0:X}".format(d))
                if m != 15:
                    show_mask = True

        if show_mask == True:
            sb.append("/{0:08X}".format(id_mask_raw))
        if note:
            sb.append(note)
        return "".join(str(x) for x in sb)

    def tool_information(self):
        return self.get_value(0x80, 0x40)

    def sha_type(self):
        sha_type = self.get_value(0x13C)
        if sha_type == MagicNumber.HASH_SHA_384:
            return "SHA-384"
        elif sha_type == MagicNumber.HASH_SHA_512:
            return "SHA-512"
        else:
            return "unknown"

    def descriptor_format(self):
        desc_format = self.descriptor_format_version()
        if desc_format == 0x0:
            return "SDM-lite"
        elif desc_format == 0x10:
            return "SDM-full"
        elif desc_format == 0x20:
            return "FM"
        else:
            return "unknown"

    def is_fm_format(self):
        return self.descriptor_format() == "FM"

    def build_number(self):
        return self.get_value(0x42C)

    def version(self):
        return self.get_value(0x420)

    def version_string(self):
        ver = self.version()
        acds_major = (ver>>24) & 0xFF
        acds_minor = (ver>>16) & 0xFF
        # FYI acds_update calculation matches the firmware build, but disagrees with the Nadder_Config_Data spec
        # the spec says ver bits 0-15 represent the update version, this assumes 8-15
        acds_update = (ver>>8) & 0xFF
        acds_acs_patch = (ver) & 0xFF

        version_string = str(acds_major) + "." + str(acds_minor)
        if acds_update != 0:
            version_string += "." + str(acds_update)

        version_string += " b" + str(self.build_number())

        if acds_acs_patch != 0:
            version_string += " p" + str(acds_acs_patch)

        return version_string

    def build_label(self):
        label = self.get_raw_value(offset=0xFE0, size=28)
        return str(label).rstrip('\0')

    def osc1_clk(self):

        osc1_clk_1 = self.get_value(0x94,size=4)
        if osc1_clk_1 & (1<<31) == (1<31):
            return "Internal Oscillator"
        else:
            clk_freq = osc1_clk_1 & ((1<<28)-1)
            if clk_freq == 0:
                return "Disabled"
            else:
                return str(clk_freq) + "Hz"

    def configuration_type(self):
        cfg_type = self.get_value(0x49B,size=1)
        if cfg_type == 0:
            return "other"
        elif cfg_type == 8:
            return "AVSTx32"
        elif cfg_type == 9:
            return "QSPI"
        elif cfg_type == 10:
            return "NAND"
        elif cfg_type == 12:
            return "SDMMC"
        elif cfg_type == 13:
            return "AVSTx16"
        elif cfg_type == 14:
            return "AVSTx8"
        else:
            return "unknown"

    def hps_flags(self):

        hps_flags = self.get_value(0x499,size=1)
        flags = []

        if hps_flags & 0x1 == 0x1:
            flags.append("HPS-JTAG & SDM-JTAG: Same Chain")
        elif hps_flags & 0x2 == 0x2:
            flags.append("HPS-JTAG & SDM-JTAG: Separate")
        elif hps_flags & 0x3 == 0x3:
            flags.append("HPS-JTAG: Illegal Setting")
        else:
            flags.append("HPS-JTAG: Disabled")

        return flags

    def validate(self):

        if self.size() == 0:
            raise ValueError('cmf descriptor is not valid - size is zero')

        if self.size() != 4096:
            raise ValueError('cmf descriptor is not valid - size is ' + str(self.size()))

        if self.sha_type() == "unknown":
            raise ValueError('cmf is not valid - SHA Type at offset 0x13C is not valid')
        if self.descriptor_format() == "SDM-full":
            if self.sha_type() != "SHA-512":
                raise ValueError('cmf is not valid - SHA Type ' + self.sha_type() + ' at offset 0x13C is not valid for SDM-full')
        elif self.descriptor_format() == "SDM-lite":
            if self.sha_type() != "SHA-384":
                raise ValueError('cmf is not valid - SHA Type ' + self.sha_type() + ' at offset 0x13C is not valid for SDM-lite')
        elif self.descriptor_format() == "FM":
            if self.sha_type() != "SHA-512":
                raise ValueError('cmf is not valid - SHA Type ' + self.sha_type() + ' at offset 0x13C is not valid for FM')
        else:
            raise ValueError('cmf is not valid - descriptor format is not SDM-full or SDM-lite')

        # extra assertions made by css tool
        assert(self.get_value(offset=0x8) == 0x400) # Length of Bootrom header
        assert(self.get_value(offset=0xC) == 0) # Flags

        # b30:0 is public version number which is 0 in current spec
        # b31 is release flag, which can be 1.
        assert(self.get_value(offset=0x404) == 0 or self.get_value(offset=0x404) == 1<<31)

        assert(self.get_value(offset=0x40C) == 0x464D43) # Type of Section a.k.a. "CMF"

        return super(CmfDescriptor,self).validate()

    def fragment_properties_str(self):
        return_string = super(CmfDescriptor, self).fragment_properties_str()
        return_string += "  [Size: " + str(self.size()/1024) + " KBytes]\n"
        return_string += "  Format: " + str(self.descriptor_format()) + "\n"
        return_string += "  SHA Type: " + str(self.sha_type()) + "\n"
        return_string += "  Version: " + self.version_string() + "\n"
        return_string += "  Build Label: " + self.build_label() + "\n"
        return_string += "  Jtag Physical ID: " + self.physjtag() + "\n"
        return_string += "  Jtag ID/Mask: " + self.jtagid() + "\n"
        return_string += "  OSC Clock: " + self.osc1_clk() + "\n"
        return_string += "  HPS Flags: " + str(self.hps_flags()) + "\n"
        return_string += "  Configuration Type: " + str(self.configuration_type()) + "\n"
        return_string += "\n"
        return return_string

class CmfDescriptorTest(unittest.TestCase):

    def setUp(self):
        self.longMessage = True

    def init_cmf(self):
        ba = bytearray([0]*4096)
        cmf_desc = CmfDescriptor(ba)
        self.assertNotEquals(cmf_desc, None)

        cmf_desc.set_value(value=0x62294895, offset=0)
        cmf_desc.set_value(value=0x97566593, offset=0x400)
        cmf_desc.set_value(value=0x49303819, offset=0x13C)
        cmf_desc.set_value(value=0x400, offset=0x8)
        cmf_desc.set_value(value=0x464D43, offset=0x40C)
        cmf_desc.set_value(value=cmf_desc.crc(calculate=True, crc_addr=0x3B4), offset=0x3B4)
        cmf_desc.set_value(value=cmf_desc.crc(calculate=True), offset=0xFFC)

        return cmf_desc

    def test_constructor(self):
        cmf_desc = self.init_cmf()
        cmf_desc.validate()
        cmf_desc.update_crc()
        cmf_desc.validate()
        # By default, this descriptor is in SDM-full format
        self.assertFalse(cmf_desc.is_fm_format())


    def test_crc_update(self):
        cmf_desc = self.init_cmf()

        # Descriptor Format Version
        cmf_desc.set_value(value=0x0, offset=0x4)

        # BootROM Header
        cmf_desc.set_value(value=0x400, offset=0x8)

        # Flags
        cmf_desc.set_value(value=0x0, offset=0xC)

        # Section Type = CMF
        cmf_desc.set_value(value=0x464D43, offset=0x40C)

        cmf_desc.update_crc()

        cmf_desc.validate()


    def test_initialize(self):
        cmf_desc = CmfDescriptor()
        cmf_desc.initialize()
        cmf_desc.validate()

    def test_version_string(self):
        cmf_desc = self.init_cmf()
        cmf_desc.set_value(value=0x12000043, offset=0x420)
        actual = cmf_desc.version_string()
        expected =  "%d.%d b%d p%d" % (0x12, 0x00, cmf_desc.build_number(), 0x43)
        self.assertEqual(actual,expected,"version string mismatch")

    def test_fm_desc_format(self):
        cmf_desc = self.init_cmf()
        cmf_desc.set_value(offset=4, value=0x20)
        cmf_desc.set_value(offset=0x13C, value=MagicNumber.HASH_SHA_512)
        cmf_desc.update_crc()
        cmf_desc.validate()
        self.assertTrue(cmf_desc.is_fm_format())


if __name__ == '__main__':
    unittest.main()
