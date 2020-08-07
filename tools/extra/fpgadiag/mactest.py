#! /usr/bin/env python3
# Copyright(c) 2020, Intel Corporation
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
import glob
import eth_group
from eth_group import *
from common import FpgaFinder, exception_quit, COMMON, hexint

SYSF_IF = '/sys/class/net'
DIVIDE = '-' * 80

HOST_SIDE = 1


class MacromCompare(COMMON):
    mode = 0
    mode_list = {0: 8,  # 8x10g
                 1: 4,  # 4x25g
                 2: 2,  # 2x1x25g
                 3: 4,  # 4x25g+2x25g
                 4: 4  # 2x2x25g
                 }

    def __init__(self, args):
        self.args = args
        self.ethif = {}
        self.mac = []
        self.number = 4
        self.bitstream_id = args.bitstream_id

    def get_pci_common_root_path(self, path):
        link = os.readlink(path)
        m = link.split(os.sep)
        if len(m) > 4:
            return os.sep.join(m[:-4])

    def get_if_and_mac_list(self):
        pci_root = self.get_pci_common_root_path(self.args.fpga_root)
        ifs = os.listdir(SYSF_IF)
        for i in ifs:
            root = self.get_pci_common_root_path(os.path.join(SYSF_IF, i))
            if pci_root == root:
                with open(os.path.join(SYSF_IF, i, 'address')) as f:
                    self.ethif[i] = f.read().strip()

        if self.ethif:
            print('Found {} ethernet interfaces:'.format(len(self.ethif)))
            ethifs = sorted(self.ethif.items())
            for i in ethifs:
                print('  {:<20} {}'.format(*i))
            print(DIVIDE)
        else:
            exception_quit('No ethernet interface found!')

    def str2hex(self, c):
        return int(binascii.b2a_hex(c), 16)

    def read_bitstream_id(self):
        with open(self.bitstream_id[0], 'r') as f:
            v = f.read()
            self.mode = (int(v, 16) >> 32) & 0xF

    def read_mac_info(self):
        mac = 0
        byte_offset = [40, 32, 24, 16, 8, 0]
        if self.args.mac_addr and self.args.mac_cnt:
            location = 'sysfs'
            with open(self.args.mac_addr, 'r') as f:
                mac_addr = f.read().strip()
            with open(self.args.mac_cnt, 'r') as f:
                cnt = f.read().strip()
                mac_number = int(cnt)
            m = mac_addr.split(':')
            if len(m) != 6:
                exception_quit('MAC address {} is invalid'.format(mac_addr))
            for i, x in enumerate(byte_offset):
                mac |= int(m[i], 16) << x
        else:
            location = 'NVMEM'
            with open(self.args.nvmem, 'rb') as f:
                f.seek(self.args.offset, 0)
                for x in byte_offset:
                    mac |= self.str2hex(f.read(1)) << x
                mac_number = self.str2hex(f.read(1))

        print('Read {} mac addresses from {}:'.format(mac_number, location))
        vendor = '{:06x}'.format(mac >> 24)
        for i in range(mac_number):
            mac_str = '{}{:06x}'.format(vendor, (mac & 0xffffff))
            fmt_str = ':'.join([mac_str[x:x + 2]
                                for x in range(0, len(mac_str), 2)])
            self.mac.append(fmt_str)
            mac += 1
        for m in self.mac:
            print('  {}'.format(m))
        print(DIVIDE)

    def compare_eth_mac_with_macrom(self):
        result = 'PASS'
        for m in self.mac:
            if m not in self.ethif.values():
                print('{} in MAC ROM, is not used!'.format(m))
        ethifs = sorted(self.ethif.items())
        for e, m in ethifs:
            if m not in self.mac:
                print('{} mac {}, is not from MAC ROM'.format(e, m))
                result = 'FAIL'
        print('TEST {}!'.format(result))

    def start(self):
        self.get_if_and_mac_list()
        self.read_bitstream_id()
        self.read_mac_info()
        self.compare_eth_mac_with_macrom()


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
    parser.add_argument('--offset',
                        default='0', type=hexint,
                        help='read mac address from a offset address')
    parser.add_argument('--debug', '-d', action='store_true',
                        help='Output debug information')
    args, left = parser.parse_known_args()

    f = FpgaFinder(args.segment, args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        if args.debug:
            s = 'bdf: {segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**d)
            print(s)
    if len(devs) > 1:
        exception_quit('{} FPGAs are found\nplease choose '
                       'one FPGA to do mactest'.format(len(devs)))
    if not devs:
        exception_quit('no FPGA found')
    args.fpga_root = devs[0].get('path')
    args.bitstream_id = f.find_node(devs[0].get('path'),
                                    'bitstream_id', depth=3)

    mac_addrs = glob.glob(os.path.join(devs[0].get('path'),
                                       'dfl-fme*', 'dfl-fme*', '*spi*',
                                       'spi_master', 'spi*', 'spi*',
                                       'mac_address'))
    args.mac_addr = None
    if len(mac_addrs) > 0:
        args.mac_addr = mac_addrs[0]
    mac_cnts = glob.glob(os.path.join(devs[0].get('path'),
                                      'dfl-fme*', 'dfl-fme*', '*spi*',
                                      'spi_master', 'spi*', 'spi*',
                                      'mac_count'))
    args.mac_cnt = None
    if len(mac_cnts) > 0:
        args.mac_cnt = mac_cnts[0]

    if args.mac_addr is None or args.mac_cnt is None:
        nvmem_path = f.find_node(devs[0].get('path'), 'nvmem', depth=7)
        if not nvmem_path:
            nvmem_path = f.find_node(devs[0].get('path'), 'eeprom', depth=7)
        if not nvmem_path:
            exception_quit('No nvmem found at {}'.format(devs[0].get('path')))
        args.nvmem = None
        if len(nvmem_path) > 0:
            args.nvmem = nvmem_path[0]
        if len(nvmem_path) > 1 and args.debug:
            s = 'multi nvmem found, select {} to do mactest'.format(args.nvmem)
            print(s)

    m = MacromCompare(args)
    m.start()


if __name__ == "__main__":
    main()
