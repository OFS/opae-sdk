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

import fpga_common as cmd
import os
import sys

# RW | It contains thermal management configure files
FME_SYSFS_THERMAL_MGMT = os.path.join(cmd.FME_DEVICE, "thermal_mgmt")

# RO | Reads out FPGA temperature in Centigrade
FME_SYSFS_THERMAL_MGMT_TEMP = None

# RW | Get/set thermal threshold, write values: 7-bit unsigned integer,
# 0 - disable threshold1
FME_SYSFS_THERMAL_MGMT_THRESHOLD1 = None

# RW | Get/set thermal threshold2, write values: 7-bit unsigned integer, 0
# - disable threshold2
FME_SYSFS_THERMAL_MGMT_THRESHOLD2 = None

# RO | Check if Temperature reaches threshold1, 0 - not; 1 - yes
FME_SYSFS_THERMAL_MGMT_THRESHOLD1_REACHED = None

# RO | Check if Temperature reaches threshold2, 0 - not; 1 - yes
FME_SYSFS_THERMAL_MGMT_THRESHOLD2_REACHED = None

# RW | Get/set the policy of threshold1
# 0 - AP2 state (90% throttle) 1 - AP1 state (50% throttle)
# other values fail with -EINVAL
FME_SYSFS_THERMAL_MGMT_THRESHOLD1_POLICY = None

# RO | Get Therm Trip Threshold
FME_SYSFS_THERMAL_MGMT_THRESHOLD_TRIP = None


class temp_command(cmd.fpga_command):

    name = "temp"
    output_message = None

    def args(self, parser):
        pass

    def run(self, args):
        global FME_SYSFS_THERMAL_MGMT
        FME_SYSFS_THERMAL_MGMT = FME_SYSFS_THERMAL_MGMT.format(
            socket_id=args.socket_id, fme_instance=0)
        # print(FME_SYSFS_THERMAL_MGMT)
        self.get_temp(FME_SYSFS_THERMAL_MGMT)

    def get_temp(self, temppath):
        global FME_SYSFS_THERMAL_MGMT_TEMP
        global FME_SYSFS_THERMAL_MGMT_THRESHOLD1
        global FME_SYSFS_THERMAL_MGMT_THRESHOLD1_POLICY
        global FME_SYSFS_THERMAL_MGMT_THRESHOLD1_REACHED
        global FME_SYSFS_THERMAL_MGMT_THRESHOLD2
        global FME_SYSFS_THERMAL_MGMT_THRESHOLD2_REACHED
        global FME_SYSFS_THERMAL_MGMT_THRESHOLD_TRIP

        if os.path.isdir(temppath):
            print("Thermal management is supported, getting values ...")
        else:
            print("Thermal management is NOT supported!")
            return

        FME_SYSFS_THERMAL_MGMT_TEMP = FME_SYSFS_THERMAL_MGMT + "/temperature"
        self.output_message = "FPGA thermal temperature"
        ftr = self.get_feature(FME_SYSFS_THERMAL_MGMT_TEMP)
        if ftr is not None:
            ftr = ":  " + ftr + u"\u00b0" + " Centigrade"
            self.printValue(ftr)
        else:
            print "Feature not supported:  temperature"

        FME_SYSFS_THERMAL_MGMT_THRESHOLD1 = \
            FME_SYSFS_THERMAL_MGMT + "/threshold1"
        self.output_message = "FPGA thermal threshold 1"
        ftr = self.get_feature(FME_SYSFS_THERMAL_MGMT_THRESHOLD1)
        if ftr is not None:
            if ftr == "0":
                ftr = ":  " + ftr + "disabled"
            else:
                ftr = ":  " + ftr + u"\u00b0" + " Centigrade"
            self.printValue(ftr)
        else:
            print "Feature not supported:  threshold 1"

        FME_SYSFS_THERMAL_MGMT_THRESHOLD1_POLICY = \
            FME_SYSFS_THERMAL_MGMT + "/threshold1_policy"
        self.output_message = "FPGA thermal threshold 1 policy"
        ftr = self.get_feature(FME_SYSFS_THERMAL_MGMT_THRESHOLD1_POLICY)
        if ftr is not None:
            if ftr == "0":
                ftr = ":  " + ftr + " (AP2 state (90% throttle))"
            elif ftr == "1":
                ftr = ":  " + ftr + " (AP1 state (50% throttle))"
            else:
                ftr = ":  " + ftr + "error"
            self.printValue(ftr)
        else:
            print "Feature not supported:  threshold 1"

        FME_SYSFS_THERMAL_MGMT_THRESHOLD1_REACHED = \
            FME_SYSFS_THERMAL_MGMT + "/threshold1_reached"
        self.output_message = "FPGA thermal threshold 1 reached"
        ftr = self.get_feature(FME_SYSFS_THERMAL_MGMT_THRESHOLD1_REACHED)
        if ftr is not None:
            if ftr == "0":
                ftr = ":  " + ftr + " (false)"
            else:
                ftr = ":  " + ftr + " (true)"
            self.printValue(ftr)
        else:
            print "Feature not supported:  threshold 1 reached"

        FME_SYSFS_THERMAL_MGMT_THRESHOLD2 = \
            FME_SYSFS_THERMAL_MGMT + "/threshold2"
        self.output_message = "FPGA thermal threshold 2"
        ftr = self.get_feature(FME_SYSFS_THERMAL_MGMT_THRESHOLD2)
        if ftr is not None:
            if ftr == "0":
                ftr = ":  " + ftr + " (disabled)"
            else:
                ftr = ":  " + ftr + u"\u00b0" + " Centigrade"
            self.printValue(ftr)
        else:
            print "Feature not supported:  threshold 2"

        FME_SYSFS_THERMAL_MGMT_THESHOLD2_REACHED = \
            FME_SYSFS_THERMAL_MGMT + "/threshold2_reached"
        self.output_message = "FPGA thermal threshold 2 reached"
        ftr = self.get_feature(FME_SYSFS_THERMAL_MGMT_THESHOLD2_REACHED)
        if ftr is not None:
            if ftr == "0":
                ftr = ":  " + ftr + " (false)"
            else:
                ftr = ":  " + ftr + " (true)"
            self.printValue(ftr)
        else:
            print "Feature not supported:  threshold 2 reached"

        FME_SYSFS_THERMAL_MGMT_THRESHOLD_TRIP = \
            FME_SYSFS_THERMAL_MGMT + "/threshold_trip"
        self.output_message = "FPGA thermal threshold trip"
        ftr = self.get_feature(FME_SYSFS_THERMAL_MGMT_THRESHOLD_TRIP)
        if ftr is not None:
            ftr = ":  " + ftr + u"\u00b0" + " Centigrade"
            self.printValue(ftr)
        else:
            print "Feature not supported:  threshold trip"

    def get_feature(self, feature_path):
        if cmd.fpga_command.fme_feature_is_supported(
                self,
                feature_path):
            with open(feature_path, "r") as fd:
                value = fd.read().strip().lower()
	        radix = 16 if value.startswith('0x') else 10
            return str(int(value, radix))

    def printValue(self, ftr):
        line_length = 40
        padding = line_length - len(self.output_message)
        sys.stdout.write(" " * padding)
        print self.output_message + ftr


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    cmd.global_arguments(parser)
    temp_command(parser)
    args = parser.parse_args()
    args.func(args)
