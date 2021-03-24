#! /usr/bin/env python3
# Copyright(c) 2018-2021, Intel Corporation
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


class FPGAHSSISTATS(HSSICOMMON):
    hssi_eth_stats = (('tx_packets', 0),
                      ('rx_packets', 1),
                      ('rx_crc_errors', 2),
                      ('rx_align_errors', 3),
                      ('tx_bytes', 4),
                      ('rx_bytes', 5),
                      ('tx_pause', 6),
                      ('rx_pause', 7),
                      ('rx_errors', 8),
                      ('tx_errors', 9),
                      ('rx_unicast', 10),
                      ('rx_multicast', 11),
                      ('rx_broadcast', 12),
                      ('tx_discards', 13),
                      ('tx_unicast', 14),
                      ('tx_multicast', 15),
                      ('tx_broadcast', 16),
                      ('ether_drops', 18),
                      ('rx_total_packets', 19),
                      ('rx_undersize', 20),
                      ('rx_oversize', 21),
                      ('rx_64_bytes', 22),
                      ('rx_65_127_bytes', 23),
                      ('rx_128_255_bytes', 24),
                      ('rx_256_511_bytes', 25),
                      ('rx_512_1023_bytes', 26),
                      ('rx_1024_1518_bytes', 27),
                      ('rx_gte_1519_bytes', 28),
                      ('rx_jabbers', 29),
                      ('rx_runts', 30))

    def __init__(self, args):
        self.pcieaddress = args.pcieaddress
        self.hssi_grps = args.hssi_grps
        HSSICOMMON.__init__(self)

    def get_hssi_stats(self):
        """
        clear ctl address and ctl sts CSR
        write 0x3 value ctl address and address bit
        write read cmd 0x1 value ctl sts csr
        poll for status
        read LSB stats
        clear ctl address and ctl sts CSR
        write 0x3 value ctl address , address bit  and set 31 bit
        write read 0x1 value ctl sts csr
        poll for status
        read MSB stats
        print stats
        """
        self.open(self.hssi_grps[0][0])
        ctl_addr = hssi_ctl_addr(0)
        ctl_addr.sal_cmd(HSSI_SALCMD.READ_MAC_STATISTIC.value)
        value = 0
        print("------------HSSI stats------------")
        for str, reg in self.hssi_eth_stats:

            # Read LSB value
            ctl_addr.addressbit(reg)
            value_lsb = self.read_reg(0, ctl_addr.value)

            # Read MSB value
            ctl_addr.value = self.register_field_set(ctl_addr.value, 31, 1, 1)
            value_msb = self.read_reg(0, ctl_addr.value)

            # 64 bit value
            value = (value_msb << 32) | (value_lsb)

            print("{0: <20}".format(str), end=' | ')
            print("{0: >12}".format(value), end=' | ')
            print()

        self.close()

    def hssi_stats_start(self):
        """
        print hssi info
        get hssi stats
        """
        print("----hssi_stats_start----")
        self.hssi_info(self.hssi_grps[0][0])
        self.get_hssi_stats()


def main():
    """
    parse input arguemnts pciaddress and mtu
    enum fpga pcie devices and find match
    read hssi statistics
    """
    parser = argparse.ArgumentParser()

    pcieaddress_help = 'bdf of device to program \
                        (e.g. 04:00.0 or 0000:04:00.0).' \
                       ' Optional when one device in system.'
    parser.add_argument('--pcieaddress', '-P',
                        default=None, help=pcieaddress_help)

    args, left = parser.parse_known_args()

    print(args)
    print("pcieaddress:", args.pcieaddress)

    f = FpgaFinder(args.pcieaddress)
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
    print("devs[0]", devs[0])
    args.hssi_grps = f.find_hssi_group(args.fpga_root)
    print("args.hssi_grps", args.hssi_grps)
    if len(args.hssi_grps) == 0:
        sys.exit(1)

    print("fpga uid dev:", args.hssi_grps[0][0])
    lp = FPGAHSSISTATS(args)
    lp.hssi_stats_start()


if __name__ == "__main__":
    main()
