#!/usr/bin/env python
import os
import unittest
import zipfile

from cmf import Cmf
from engineering_cert import EngineeringCert
from fragment import BitstreamFragment
from fragment_collection import BitstreamFragmentCollection
from magic import MagicNumber
from section import BitstreamSection
from signature_chain import SignatureChain
from util.convert import Convert
from util.fileutil import FileUtils
from zip import BitstreamZip


class BitstreamFactory(object):

    def __init__(self):
        pass

    def generate(self, stream, filename=None):
        magic_bytes = stream.peek(4)
        if len(magic_bytes) < 4: raise ValueError('Unknown Stream - Too Short')
        magic = Convert().bytearray_to_integer(magic_bytes, offset=0, num_of_bytes=4)

        if magic == MagicNumber().CMF_DESCRIPTOR:

            # ToDo: Add support for CmfDescriptorCssModule right here
            cmf = Cmf()

            cmf.read(fp=stream)
            cmf.validate()

            if cmf.get_value(offset=0x408, size=4) != 0:
                assert(cmf.size() == cmf.get_value(offset=0x408, size=4))

            number_of_sections_remaining = cmf.get_value(offset=4096+0xf00, size=4)

            #print str(number_of_sections_remaining)
            if number_of_sections_remaining == 0:
                if filename is not None: cmf.filename = filename
                return cmf

            # consider making an rbf/bitstream class
            bitstream_sections = BitstreamFragmentCollection(fragments=[cmf])

            # apply some recursion here
            bitstream_sections.append(fragments=self.generate(stream=stream))

            return bitstream_sections

        elif magic == MagicNumber().SECTION_DESCRIPTOR:
            # CERT sections are better handled by the EngineeringCert class, so peek ahead and use that if we can
            section_type = Convert().bytearray_to_integer(stream.peek(0xC+4), offset=0xC,num_of_bytes=4)
            if section_type == MagicNumber().CERT_SECTION_DESCRIPTOR:
                cert = EngineeringCert()
                cert.read(fp=stream)
                return cert

            section = BitstreamSection()
            section.max_size = 4096*2
            section.read(fp=stream)

            if section.size() == 0:
                return []

            section.max_size = section.get_value(offset=0x8, size=4)
            if section.size() > 4096+0xf00:
                number_of_sections_remaining = section.get_value(offset=4096+0xf00, size=4)
            else:
                number_of_sections_remaining = 0

            section.read(fp=stream)

            section.validate()

            if number_of_sections_remaining == 0:
                return [section]
            else:
                return [section] + self.generate(stream=stream)

        elif magic == MagicNumber().SIGNATURE_SINGLE_ROOT_ENTRY or magic == MagicNumber().SIGNATURE_MULTI_ROOT_ENTRY:
            sig_chain = SignatureChain()
            sig_chain.read(fp=stream)
            sig_chain.validate()
            return sig_chain

        elif magic == MagicNumber().SIGNATURE_PUBLIC_KEY_ENTRY:
            # This is a Public Key Entry
            # However, the Public Key magic number has two possible values (no enablement and enablement valid)
            # FYI, enablement valid magic number is used when the public key is part of a Multi Root Entry.
            magic_bytes = stream.peek(28)
            if len(magic_bytes) < 28: raise ValueError('Unknown Stream - Too Short')
            # Read Public Key magic (at offset 0x18)
            pubkey_magic = Convert().bytearray_to_integer(magic_bytes, offset=0x18, num_of_bytes=4)

            # Enable multi root format if magic number is PUBLIC_KEY_MULTI_ROOT
            enable_multi = pubkey_magic == MagicNumber().PUBLIC_KEY_MULTI_ROOT

            from sign.module import PublicKeyCssModule
            css_module = PublicKeyCssModule(enable_multi=enable_multi)
            css_module.read(fp=stream)
            css_module.validate()
            return css_module

        fragment = BitstreamFragment()
        fragment.read(fp=stream)

        return fragment

    def generate_from_file(self, filename, no_format=False):

        filename = FileUtils().get_file(filename)

        if zipfile.is_zipfile(filename):
            return BitstreamZip().load(filename)

        fp = FileUtils().read_binary_file_open(filename=filename)
        if no_format:
            bitstream_frag = BitstreamFragment()
            bitstream_frag.read(fp=fp)
        else:
            bitstream_frag = self.generate(stream=fp,filename=filename)
        fp.close()

        return bitstream_frag

    def generate_from_bytearray(self, byte_array):
        filename = None
        fp = None
        try:
            filename = FileUtils.create_tmp_file(prefix="ba_frag")
            with FileUtils().write_binary_file_open(filename=filename) as fp:
                fp.write(byte_array)
            FileUtils().close_binary_file(fp, filename)
            fragment = self.generate_from_file(filename)
        finally:
            if fp is not None: fp.close()
            FileUtils.if_exists_delete_file(filename)

        return fragment

class BitstreamFactoryTest(unittest.TestCase):

    def test_cmf(self):
        cmf = BitstreamFactory().generate_from_file("../test/files/example.cmf")
        cmf.validate()
        self.assertTrue(isinstance(cmf, Cmf))

    def test_eng_cert(self):
        cert = BitstreamFactory().generate_from_file("../test/files/example_eng_cert.cert")
        cert.validate()
        self.assertTrue(isinstance(cert, EngineeringCert))

    def test_zip(self):
        z = BitstreamFactory().generate_from_file("../test/files/example.zip")
        z.validate()
        self.assertTrue(isinstance(z, BitstreamZip))

    def test_sig_chain(self):
        sc = BitstreamFactory().generate_from_file("../test/keys/codesign1.qky")
        sc.validate()
        self.assertTrue(isinstance(sc, SignatureChain))

    def test_rbf(self):
        rbf = BitstreamFactory().generate_from_file("../test/files/example.rbf")
        rbf.validate()
        self.assertTrue(isinstance(rbf, BitstreamFragmentCollection))
        print 'rbf has ' + str(rbf.number_of_fragments()) + ' sections'

        # if this fails -- see build issue 8257
        # it failed again in build issue 8303 -- with 2 != 3 (different signature this time)
        # ToDo: You must go to the Toronto build machine and try this before you re-enable
        # Issue fixed 6/25/2017 -- need to test this well before allowing this unit test
        if os.name == 'nt':
            self.assertEqual(rbf.number_of_fragments(), 4)

    def test_peek_keychain(self):
        keychain_file = "../test/keys/codesign1.qky"
        fp = FileUtils().read_binary_file_open(filename=keychain_file)

        magic_bytes = fp.peek(24)
        if len(magic_bytes) < 24: raise ValueError('Unknown Stream - Too Short')
        # Read Public Key magic (at offset 0x20)
        pubkey_magic = Convert().bytearray_to_integer(magic_bytes, offset=0x20, num_of_bytes=4)

        self.assertTrue(pubkey_magic == MagicNumber().PUBLIC_KEY_SINGLE_ROOT)

        fp.close()
