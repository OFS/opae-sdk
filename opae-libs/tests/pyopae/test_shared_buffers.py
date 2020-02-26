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
import struct
import sys

# pylint: disable=E0602, E0603

class TestSharedBuffer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.system = mockopae.test_system()
        cls.platform = mockopae.test_platform.get("skx-p")
        cls.system.initialize()
        cls.system.prepare_sysfs(cls.platform)
        opae.fpga.initialize(None)
        cls.props = opae.fpga.properties(type=opae.fpga.ACCELERATOR)
        cls.toks = opae.fpga.enumerate([cls.props])
        assert cls.toks
        cls.handle = opae.fpga.open(cls.toks[0])
        assert cls.handle

    @classmethod
    def tearDownClass(cls):
        cls.system.finalize()

    def test_allocate(self):
        buff1 = opae.fpga.allocate_shared_buffer(self.handle, 4096)
        buff2 = opae.fpga.allocate_shared_buffer(self.handle, 4096)
        assert buff1
        assert buff2
        assert buff1.size() == 4096
        assert buff1.wsid() != 0
        # TODO: look into wsid in new mock system
        # assert buff1.io_address() != 0
        mv = memoryview(buff1)
        assert mv
        assert not buff1.compare(buff2, 4096)
        buff1.fill(0xAA)
        buff2.fill(0xEE)
        assert buff1.compare(buff2, 4096)
        if sys.version_info[0] == 2:
            assert mv[0] == '\xaa'
            assert mv[-1] == '\xaa'
        else:
            assert mv[0] == 0xaa
            assert mv[-1] == 0xaa
        ba = bytearray(buff1)
        assert ba[0] == 0xaa
        buff1[42] = int(65536)
        assert struct.unpack('<L', (bytearray(buff1[42:46])))[0] == 65536

    def test_conext_release(self):
        assert self.handle
        self.handle.close()
        with opae.fpga.open(self.toks[0]) as h:
            buff = opae.fpga.allocate_shared_buffer(h, 4096)
            assert buff
            assert buff.size() == 4096
            assert buff.wsid() != 0
        assert not h
        assert buff.size() == 0
        assert buff.wsid() == 0

