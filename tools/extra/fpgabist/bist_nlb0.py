#!/usr/bin/env python
# Copyright(c) 2018, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
# may be used to  endorse or promote  products derived  from this  software
# without specific prior written permission.
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
import subprocess

import bist_common as bc

afu_clk_freqs = {bc.VCP_ID: 200000000}


class Nlb0Mode(bc.BistMode):
    name = "nlb_0"

    def __init__(self):
        channel = [('vh0', 'vh0'), ('vh1', 'vh1'),
                   ('vh0', 'vh1'), ('vh1', 'vh0')]
        params = ('--mode=lpbk1 --read-vc={rvc} --write-vc={wvc} '
                  '--multi-cl=4 --begin=1024 --end=1024 --timeout-sec=1 '
                  '--cont')
        self.executables = {'-'.join(ch): params.format(rvc=ch[0], wvc=ch[1])
                            for ch in channel}

    def run(self, path, bdf, bd_id=0, guid=''):
        tp = self.executables.items()
        tp.sort()
        ret = 0
        for test, param in tp:
            print "Running fpgadiag lpbk1 {} test...".format(test)
            cmd = "fpgadiag -B {} -D {} -F {} {}".format(hex(bdf['bus']),
                                                         hex(bdf['device']),
                                                         hex(bdf['function']),
                                                         param)
            if guid:
                cmd += ' -G {}'.format(guid)
            if bd_id != 0:
                cmd += ' -T {}'.format(afu_clk_freqs.get(bd_id, 400000000))
            try:
                subprocess.check_call(cmd.split(' '))
            except subprocess.CalledProcessError as e:
                print "Failed Test: {}".format(test)
                print e
                ret += 1
        print "Finished Executing NLB (FPGA DIAG) Tests\n"
        return ret
