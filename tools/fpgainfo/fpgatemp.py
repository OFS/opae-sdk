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
import sysfs


def print_celcius(value):
    return u'{}\u00b0 C'.format(value)


class temp_command(cmd.fpga_command):
    name = "temp"
    thermal_props = [("temperature", print_celcius),
                     ("threshold1", print_celcius),
                     ("threshold1_policy", int),
                     ("threshold1_reached", bool),
                     ("threshold2", print_celcius),
                     ("threshold2_reached", bool),
                     ("threshold2_policy", int),
                     ("threshold_trip", print_celcius)]

    def run(self, args):
        info = sysfs.sysfsinfo()
        for fme in info.fme(**vars(args)):
            fme.thermal_mgmt.print_info("//****** THERMAL ******//",
                                        *[p[0] for p in self.thermal_props],
                                        **dict(self.thermal_props))


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    temp_command(parser)
    args = parser.parse_args()
    args.func(args)
