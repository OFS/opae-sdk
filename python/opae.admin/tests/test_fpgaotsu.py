# Copyright(c) 2019, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
import os
import tempfile
import unittest

from opae.admin.tools import fpgaotsu_09C4

DUMMY_FILE = 'dcp_1_2_rot_19_1_b170_reversed.rpd'
DUMMY_SIZE = 0x1800000

VALID_09C4_CFG = '''
{{
    "device": "0x09C4",
    "flash": [
        {{
            "comment": "Erase the last three 64KiB sectors.",
            "erase_end": "0x7ffffff",
            "erase_start": "0x7fd0000",
            "type": "flash"
        }},
        {{
            "comment": "Erase 0x1800000 to 0x3ffffff.",
            "erase_end": "0x3ffffff",
            "erase_start": "0x1800000",
            "type": "flash"
        }},
        {{
            "comment": "Write/Verify User POF to 0x1800000.",
            "filename": "{DUMMY_FILE}",
            "seek": "0x1800000",
            "start": "0x1800000",
            "type": "flash",
            "verify": true
        }},
        {{
            "comment": "Erase/Wr/Vfy 0x10000-0x17fffff of Gold Factory POF.",
            "end": "0x17fffff",
            "erase_end": "0x17fffff",
            "erase_start": "0x10000",
            "filename": "{DUMMY_FILE}",
            "seek": "0x10000",
            "start": "0x10000",
            "type": "flash",
            "verify": true
        }}
    ],
    "product": "Intel PAC with Intel Arria 10 GX FPGA",
    "program": "one-time-update",
    "vendor": "0x8086"
}}
'''.strip().format(DUMMY_FILE=DUMMY_FILE)


class test_09C4(unittest.TestCase):
    def setUp(self):
        self.valid_manifest = tempfile.NamedTemporaryFile(mode='w+',
                                                          prefix='otsu09C4-')
        self.valid_manifest.write(VALID_09C4_CFG)
        self.valid_manifest.seek(0)
        self.dummy_file_path = os.path.join(
            os.path.dirname(self.valid_manifest.name), DUMMY_FILE)
        self.dummy_file = open(self.dummy_file_path, 'wb+')
        self.manifest_loader = fpgaotsu_09C4.otsu_manifest_loader(
            self.valid_manifest)
        with open('/dev/urandom', 'rb') as fp:
            self.dummy_file.write(fp.read(DUMMY_SIZE))

    def tearDown(self):
        self.valid_manifest.close()
        self.dummy_file.close()
        if os.path.exists(self.dummy_file_path):
            os.unlink(self.dummy_file_path)

    def test_load(self):
        """test_load
        Given a valid manifest file
        And the manifest references flash files that exists
        in the same directory as the manifest
        And that file's size is large enough so that computed offsets stay
        within bounds
        When I call load_and_validate
        Then I get a dictionary with the expected keys/values.
        """
        cfg = self.manifest_loader.load_and_validate()
        self.assertIsInstance(cfg, dict)
        self.assertDictContainsSubset({"vendor": "0x8086",
                                       "device": "0x09C4",
                                       "program": "one-time-update"}, cfg)

    def test_load_missing_flash_file(self):
        """test_load_missing_flash_file
        Given a valid manifest file
        And the manifest references flash files that doesn't exist
        When I call load_and_validate
        Then I get 'None' as the return variable
        """
        self.dummy_file.close()
        os.unlink(self.dummy_file_path)
        self.assertIsNone(self.manifest_loader.load_and_validate())

    def test_load_small_flash_file(self):
        """test_load_small_flash_file
        Given a valid manifest file
        And the manifest references flash files that exists
        in the same directory as the manifest
        And that file's size is smaller than so that computed offsets go
        out of bounds
        When I call load_and_validate
        Then I get 'None' as the return variable
        """
        self.dummy_file.close()
        os.unlink(self.dummy_file_path)
        with open(self.dummy_file_path, 'wb') as fp:
            with open('/dev/urandom', 'rb') as inp:
                fp.write(inp.read(int(DUMMY_SIZE/2.0)))
        size = os.stat(self.dummy_file_path).st_size
        self.assertLess(size, DUMMY_SIZE)
        self.assertIsNone(self.manifest_loader.load_and_validate())
