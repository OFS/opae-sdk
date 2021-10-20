#! /usr/bin/env python3
# Copyright(c) 2021, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#  this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#  this list of conditions and the following disclaimer in the documentation
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

import re
import os
import glob
import argparse
import sys
import traceback
import fcntl
import stat
import struct
import mmap
from ethernet.hssicommon import *


class FPGAHSSILPBK(HSSICOMMON):
    def __init__(self, args):
        self._loopback = args.loopback
        self._hssi_grps = args.hssi_grps
        self._pcie_address = args.pcie_address
        self._port = args.port
        HSSICOMMON.__init__(self)

    def hssi_loopback_en(self):
        """
        clear ctl address and ctl sts CSR
        write 0x7/0x8 value ctl address
        write 0x2 value ctl sts csr
        poll for status
        clear ctl address and ctl sts CSR
        """
        self.open(self._hssi_grps[0][0])

        hssi_feature_list = hssi_feature(self.read32(0, 0xC))
        if (self._port >= HSSI_PORT_COUNT):
            print("Invalid Input port number")
            self.close()
            return False

        enable = self.register_field_get(hssi_feature_list.port_enable,
                                         self._port)
        if enable == 0:
            print("Input port is not enabled or active")
            self.close()
            return False

        ctl_addr = hssi_ctl_addr(0)
        if self._loopback == 'enable':
            ctl_addr.sal_cmd = HSSI_SALCMD.ENABLE_LOOPBACK.value
        else:
            ctl_addr.sal_cmd = HSSI_SALCMD.DISABLE_LOOPBACK.value

        # set port number
        ctl_addr.port_address = self._port

        cmd_sts = hssi_cmd_sts(0)
        cmd_sts.value = 0x2

        ret = self.clear_ctl_sts_reg(0)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            self.close()
            return False

        self.write32(0, HSSI_CSR.HSSI_CTL_ADDRESS.value, ctl_addr.value)
        self.write32(0, HSSI_CSR.HSSI_CTL_STS.value, cmd_sts.value)

        if not self.read_poll_timeout(0,
                                      HSSI_CSR.HSSI_CTL_STS.value,
                                      0x2):
            print("HSSI ctl sts csr fails to update ACK")
            self.close()
            return False

        ret = self.clear_ctl_sts_reg(0)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            self.close()
            return False

        self.close()
        return True

    def hssi_loopback_start(self):
        """
        print hssi info
        enable/disable hssi loopback
        """
        if not self.hssi_info(self._hssi_grps[0][0]):
            print("Failed to read hssi information")
            return False
        if not self.hssi_loopback_en():
            return False
        return True


def main():
    """
    parse input arguemnts pciaddress and mtu
    enum fpga pcie devices and find match
    enable/disable loopback
    """

    parser = argparse.ArgumentParser()
    pcieaddress_help = 'sbdf of device to program \
                       (e.g. 0000:04:00.0).' \
                       ' Optional when one device in system.'
    parser.add_argument('--pcie-address', '-P',
                        default=None, help=pcieaddress_help)

    parser.add_argument('--loopback',
                        choices=['enable', 'disable'], nargs='?',
                        default=None,
                        help='loopback enable')

    parser.add_argument('--port', type=int,
                        default=0,
                        help='hssi port number')

    # exit if no commad line argument
    args = parser.parse_args()
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    args, left = parser.parse_known_args()

    print("args", args)

    if not verify_pcie_address(args.pcie_address.lower()):
        sys.exit(1)

    if args.loopback is None:
        print('please specify --loopback enable/disable')
        sys.exit(1)

    f = FpgaFinder(args.pcie_address.lower())
    devs = f.enum()
    for d in devs:
        print('sbdf: {segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**d))
        print('FPGA dev:', d)
    if len(devs) > 1:
        print('{} FPGAs are found\nplease choose '
              'one FPGA'.format(len(devs)))
        sys.exit(1)
    if not devs:
        print('no FPGA found')
        sys.exit(1)

    args.hssi_grps = f.find_hssi_group(devs[0].get('pcie_address'))
    print("args.hssi_grps", args.hssi_grps)
    if len(args.hssi_grps) == 0:
        print("Failed to find HSSI feature", devs[0].get('pcie_address'))
        sys.exit(1)

    print("fpga uio dev:", args.hssi_grps[0][0])

    lp = FPGAHSSILPBK(args)
    if not lp.hssi_loopback_start():
        print("Failed to enable/disable loopback")
        sys.exit(1)

    if args.loopback == 'enable':
        print("hssi loopback enabled to port", args.port)
    else:
        print("hssi loopback disabled to port", args.port)


if __name__ == "__main__":
    main()
