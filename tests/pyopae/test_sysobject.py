# Copyright(c) 2018, Intel Corporation
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
import sys
import uuid

# pylint: disable=E0602, E0603

class TestSysObject(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.system = mockopae.test_system()
        cls.platform = mockopae.test_platform.get("dfl-n3000")
        cls.system.initialize()
        cls.system.prepare_sysfs(cls.platform)
        opae.fpga.initialize(None)
        props = opae.fpga.properties(type=opae.fpga.ACCELERATOR)
        cls.afu_toks = opae.fpga.enumerate([props])
        assert cls.afu_toks
        cls.afu_handle = opae.fpga.open(cls.afu_toks[0])
        assert cls.afu_handle
        props = opae.fpga.properties(type=opae.fpga.DEVICE)
        cls.fme_toks = opae.fpga.enumerate([props])
        assert cls.fme_toks
        cls.fme_handle = opae.fpga.open(cls.fme_toks[0], opae.fpga.OPEN_SHARED)
        assert cls.fme_handle

    @classmethod
    def tearDownClass(cls):
        cls.system.finalize()

    def test_token_object(self):
        afuid = self.afu_toks[0].afu_id.bytes()[:-1]
        afu_guid = self.platform.devices[0].afu_guid
        u1 = uuid.UUID(afuid)
        u2 = uuid.UUID(afu_guid)
        assert u1 == u2
        afuid2 = self.afu_toks[0]["afu_id"].bytes()[:-1]
        assert afuid == afuid2
        self.assertEquals(self.afu_toks[0].wrong, None)
        with self.assertRaises(RuntimeError):
            print(self.afu_toks[0]['../fpga'].read64())

    def test_handle_object(self):
        afuid = self.afu_handle.afu_id.bytes()[:-1]
        afu_guid = self.platform.devices[0].afu_guid
        u1 = uuid.UUID(afuid)
        u2 = uuid.UUID(afu_guid)
        assert u1 == u2

    def test_object_object(self):
        pr = self.fme_handle.find('dfl-fme-region.*/fpga_region/region*')
        compat_id = pr.compat_id
        u1 = uuid.UUID(compat_id.bytes()[:-1])
        u2 = uuid.UUID(self.platform.devices[0].fme_guid)
        assert u1 == u2

    def test_object_read64(self):
        errors = self.afu_handle.errors
        err = errors.errors
        n1 = err.read64()
        self.assertEquals(n1, 0)
        with self.assertRaises(RuntimeError):
            print(errors.read64())


