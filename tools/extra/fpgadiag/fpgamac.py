#! /usr/bin/env python3
# Copyright(c) 2019, Intel Corporation
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
from __future__ import absolute_import
import argparse
import time
import struct
import os
import sys
import eth_group
from eth_group import *
from common import FpgaFinder, exception_quit, COMMON, hexint

FPGA_MAC_GROUP_ID = {'host': 1, 'line': 0}

MTU_REG_OFFSET = {
    10: {
        'tx': 0x002C,
        'rx': 0x00AE
    },
    25: {
        'tx': 0x407,
        'rx': 0x506
    },
    40: {
        'tx': 0x407,
        'rx': 0x506
    }
}


class FPGAMAC(COMMON):
    ports = []
    mac_number = 0
    mac_group_dev = ''
    speed = 10

    def __init__(self, args):
        try:
            if args.mtu is None or args.mtu == '':
                self.mtu = args.mtu
            else:
                self.mtu = int(args.mtu, 0)
        except ValueError as e:
            exception_quit('invalid mtu value {}'.format(args.mtu), 4)
        self.side = args.side
        self.argport = args.port
        self.direction = args.direction
        self.eth_grps = args.eth_grps

    def check_args(self):
        if isinstance(self.mtu, int) and (self.mtu < 1 or self.mtu > 65535):
            s = 'mtu size {} is out of range 1~65535'.format(self.mtu)
            exception_quit(s, 5)
        if self.speed not in MTU_REG_OFFSET:
            exception_quit('unknown speed {}'.format(self.speed), 6)

    def fpga_mac_reg_write(self, f, mac, reg, value):
        self.fpga_eth_reg_write(f, 'mac', mac, reg, value)

    def fpga_mac_reg_read(self, f, mac, reg):
        return self.fpga_eth_reg_read(f, 'mac', mac, reg)

    def fpga_mac_mtu(self):
        with open(self.mac_group_dev, 'w') as f:
            if self.mtu == '':
                tab_len = 64 if self.direction == 'both' else 45
                print(''.center(tab_len, '='))
                print('maximum frame length'.ljust(24, ' '), end=' | ')
                if self.direction in ['tx', 'both']:
                    print('transmit'.ljust(16), end=' | ')
                if self.direction in ['rx', 'both']:
                    print('receive'.ljust(16), end=' | ')
                print()
                for i in self.ports:
                    print("mac {}".format(i).ljust(24), end=' | ')
                    if self.direction in ['tx', 'both']:
                        offset = MTU_REG_OFFSET[self.speed]['tx']
                        v = self.fpga_mac_reg_read(f, i, offset)
                        print('{:<16}'.format(v), end=' | ')
                    if self.direction in ['rx', 'both']:
                        offset = MTU_REG_OFFSET[self.speed]['rx']
                        v = self.fpga_mac_reg_read(f, i, offset)
                        print('{:<16}'.format(v), end=' | ')
                    print()
            else:
                for i in self.ports:
                    if self.direction in ['tx', 'both']:
                        offset = MTU_REG_OFFSET[self.speed]['tx']
                        self.fpga_mac_reg_write(f, i, offset, self.mtu)
                    if self.direction in ['rx', 'both']:
                        offset = MTU_REG_OFFSET[self.speed]['rx']
                        self.fpga_mac_reg_write(f, i, offset, self.mtu)
                print("Done")

    def get_port_group_info(self):
        info = self.get_eth_group_info(self.eth_grps)
        for mac_grp in info:
            if mac_grp == FPGA_MAC_GROUP_ID[self.side]:
                num, _, spd, node = info[mac_grp]
                self.mac_grp = mac_grp
                self.mac_number = num
                self.mac_group_dev = node
                self.speed = spd

    def start(self):
        self.get_port_group_info()
        self.check_args()
        self.ports = self.get_port_list(self.argport, self.mac_number)
        if self.mtu is None:
            print('Please set configruation type of MAC, such as "--mtu"')
        else:
            self.fpga_mac_mtu()

    def eth_group_mac_reg_write(self, eth_group, mac, reg, value):
        self.eth_group_reg_write(eth_group, 'mac', mac, reg, value)

    def eth_group_mac_reg_read(self, eth_group, mac, reg):
        return self.eth_group_reg_read(eth_group, 'mac', mac, reg)

    def eth_group_mac_mtu(self):
        for keys, values in self.eth_grps.items():
            eth_group_inst = eth_group()
            ret = eth_group_inst.eth_group_open(int(values[0]), values[1])
            if ret != 0:
                return None

            if self.mtu == '':
                tab_len = 64 if self.direction == 'both' else 45
                print(''.center(tab_len, '='))
                print('maximum frame length'.ljust(24, ' '), end=' | ')
                if self.direction in ['tx', 'both']:
                    print('transmit'.ljust(16), end=' | ')
                if self.direction in ['rx', 'both']:
                    print('receive'.ljust(16), end=' | ')
                print()
                for i in self.ports:
                    print("mac {}".format(i).ljust(24), end=' | ')
                    if self.direction in ['tx', 'both']:
                        offset = MTU_REG_OFFSET[self.speed]['tx']
                        v = self.eth_group_mac_reg_read(eth_group_inst,
                                                        i, offset)
                        print('{:<16}'.format(v), end=' | ')
                    if self.direction in ['rx', 'both']:
                        offset = MTU_REG_OFFSET[self.speed]['rx']
                        v = self.eth_group_mac_reg_read(eth_group_inst,
                                                        i, offset)
                        print('{:<16}'.format(v), end=' | ')
                    print()
            else:
                for i in self.ports:
                    if self.direction in ['tx', 'both']:
                        offset = MTU_REG_OFFSET[self.speed]['tx']
                        self.eth_group_mac_reg_write(eth_group_inst,
                                                     i, offset, self.mtu)
                    if self.direction in ['rx', 'both']:
                        offset = MTU_REG_OFFSET[self.speed]['rx']
                        self.eth_group_mac_reg_write(eth_group_inst,
                                                     i, offset, self.mtu)
                eth_group_inst.eth_group_close()
                print("Done")

    def eth_group_port_info(self):
        info = self.eth_group_info(self.eth_grps)
        for mac_grp in info:
            if mac_grp == FPGA_MAC_GROUP_ID[self.side]:
                num, _, spd = info[mac_grp]
                self.mac_grp = mac_grp
                self.mac_number = num
                self.speed = spd

    def eth_group_start(self):
        self.eth_group_port_info()
        self.check_args()
        self.ports = self.get_port_list(self.argport, self.mac_number)
        if self.mtu is None:
            print('Please set configruation type of MAC, such as "--mtu"')
        else:
            self.eth_group_mac_mtu()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--segment', '-S', type=hexint,
                        help='Segment number of PCIe device')
    parser.add_argument('--bus', '-B', type=hexint,
                        help='Bus number of PCIe device')
    parser.add_argument('--device', '-D', type=hexint,
                        help='Device number of PCIe device')
    parser.add_argument('--function', '-F', type=hexint,
                        help='Function number of PCIe device')
    parser.add_argument('--mtu', nargs='?', const='',
                        help='maximum allowable ethernet frame length')
    parser.add_argument('--port', nargs='*', default='all',
                        help='select specific port (default: %(default)s)')
    parser.add_argument('--direction', choices=['tx', 'rx', 'both'],
                        default='both',
                        help='select direction of port (default: %(default)s)')
    parser.add_argument('--side', choices=['line', 'host'], default='host',
                        help='select mac on which side (default: %(default)s)')
    parser.add_argument('--debug', '-d', action='store_true',
                        help='Output debug information')
    args, left = parser.parse_known_args()

    f = FpgaFinder(args.segment, args.bus, args.device, args.function)
    devs = f.find()
    if args.debug:
        for d in devs:
            s = 'bdf: {segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**d)
            print(s)
    if len(devs) > 1:
        s = '{} FPGAs are found\nplease choose one FPGA'.format(len(devs))
        exception_quit(s, 1)
    if not devs:
        sys.exit(2)
    args.eth_grps = f.find_eth_group()
    print("args.eth_grps", args.eth_grps)
    for keys, values in args.eth_grps.items():
        print(keys, ":", values)

    lp = FPGAMAC(args)
    lp.eth_group_start()


if __name__ == "__main__":
    main()
