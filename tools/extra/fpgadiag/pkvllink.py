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

status_shift = 0x200


class PKVLSTATS(PKVLCOMMON):
    # PCS status item : (dev, bit index, base)
    pcs_status = {'line': (3, 2, 0xa002),
                  'host': (4, 2, 0xa002)}

    def __init__(self, args):
        self.pkvl_devs = args.pkvl_devs

    def print_port_link_status(self, handler, side, phy, dev, idx, reg):
        print("{0: <30}".format(side + ' side'), end=' | ')
        for i in range(self.PKVL_PORT_NUMBER):
            v = struct.pack(self.data_fmt, self.data_len, 0,
                            dev, reg + i * status_shift, 0)
            data = self.ioctl(handler, self.PKVL_READ_PHY_REG, v)
            _, _, _, _, v = struct.unpack(self.data_fmt, data)
            status = ('up' if v & (1 << idx) else 'down')
            print("{0: <12}".format(status), end=' | ')
        print()

    def print_pkvl_link_status(self):
        for p in range(self.PKVL_PHY_NUMBER):
            with open(self.char_devs[p], 'rw') as f:
                print('=' * 100)
                print('PKVL PHY {}'.format(p))
                print("{0: <30}".format('side'), end=' | ')
                for i in range(self.PKVL_PORT_NUMBER):
                    print('port {:<7}'.format(i), end=' | ')
                print()
                for side, c in self.pcs_status.items():
                    dev, idx, base = c
                    self.print_port_link_status(f, side, p, dev, idx, base)

    def start(self):
        self.get_pkvl_char_device(self.pkvl_devs)
        self.print_pkvl_link_status()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bus', '-B',
                        help='Bus number of PCIe device')
    parser.add_argument('--device', '-D',
                        help='Device number of PCIe device')
    parser.add_argument('--function', '-F',
                        help='Function number of PCIe device')
    args, left = parser.parse_known_args()

    args = convert_argument_str2hex(args, ['bus', 'device', 'function'])
    f = FpgaFinder(args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        print('bus:0x{bus:x} device:0x{dev:x} function:0x{func:x}'.format(**d))
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
