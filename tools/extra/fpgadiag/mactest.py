#! /usr/bin/env python
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
import os
import argparse
import binascii
import sys
from common import FpgaFinder, exception_quit
from common import COMMON, convert_argument_str2hex

sys_if = '/sys/class/net'
divide = '-' * 80
fvl_side = 1


class MacromCompare(COMMON):
    def __init__(self, args):
        self.args = args
        self.ethif = {}
        self.mac = []
        self.number = 4

    def get_pci_common_root_path(self, path):
        link = os.readlink(path)
        m = link.split(os.sep)
        if len(m) > 4:
            return os.sep.join(m[:-4])

    def get_netif_number(self):
        info = self.get_eth_group_info(self.args.eth_grps)
        for grp in info:
            if grp == fvl_side:
                self.number, _, spd, node = info[grp]

    def get_if_and_mac_list(self):
        self.get_netif_number()
        pci_root = self.get_pci_common_root_path(self.args.fpga_root)
        ifs = os.listdir(sys_if)
        for i in ifs:
            root = self.get_pci_common_root_path(os.path.join(sys_if, i))
            if pci_root == root:
                with open(os.path.join(sys_if, i, 'address')) as f:
                    self.ethif[i] = f.read().strip()

        if self.ethif:
            print('Found {} ethernet interfaces:'.format(len(self.ethif)))
            ethifs = sorted(self.ethif.items())
            for i in ethifs:
                print('  {:<20} {}'.format(*i))
            print(divide)
        else:
            exception_quit('No ethernet interface found!')

    def str2hex(self, c):
        return int(binascii.b2a_hex(c), 16)

    def read_mac_from_nvmem(self):
        mac_number = 0
        mac = 0
        with open(self.args.nvmem, 'rb') as f:
            f.seek(self.args.offset, 0)
            for x in [40, 32, 24, 16, 8, 0]:
                mac |= self.str2hex(f.read(1)) << x
            mac_number = self.str2hex(f.read(1))
        print('Read {} mac addresses from NVMEM:'.format(mac_number))
        vendor = '{:06x}'.format(mac >> 24)
        for i in range(mac_number):
            mac += 1
            mac_str = '{}{:06x}'.format(vendor, (mac & 0xffffff))
            fmt_str = ':'.join([mac_str[x:x + 2]
                                for x in range(0, len(mac_str), 2)])
            self.mac.append(fmt_str)
        for m in self.mac:
            print('  {}'.format(m))
        print(divide)

    def compare_eth_mac_with_macrom(self):
        result = 'PASS'
        for m in self.mac:
            if m not in self.ethif.values():
                print('{} in MAC ROM, have no relative interface!'.format(m))
                result = 'FAIL'
        ethifs = sorted(self.ethif.items())
        for e, m in ethifs:
            if m not in self.mac:
                print('{} mac {}, is not from MAC ROM'.format(e, m))
                result = 'FAIL'
        print('TEST {}!'.format(result))

    def start(self):
        self.get_if_and_mac_list()
        self.read_mac_from_nvmem()
        self.compare_eth_mac_with_macrom()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--segment', '-S',
                        help='Segment number of PCIe device')
    parser.add_argument('--bus', '-B',
                        help='Bus number of PCIe device')
    parser.add_argument('--device', '-D',
                        help='Device number of PCIe device')
    parser.add_argument('--function', '-F',
                        help='Function number of PCIe device')
    parser.add_argument('--offset',
                        default='0',
                        help='read mac address from a offset address')
    args, left = parser.parse_known_args()

    args = convert_argument_str2hex(
        args, ['segment', 'bus', 'device', 'function', 'offset'])

    f = FpgaFinder(args.segment, args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        print(
            'bdf: {segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(
                **d))
    if len(devs) > 1:
        exception_quit('{} FPGAs are found\nplease choose '
                       'one FPGA to do mactest'.format(len(devs)))
    if not devs:
        exception_quit('no FPGA found')
    args.fpga_root = devs[0].get('path')
    nvmem_path = f.find_node(devs[0].get('path'), 'nvmem', depth=7)
    if not nvmem_path:
        nvmem_path = f.find_node(devs[0].get('path'), 'eeprom', depth=7)
    if not nvmem_path:
        exception_quit('No nvmem found at {}'.format(devs[0].get('path')))
    args.nvmem = nvmem_path[0]
    if len(nvmem_path) > 1:
        print('multi nvmem found, '
              'select {} to do mactest'.format(args.nvmem))
    args.eth_grps = f.find_node(devs[0].get('path'), 'eth_group*/dev', depth=3)
    if not args.eth_grps:
        exception_quit('No ethernet group found')
    for g in args.eth_grps:
        print('ethernet group device: {}'.format(g))

    m = MacromCompare(args)
    m.start()


if __name__ == "__main__":
    main()
