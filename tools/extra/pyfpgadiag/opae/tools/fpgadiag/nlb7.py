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
from diagtest import diagtest
from opae.utils import CACHELINE_BYTES
# pylint: disable=E0611
from opae import fpga

HIGH = 0xffffffff
LOW = 0x0


class nlb7(diagtest):
    guid = "7BAF4DEA-A57C-E91E-168A-455D9BDA88A3"

    SW_NOTICE = 0x0158
    NOTICE_VALUE = 0x10101010

    def setup(self):
        if super(nlb7, self).setup():
            if "usmg" in self.args.notice:
                self.logger.error("Not implemented")
                return False
            return True
        return False

    def add_arguments(self, parser):
        super(nlb7, self).add_arguments(parser)
        parser.add_argument(
            '-N',
            '--notice',
            choices=[
                'poll',
                'csr-write',
                'umsg-data',
                'umsg-hint'],
            default='poll',
            help='Mechanism to get notice from fpga')

    def configure_test(self):
        super(nlb7, self).configure_test()
        if self.args.notice == "csr-write":
            self.cfg |= (1 << 26)

    def buffer_size(self):
        return (self.args.end+1)*CACHELINE_BYTES

    def setup_buffers(self, handle, dsm, src, dst):
        dst.fill(0)
        src.fill(0xdeca)
        # TODO: set umsg mas when it is implemented
        # fpga.umsg_set_mask(handle, LOW) # Not a real API

    def test_buffers(self, handle, cachelines, dsm, src, dst):
        self.logger.debug("test_buffers")
        sz = cachelines*CACHELINE_BYTES
        # test flow
        # 1. CPU polls on address N+1
        self.logger.debug("poll_buffers: {} {}".format(dst.size(), sz))
        if not dst.poll32(sz, HIGH):
            raise RuntimeError(
                "Polling failed for buffer at offset {}".format(sz))

        # 2. CPU copies dest to src
        self.logger.debug("copying dst to src: {}".format(sz))
        dst.copy(src, sz)
        self.logger.debug("dst copied {} bytes to src".format(sz))

        # fence operation
        fpga.memory_barrier()
        if self.args.notice == "csr-write":
            self.write_csr32(handle, self.SW_NOTICE, self.NOTICE_VALUE)
        else:  # poll
            src.write32(HIGH, sz)
            src.write32(HIGH, sz+4)
            src.write32(HIGH, sz+8)

        self.wait_for_dsm(dsm)
