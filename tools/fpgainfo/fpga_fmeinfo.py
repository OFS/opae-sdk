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


class fme_command(fpga_common.fpga_command):

    name = "fme"

    def args(self, parser):
        pass

    def run(self, args):

        global FME_SYSFS_PATH

        FME_SYSFS_PATH = fpga_common.FME_DEVICE.format(
            socket_id=args.socket_id, fme_instance=0)
        # print(FME_SYSFS_INFO)
        self.print_fme_info(FME_SYSFS_PATH)

    def get_feature(self, feature_path):
        if fpga_common.fpga_command.fme_feature_is_supported(
                self,
                feature_path):
            with open(feature_path, "r") as fd:
                value = fd.read().strip().lower()
                return value
        else:
            print "Feature not supported: " + feature_path + "\n"

    def print_fme_info(self, fmepath):
        # RO | It contains FME Babric version.
        FME_SYSFS_FABRIC_VERSION = fmepath + "/version"
        # RO | It contains the number of PR ports.
        FME_SYSFS_PR_NUM = fmepath + "/ports_num"
        # RO | It contains FME Socket ID
        FME_SYSFS_SOCKET_ID = fmepath + "/socket_id"
        # RO | It contains FME bitstream ID
        FME_SYSFS_BITSTREAM_ID = fmepath + "/bitstream_id"
        # RO | It contains FME bitstream-metadata
        FME_SYSFS_BITSTREAM_METADATA = fmepath + "/bitstream_metadata"
        # RO | It contains FME PR interface ID
        FME_SYSFS_PR_INTER_ID = fmepath + "/pr/interface_id"

        feature = self.get_feature(FME_SYSFS_FABRIC_VERSION)
        if feature is not None:
            print "FME fabric version:" + feature + "\n"
        else:
            print "Feature not supported: " + FME_SYSFS_FABRIC_VERSION + "\n"

        feature = self.get_feature(FME_SYSFS_PR_NUM)
        if feature is not None:
            print "Number of PR ports: " + feature + "\n"
        else:
            print "Feature not supported: " + FME_SYSFS_PR_NUM + "\n"

        feature = self.get_feature(FME_SYSFS_SOCKET_ID)
        if feature is not None:
            print "FME Socket ID: " + feature + "\n"
        else:
            print "Feature not supported: " + FME_SYSFS_SOCKET_ID + "\n"

        feature = self.get_feature(FME_SYSFS_BITSTREAM_ID)
        if feature is not None:
            print "FME Bitstream ID: " + feature + "\n"
        else:
            print "Feature not supported: " + FME_SYSFS_BITSTREAM_ID + "\n"

        feature = self.get_feature(FME_SYSFS_BITSTREAM_METADATA)
        if feature is not None:
            print "FME Bitstream Metadata: " + feature + "\n"
        else:
            print("Feature not supported: " + FME_SYSFS_BITSTREAM_METADATA +
                  "\n")

        feature = self.get_feature(FME_SYSFS_PR_INTER_ID)
        if feature is not None:
            print "FME PR interface ID: " + feature + "\n"
        else:
            print "Feature not supported: " + FME_SYSFS_PR_INTER_ID + "\n"


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    fpga_common.global_arguments(parser)
    power_command(parser)
    args = parser.parse_args()
    args.func(args)
