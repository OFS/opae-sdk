#!/usr/bin/env python

import unittest

from bitstream.cmf import Cmf
from bitstream.signature import Signature
from bitstream.signature_chain_block0_entry import SignatureChainBlock0Entry
from util.exttool import ExtTool
from util.fileutil import FileUtils
from sign.module import CmfDescriptorCssModule
import os

#
#  This is a code wrapper for the cmf_sign tool which is used for ephemeral code signing
#

class CmfSign(ExtTool):
    def __init__(self):
        super(CmfSign, self).__init__()
        self.signatures = {}
        self.ipfiles = {}
        self.opfiles = {}
        self.signfilename = None
        self.newkeyfilename = None

    def create_tmp_files(self, filestosign):
        self.filestosign = filestosign
        self.cleanup()

        # Generate the sign file
        self.signfilename = FileUtils.create_tmp_file(suffix=".txt", text=False)
        signfile = open(self.signfilename, "w")
        self.ipfiles = {}
        self.opfiles = {}
        for filename in filestosign:
            #Store the input modules to disk
            input_temp_filename = FileUtils.create_tmp_file(suffix=".module", text=False)
            self.ipfiles[filename] = input_temp_filename

            newfile = open(input_temp_filename, "wb")
            newfile.write(filestosign[filename])
            newfile.close()

            #Create a temp name for output modules
            outputtempfilename = FileUtils.create_tmp_file(suffix=".module", text=False)
            self.opfiles[filename] = outputtempfilename

            #Store the entry to signfile
            signfile.write(input_temp_filename + ';' + outputtempfilename + '\n')
        signfile.close()

    def cmf_sign_name(self):
        result = "cmf_sign"
        if os.name == "nt":
            currdir = os.path.dirname(os.path.abspath(__file__))
            result = os.path.join(currdir, "..", "cmf_sign", "cmf_sign.exe")
        return result

    def sign_files(self):
        assert(self.signfilename)
        self.signatures = {}
        # New PKey will be created by Chung Shien's tool:
        self.newkeyfilename = FileUtils.create_tmp_file(suffix=".module", text=False)

        # Run Chung Shien's tool to perform the ephemeral signing
        exttool = self.execute([self.cmf_sign_name(), self.signfilename, self.newkeyfilename])
        # Attempt to load in the results
        for filename in self.opfiles:
           try:
               self.signatures[filename] = CmfDescriptorCssModule().load(self.opfiles[filename])
           except:
               print "cmf_sign produced bad file:" + self.opfiles[filename]
               raise Exception('The cmf_sign tool was unable to create a proper set of files')
        return True

    def get_pub_key_filename(self):
        return self.newkeyfilename

    def get_signature(self, filename):
        if not filename in self.signatures:
            return None
        newblock0_sig = SignatureChainBlock0Entry().initialize()
        newblock0_sig.set_signature(self.signatures[filename].signature())
        newblock0_sig.validate()
        return newblock0_sig

    def cleanup(self):
        for filename in self.ipfiles:
            FileUtils.if_exists_delete_file(self.ipfiles[filename])
        ipfiles = {}
        for filename in self.opfiles:
            FileUtils.if_exists_delete_file(self.opfiles[filename])
        opfiles = {}
        FileUtils.if_exists_delete_file(self.signfilename)
        self.signfilename = None


class CmfSignTest(unittest.TestCase):

    def test_constructor(self):
        cmfsign = CmfSign()
        self.assertNotEqual(cmfsign, None)

    def test_outoforder(self):
        cmfsign = CmfSign()

        # cleanup valid at any time
        cmfsign.cleanup()

        # get_signature should return None if called ooo
        self.assertEquals(None, cmfsign.get_signature("blat"))

        # get_pub_key_filename should return None if called ooo
        self.assertEquals(None, cmfsign.get_pub_key_filename())

        # sign_files should assert if called ooo
        test_failed = 1
        try:
            cmfsign.sign_files()
        except:
            test_failed = 0
        self.assertEquals(test_failed, 0)

    def test_cmf_sign(self):
        cmfsign = CmfSign()
        filestosign = {}

        cmf = Cmf()
        cmf.load("../test/files/example.cmf")
        filestosign["first"] = CmfDescriptorCssModule(cmf_descriptor=cmf.cmf_descriptor(), signature=Signature().initialize()).get_raw_byte_array()

        cmfsign.create_tmp_files(filestosign)

        cmfsign.sign_files()

        self.assertNotEqual(cmfsign.get_pub_key_filename(), None)

        self.assertNotEqual(cmfsign.get_signature("first"), None)

        cmfsign.cleanup()
