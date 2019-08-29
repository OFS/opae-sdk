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
import json
import os
import tempfile
import unittest
from opae.admin.tools import fpgaotsu

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

CSV_CFG = '''
"device","flash__comment","flash__end","flash__filename","flash__seek",
"flash__start","flash__erase_end","flash__erase_start","flash__type",
"flash__verify","product","program","vendor""0x09C4","Erase the last three 64KiB sectors.",
"","","","","0x7ffffff","0x7fd0000","flash","","Intel PAC with Intel Arria 10 GX FPGA",
"one-time-update","0x8086","","Erase 0x1800000 to 0x3ffffff.","","","","","0x3ffffff",
"0x1800000","flash","","","","""","Write/Verify User POF to 0x1800000.","",
"dcp_1_2_rot_19_1_b170_reversed.rpd","0x1800000","0x1800000","","","flash","True","","",""
"","Erase/Wr/Vfy 0x10000-0x17fffff of Gold Factory POF.","0x17fffff",
"dcp_1_2_rot_19_1_b170_reversed.rpd","0x10000","0x10000","0x17fffff","0x10000","flash","True",
"","",""
'''.strip().format(DUMMY_FILE=DUMMY_FILE)

def modify_manifest(data, to_dict=False, to_super_rsu=False, delete_key=None):
    mandatory_keys = ['product', 'vendor', 'device',
                      'program', 'flash']

    if to_dict:
        data.update(vendor=dict(vendor=0))
        data.update(device=dict(device=0))

    if to_super_rsu:
        data.update(program="super-rsu")

    if delete_key and delete_key in mandatory_keys:
        try:
            del data[delete_key]
        except KeyError:
            raise

    return data


class test_09C4(unittest.TestCase):
    def setUp(self):
        self.valid_manifest = tempfile.NamedTemporaryFile(mode='w+',
                                                          prefix='otsu09C4-')
        self.valid_manifest.write(VALID_09C4_CFG)
        self.valid_manifest.seek(0)
        self.dummy_file_path = os.path.join(
            os.path.dirname(self.valid_manifest.name), DUMMY_FILE)
        self.dummy_file = open(self.dummy_file_path, 'wb+')
        self.manifest_loader = fpgaotsu.otsu_manifest_loader(
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
        When I call load
        Then I get a dictionary as the returned object.
        """
        cfg = self.manifest_loader.load()
        self.assertIsInstance(cfg, dict)

    def test_load_and_validate(self):
        """test_load_and_validate
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

    def test_load_missing_flash_type(self):
        """test_load_missing_flash_type
        Given a valid manifest file
        And the manifest references no flash type
        When I call load_and_validate
        Then I get 'None' as the return variable
        """
        data = json.load(self.valid_manifest)
        new_data = modify_manifest(data, delete_key='flash')
        with tempfile.NamedTemporaryFile(mode='w+', prefix='invalid_otsu-') as fd:
            json.dump(new_data, fd, indent=4)
            manifest_loader = fpgaotsu.otsu_manifest_loader(fd)
            fd.seek(0)
            cfg = manifest_loader.load_and_validate()
            self.assertIsNone(cfg)

    def test_validate_mandatory_keys1(self):
        """test_validate_mandatory_keys1
        Given a valid manifest json
        And the manifest contains a missing key component
        When I call validate_mandatory_keys
        Then I get KeyError exception
        """
        data = json.load(self.valid_manifest)
        new_data = modify_manifest(data, delete_key='program')
        with tempfile.NamedTemporaryFile(mode='w+', prefix='invalid_otsu-') as fd:
            json.dump(new_data, fd, indent=4)
            manifest_loader = fpgaotsu.otsu_manifest_loader(fd)
            fd.seek(0)
            with self.assertRaises(KeyError):
                manifest_loader.validate_mandatory_keys(new_data)

    def test_validate_mandatory_keys2(self):
        """test_validate_mandatory_keys2
        Given a valid manifest json
        And the manifest key contains an invalid string or integer
        When I call validate_mandatory_keys
        Then I get TypeError exception
        """
        data = json.load(self.valid_manifest)
        new_data = modify_manifest(data, to_dict=True)
        with tempfile.NamedTemporaryFile(mode='w+', prefix='invalid_otsu-') as fd:
            json.dump(new_data, fd, indent=4)
            manifest_loader = fpgaotsu.otsu_manifest_loader(fd)
            fd.seek(0)
            with self.assertRaises(TypeError):
                manifest_loader.validate_mandatory_keys(new_data)

    def test_validate_mandatory_keys3(self):
        """test_validate_mandatory_keys3
        Given a valid manifest json
        And the manifest program key is not 'one-time-update'
        When I call validate_mandatory_keys
        Then I get ValueError exception
        """
        data = json.load(self.valid_manifest)
        new_data = modify_manifest(data, to_super_rsu=True)
        with tempfile.NamedTemporaryFile(mode='w+', prefix='invalid_otsu-') as fd:
            json.dump(new_data, fd, indent=4)
            manifest_loader = fpgaotsu.otsu_manifest_loader(fd)
            fd.seek(0)
            with self.assertRaises(ValueError):
                manifest_loader.validate_mandatory_keys(new_data)


class test_other_functions(unittest.TestCase):
    def test_load_csv(self):
        """test_load_csv
        Given a manifest in csv format
        And the manifest references flash files that doesn exist
        When I call load_and_validate
        Then I get 'None' as the return variable
        """
        valid_manifest = tempfile.NamedTemporaryFile(mode='w+', prefix='otsu09C4-')
        valid_manifest.write(CSV_CFG)
        valid_manifest.seek(0)
        manifest_loader = fpgaotsu.otsu_manifest_loader(valid_manifest)
        cfg = manifest_loader.load_and_validate()
        self.assertIsNone(cfg)

    def test_all_or_none(self):
        """test_all_or_none
        Given an valid flash json/dict object
        When I call all_or_none with invalid keys args
        Then I get KeyError exception
        When I call all_or_none with valid keys args
        Then no exception is raised
        """
        obj = {"seek": "0x1800000",
               "start": "0x1800000",
               "type": "flash"}

        with self.assertRaises(KeyError):
            fpgaotsu.all_or_none(obj, 'filename', 'start')

        try:
            fpgaotsu.all_or_none(obj, 'type', 'start')
        except ExceptionType:
            self.fail("all_or_none raised ExceptionType unexpectedly")
