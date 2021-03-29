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
from hssicommon import *


class FPGAHSSILPBK(HSSICOMMON):
    def __init__(self, args):
        self._loopback = args.loopback
        self._hssi_grps = args.hssi_grps
        self._pcie_address = args.pcie_address
        HSSICOMMON.__init__(self)

    def hssi_loopback_en(self):
        """
        clear ctl address and ctl sts CSR
        write 0x7/0x8 value ctl address
        write 0x2 value ctl sts csr
        poll for status
        clear ctl address and ctl sts CSR
        """
        print("eth_group_loopback_en", self._loopback)

        self.open(self._hssi_grps[0][0])

        ctl_addr = hssi_ctl_addr(0)
        if (self._loopback):
            ctl_addr.set_sal_cmd(HSSI_SALCMD.ENABLE_LOOPBACK.value)
        else:
            ctl_addr.set_sal_cmd(HSSI_SALCMD.DISABLE_LOOPBACK.value)

        cmd_sts = hssi_cmd_sts(0)
        cmd_sts.value = 0x2

        ret = self.clear_ctl_sts_reg(0)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            self.close()
            return 0

        self.write32(0, HSSI_CSR.HSSI_CTL_ADDRESS.value, ctl_addr.value)
        self.write32(0, HSSI_CSR.HSSI_CTL_STS.value, cmd_sts.value)

        if not self.read_poll_timeout(0,
                                      HSSI_CSR.HSSI_CTL_STS.value,
                                      0x2):
            print("HSSI ctl sts csr fails to update ACK")
            self.close()
            return 0

        ret = self.clear_ctl_sts_reg(0)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            self.close()
            return 0

        self.close()

        return 0

    def hssi_loopback_start(self):
        """
        print hssi info
        enable/disable hssi loopback
        """
        print("----hssi_loopback_start----")
        self.hssi_info(self._hssi_grps[0][0])
        self.hssi_loopback_en()


def main():
    """
    parse input arguemnts pciaddress and mtu
    enum fpga pcie devices and find match
    enable/disable loopback
    """

    parser = argparse.ArgumentParser()
    pcieaddress_help = 'bdf of device to program \
                       (e.g. 04:00.0 or 0000:04:00.0).' \
                       ' Optional when one device in system.'
    parser.add_argument('--pcie-address', '-P',
                        default=DEFAULT_BDF, help=pcieaddress_help)

    parser.add_argument('--loopback',
                        choices=['enable', 'disable'], nargs='?',
                        default=None,
                        help='loopback enable')

    args, left = parser.parse_known_args()

    print("args", args)
    print("pcie_address:", args.pcie_address)
    print("args.loopback:", args.loopback)
    print(args)
    if args.loopback is None:
        print('please specify --loopback enable/disable')
        sys.exit(1)
    else:
        op = 'enable' if args.loopback else 'disable'
        print('{} fpga loopback'.format(op))

    f = FpgaFinder(args.pcie_address)
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

    args.fpga_root = devs[0].get('path')
    args.hssi_grps = f.find_hssi_group(args.fpga_root)
    print("args.hssi_grps", args.hssi_grps)
    if len(args.hssi_grps) == 0:
        sys.exit(1)

    print("fpga uid dev:", args.hssi_grps[0][0])

    lp = FPGAHSSILPBK(args)
    lp.hssi_loopback_start()


if __name__ == "__main__":
    main()
