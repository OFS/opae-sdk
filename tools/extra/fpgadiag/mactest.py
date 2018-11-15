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
import os
import argparse
import binascii
import sys
from common import FpgaFinder, exception_quit, convert_argument_str2hex

sys_if = '/sys/class/net'
divide = '-----------------------------------------'


class MacromCompare(object):
    def __init__(self, args):
        self.args = args
        self.ethif = dict(zip(args.ethif, ['', ] * len(args.ethif)))
        self.mac = []

    def print_error_exit(self, msg):
        print('Error: {}'.format(msg))
        exit(-1)

    def get_if_and_mac_list(self):
        ifs = self.args.ethif or os.listdir(sys_if)
        # a group has 4 interfaces
        for i in ifs:
            t = [j for j in ifs if j[:-1] == i[:-1]]
            if self.args.ethif or len(t) == 4:
                with open(os.path.join(sys_if, i, 'address')) as f:
                    self.ethif[i] = f.read().strip()

        if self.ethif:
            print('Found {} ethernet interfaces:'.format(len(self.ethif)))
            for i in self.ethif.items():
                print('  {:<20} {}'.format(*i))
            print(divide)
        else:
            self.print_error_exit('No ethernet interface found!')

    def read_mac_from_nvmem(self):
        with open(self.args.nvmem, 'rb') as f:
            f.seek(self.args.offset, 0)
            for i in self.ethif:
                m = []
                for x in xrange(6):
                    m.append(binascii.b2a_hex(f.read(1)))
                self.mac.append(':'.join(m))
        print('Read {} mac addresses from NVMEM:'.format(len(self.mac)))
        for m in self.mac:
            print('  {}'.format(m))
        print(divide)

    def compare_eth_mac_with_macrom(self):
        result = 'PASS'
        for m in self.mac:
            if m not in self.ethif.values():
                print('{} in MAC ROM, have no relative interface!'.format(m))
                result = 'FAIL'
        for e, m in self.ethif.items():
            if m not in self.mac:
                print('{} associate with {}, is not from MAC ROM'.format(m, e))
                result = 'FAIL'
        print('TEST {}!'.format(result))

    def start(self):
        self.get_if_and_mac_list()
        self.read_mac_from_nvmem()
        self.compare_eth_mac_with_macrom()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bus', '-B',
                        help='Bus number of PCIe device')
    parser.add_argument('--device', '-D',
                        help='Device number of PCIe device')
    parser.add_argument('--function', '-F',
                        help='Function number of PCIe device')
    parser.add_argument('--nvmem',
                        help='nvmem path')
    parser.add_argument('--ethif',
                        nargs='*',
                        default=[],
                        help='specify ethernet interfaces')
    parser.add_argument('--offset',
                        default='0',
                        help='read mac address from a offset address')
    args, left = parser.parse_known_args()

    args = convert_argument_str2hex(
        args, ['bus', 'device', 'function', 'offset'])

    if not args.nvmem:
        f = FpgaFinder(args.bus, args.device, args.function)
        devs = f.find()
        for d in devs:
            print(
                'bus:0x{bus:x} device:0x{dev:x} function:0x{func:x}'.format(
                    **d))
        if len(devs) > 1:
            exception_quit('{} FPGAs are found\nplease choose '
                           'one FPGA to do mactest'.format(len(devs)))
        if not devs:
            exception_quit('no FPGA found')
        nvmem_path = f.find_node(devs[0].get('path'), 'nvmem')
        if not nvmem_path:
            exception_quit('No nvmem found at {}'.format(devs[0].get('path')))
        for p in nvmem_path:
            print('found nvmem: {}'.format(p))
        args.nvmem = nvmem_path[0]
        if len(nvmem_path) > 1:
            print('multi nvmem found, '
                  'select {} to do mactest'.format(args.nvmem))

    if os.path.exists(args.nvmem):
        m = MacromCompare(args)
        m.start()
    else:
        print('Error: {} is not exist!'.format(args.nvmem))


if __name__ == "__main__":
    main()
