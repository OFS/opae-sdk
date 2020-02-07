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

import os
import subprocess

import bist_common as bc


class DefaultMode(bc.BistMode):
    name = "BIST"
    afu_id = "f7df405c-bd7a-cf72-22f1-44b0b93acd18"

    def __init__(self):
        self.executables = {'bist': ''}

    def run(self, gbs_path, bdf):
        if gbs_path:
            bc.load_gbs(gbs_path, bdf)
        ret = 0
        for func, param in self.executables.items():
            print "Running Built-in Self test...\n"
            cmd = [func, '-B', hex(bdf['bus']),
                   '-D', hex(bdf['device']),
                   '-F', hex(bdf['function'])]
            cmd.extend(param.split())
            try:
                subprocess.check_call(cmd)
            except subprocess.CalledProcessError as e:
                print "Failed Test: {}".format(func)
                print e
                ret += 1
        print "Finished Executing BIST\n"
        return ret
