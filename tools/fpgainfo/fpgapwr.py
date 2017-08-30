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
import os

# RO | It contains power management configure files.
FME_SYSFS_POWER_MGMT = os.path.join(fpga_common.FME_DEVICE, "power_mgmt")

# RO | PCU will write the FPGA power consumption value
FME_SYSFS_POWER_MGMT_CONSUMED = None

# RW | Get/set AP threshold 1
FME_SYSFS_POWER_MGMT_THRESHOLD1 = None

# RO | Indicates if the power exceeds Threshold 2.  Will trigger AP2 state.
FME_SYSFS_POWER_MGMT_THRESHOLD1_STATUS = None

# RO | Indicates if the power exceeds Threshold 1.  Will trigger AP1 state.
FME_SYSFS_POWER_MGMT_THRESHOLD2 = None

# RW | Get/set AP threshold 2
FME_SYSFS_POWER_MGMT_THRESHOLD2_STATUS = None

# RO | Indicates if the power exceeds Threshold 2.  Will trigger AP2 state.
RTL = None


class power_command(fpga_common.fpga_command):

    name = "power"

    def args(self, parser):
        pass

    def run(self, args):
        global FME_SYSFS_POWER_MGMT_CONSUMED
        powerpath = FME_SYSFS_POWER_MGMT.format(
            socket_id=args.socket_id, fme_instance=0)
        FME_SYSFS_POWER_MGMT_CONSUMED = os.path.join(
            powerpath, "consumed")
        self.get_power_consumed(FME_SYSFS_POWER_MGMT_CONSUMED)

    def get_power_consumed(self, powerpath):
        if self.fme_feature_is_supported(powerpath):
            with open(powerpath, "r") as fd:
                value = fd.read()
            watts = int(value, 16)
            print "Power consumption value in watts:  {}\n".format(watts)
        else:
            print("Feature not supported:")
            print powerpath


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    fpga_common.global_arguments(parser)
    power_command(parser)
    args = parser.parse_args()
    args.func(args)
