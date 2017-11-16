#!/usr/bin/env python
# Copyright(c) 2017, Intel Corporation
##
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
##
# * Redistributions of  source code  must retain the  above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
# may be used to  endorse or promote  products derived  from this  software
# without specific prior written permission.
##
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

import fpga_common
from fpgapwr import power_command
import os


class port_command(fpga_common.fpga_command):

    name = "port"

    def args(self, parser):
        pass

    def run(self, args):

        global PORT_SYSFS_PATH

        PORT_SYSFS_PATH = fpga_common.PORT_DEVICE.format(
            socket_id=args.socket_id, fme_instance=0)
        # print(FME_SYSFS_INFO)
        self.print_fme_info(PORT_SYSFS_PATH)

    def print_fme_info(self, portpath):
        # RO | It contains FME Babric version.
        PORT_SYSFS_AFU_ID = portpath + "/afu_id"

        if self.fme_feature_is_supported(PORT_SYSFS_AFU_ID):
            with open(PORT_SYSFS_AFU_ID, "r") as fd:
                value = fd.read()
            print "Port AFU ID:  0x{}\n".format(value)
        else:
            print("Feature not supported:")
            print PORT_SYSFS_AFU_ID


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    fpga_common.global_arguments(parser)
    power_command(parser)
    args = parser.parse_args()
    args.func(args)
