#! /usr/bin/env python
# Copyright(c) 2018, Intel Corporation
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

from __future__ import print_function
import argparse
import time
import os
import fcntl
import struct
from common import exception_quit, FpgaFinder, PKVLCOMMON
from common import convert_argument_str2hex

stats_shift = 0x200


class PKVLSTATS(PKVLCOMMON):
    # statistics item : (dev, group register number, base)
    host_stats = {'tx_pkt_counter': (4, 3, 0xa01b),
                  'tx_byte_counter': (4, 3, 0xa01e),
                  'rx_pkt_counter': (4, 3, 0xa021),
                  'rx_byte_counter': (4, 3, 0xa024),
                  'rx_pkt_err_counter': (4, 3, 0xa027)}
    line_stats = {'tx_pkt_counter': (3, 3, 0xa01b),
                  'tx_byte_counter': (3, 3, 0xa01e),
                  'rx_pkt_counter': (3, 3, 0xa021),
                  'rx_byte_counter': (3, 3, 0xa024),
                  'rx_pkt_err_counter': (3, 3, 0xa027)}
    pkt_gen_ctl_1 = {'host': (4, 0xa010),
                     'line': (3, 0xa010)}
    counter_rst = 6
    pkt_gen_en = 1
    pkt_chk_en = 0

    def __init__(self, args):
        self.en = args.enable
        self.clr = args.clear
        self.pkvl_devs = args.pkvl_devs

    def print_port_stats(self, handler, stats, phy, dev, num, reg):
        print("{0: <30}".format(stats), end=' | ')
        for i in range(self.PKVL_PORT_NUMBER):
            value = 0
            for r in range(num):
                v = struct.pack(self.data_fmt, self.data_len, 0, dev,
                                reg + r + i * stats_shift, 0)
                data = self.ioctl(handler, self.PKVL_READ_PHY_REG, v)
                _, _, _, _, v = struct.unpack(self.data_fmt, data)
                value |= v << (r * self.PKVL_REG_WIDTH)
            print("{0: <12}".format(value), end=' | ')
        print()

    def print_pkvl_stats(self):
        for p in range(self.PKVL_PHY_NUMBER):
            with open(self.char_devs[p], 'rw') as f:
                print('=' * 100)
                print('PKVL PHY {}'.format(p))
                for side in ['host', 'line']:
                    print("{0: <30}".format(side), end=' | ')
                    for i in range(self.PKVL_PORT_NUMBER):
                        print('port {:<7}'.format(i), end=' | ')
                    print()
                    for stats, c in getattr(
                            self, '{}_stats'.format(side)).items():
                        dev, num, base = c
                        self.print_port_stats(f, stats, p, dev, num, base)

    def enable_statistics(self):
        value = 0
        value |= (1 << self.pkt_chk_en)
        value |= (1 << self.pkt_gen_en)
        for p in range(self.PKVL_PHY_NUMBER):
            for k, c in self.pkt_gen_ctl_1.items():
                for i in range(self.PKVL_PORT_NUMBER):
                    dev, reg = c
                    self.pkvl_phy_reg_set_field(p, dev, reg + i * stats_shift,
                                                self.pkt_chk_en, 2, value)
        print('PKVL Statistics counter enabled')

    def clear_statistics(self):
        for p in range(self.PKVL_PHY_NUMBER):
            for k, c in self.pkt_gen_ctl_1.items():
                for i in range(self.PKVL_PORT_NUMBER):
                    dev, reg = c
                    self.pkvl_phy_reg_set_field(p, dev, reg + i * stats_shift,
                                                self.counter_rst, 1, 1)
        print('PKVL Statistics counter clear')

    def start(self):
        self.char_devs = self.get_pkvl_char_device(self.pkvl_devs)
        if self.en:
            self.enable_statistics()
        elif self.clr:
            self.clear_statistics()
            self.enable_statistics()
        else:
            self.print_pkvl_stats()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bus', '-B',
                        help='Bus number of PCIe device')
    parser.add_argument('--device', '-D',
                        help='Device number of PCIe device')
    parser.add_argument('--function', '-F',
                        help='Function number of PCIe device')
    parser.add_argument('--enable',
                        action='store_true',
                        default=False,
                        help='PKVL Statistics enable')
    parser.add_argument('--clear',
                        action='store_true',
                        default=False,
                        help='PKVL Statistics clear')
    args, left = parser.parse_known_args()

    args = convert_argument_str2hex(args, ['bus', 'device', 'function'])
    f = FpgaFinder(args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        print('bus:{bus:#x} device:{dev:#x} function:{func:#x}'.format(**d))
    if len(devs) > 1:
        exception_quit('{} FPGAs are found\nplease choose '
                       'one FPGA'.format(len(devs)))
    if not devs:
        exception_quit('no FPGA found')
    args.pkvl_devs = f.find_node(
        devs[0].get('path'),
        'misc/pkvl*/dev',
        depth=8)
    if not args.pkvl_devs:
        exception_quit('No pkvl device found')
    for dev in args.pkvl_devs:
        print('pkvl device: {}'.format(dev))

    ps = PKVLSTATS(args)
    ps.start()
    print('Done')


if __name__ == "__main__":
    main()
