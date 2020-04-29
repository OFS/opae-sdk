#!/usr/bin/env python3
# Copyright(c) 2017, Intel Corporation
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

from __future__ import absolute_import
from __future__ import print_function
import os
import subprocess

import bist_common as bc

dma_list = {bc.VCP_ID: {0: ('DDR4_A', 4*1024*1024*1024),
                        1: ('DDR4_B', 4*1024*1024*1024),
                        2: ('DDR4_C', 1*1024*1024*1024),
                        3: ('QDR', 16*1024*1024)}
            }


class DmaMode(bc.BistMode):
    name = "dma_afu"
    mode_list = {0x09c4: "331db30c-9885-41ea-9081-f88b8f655caa",
                 0x0b30: "331db30c-9885-41ea-9081-f88b8f655caa",
                 0x0b2b: "331db30c-9885-41ea-9081-f88b8f655caa"}

    def __init__(self):
        # Specify the test(s) for a given architecture or the default (0)
        self.executables = \
            {0:
             {'fpga_dma_test': ' -s 1073741824 -p 1048576 -r mtom'},
             bc.VCP_ID:
             {'fpga_dma_vc_test': '0x0b30'}
             }

    def run_cmd(self, cmd):
        ret = 0
        try:
            subprocess.check_call(cmd.split(' '))
        except subprocess.CalledProcessError as e:
            print("Failed Test: {}".format(cmd))
            print(e)
            ret = 1
        return ret

    def run(self, gbs_path, bdf, bd_id=0, guid=''):
        if gbs_path:
            bc.load_gbs(gbs_path, bdf)

        # Does a set of tests exist for the requested bd_id?
        if bd_id not in self.executables:
            print("No DMA tests for bd_id {}".format(bd_id))
            return 1

        ret = 0
        self.dev_id = bd_id
        # Pick the set of tests to run based on bd_id
        for func, param in self.executables[bd_id].items():
            if bd_id in dma_list:
                for i, c in dma_list[bd_id].items():
                    name, size = c
                    if self.dev_id != int(param, 16):
                        continue
                    cmd = "{} {} -B {} -D {} -S {}".format(func, param,
                                                           bdf['bus'],
                                                           i, size)
                    if guid:
                        cmd += ' -G {}'.format(guid)
                    print("Running {} test on {}...\n".format(func, name))
                    ret += self.run_cmd(cmd)
            else:
                print("Running {} test...\n".format(func))
                cmd = [func, '-B', hex(bdf['bus']),
                       '-D', hex(bdf['device']),
                       '-F', hex(bdf['function'])]
                cmd.extend(param.split())
                print("  " + ' '.join(cmd) + '\n')
                try:
                    subprocess.check_call(cmd)
                except subprocess.CalledProcessError as e:
                    print("Failed Test: {}".format(func))
                    print(e)
                    ret += 1

        print("Finished Executing DMA Tests\n")
        return ret
