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
import argparse
import sys
import traceback
import fcntl
import struct
import mmap
import mmap
import sys
import binascii
import glob
import stat
import time
from ctypes import c_uint64, Structure, Union, c_uint32

MAPSIZE = mmap.PAGESIZE
MAPMASK = MAPSIZE-1

FPGA_OFS_DEVID = ["0xbcce", "0xbccf", "0xaf00"]

PATTERN = (r'.*(?P<segment>\w{4}):(?P<bus>\w{2}):'
           r'(?P<dev>\w{2})\.(?P<func>\d).*')

BDF_PATTERN = re.compile(PATTERN, re.IGNORECASE)


def verify_pcie_address(pcie_address):
    m = BDF_PATTERN.match(pcie_address)
    if m is None:
        print("Invalid pcie address foramt", pcie_address)
        return False
    return True


def verify_fpga_device(pcie_address):
    paths = glob.glob(os.path.join("/sys/bus/pci/devices/",
                                   pcie_address,
                                   "device*"))
    for path in paths:
        with open(path, 'r') as fd:
            devic_id = fd.read().strip()
            if devic_id in FPGA_OFS_DEVID:
                return True
    return False


class dfh_bits(Structure):
    """
    pcie device Feature Header bits
    """
    _fields_ = [
                    ("id", c_uint64, 12),
                    ("revision", c_uint64, 4),
                    ("next_header_offset", c_uint64, 24),
                    ("eol", c_uint64, 1),
                    ("reserved", c_uint64, 19),
                    ("type", c_uint64, 4)
    ]


class dfh(Union):
    """
    PCIE Device Feature Header
    """
    _fields_ = [("bits", dfh_bits),
                ("value", c_uint64)]

    def __init__(self, value):
        self.value = value

    @property
    def id(self):
        return self.bits.id

    @property
    def revision(self):
        return self.bits.revision

    @property
    def next_header_offset(self):
        return self.bits.next_header_offset

    @property
    def eol(self):
        return self.bits.eol

    @property
    def type(self):
        return self.bits.type


class OFSREG(object):
    def __init__(self, args):
        self._pcie_address = args
        self.handle = None
        self.mm = None

    def open(self, address):
        paths = glob.glob(os.path.join("/sys/bus/pci/devices/",
                          self._pcie_address,
                          "resource0"))
        self.handle = open(paths[0], "rb", 0)
        base = address & ~MAPMASK
        self.mm = mmap.mmap(self.handle.fileno(),
                            MAPSIZE, mmap.MAP_SHARED,
                            mmap.PROT_READ, 0, base)

    def close(self):
        self.mm.close()
        self.handle.close()

    def reg32(self, offset):
        data_be = self.mm[offset:offset + 4]
        data = binascii.hexlify(data_be[::-1])
        return data

    def reg64(self, offset):
        data_be = self.mm[offset:offset + 8]
        data = binascii.hexlify(data_be[::-1])
        return data

    def print_pf0_csr(self):
        print("******PCIe Register ******")
        dfh_offset = 0x0
        offset = 0x0
        while True:
            self.open(dfh_offset)
            offset = 0x0
            fpga_dfh = dfh(int(self.reg64(offset), 16))
            print("\ndfh:{}   :{}".format(hex(dfh_offset),
                                          hex(fpga_dfh.value)))
            if fpga_dfh.bits.eol or not fpga_dfh.bits.next_header_offset:
                self.mm.close()
                break

            for x in range(4):
                offset = 0x8 * x
                reg = int(self.reg64(offset), 16)
                print("{}       :{}".format(hex(dfh_offset + offset),
                                            hex(reg)))

            dfh_offset += fpga_dfh.next_header_offset
            self.mm.close()

    def print_hssi_csr(self):
        print("******HSSI Register ******")
        result, hssi_offset = self.find_ofs_feature(0x15)
        if not result:
            print("Not found HSSI feature")
            return False
        self.open(hssi_offset)
        offset = 0x0
        hssi_dfh = dfh(int(self.reg64(offset), 16))
        print("\ndfh:{}   :{}".format(hex(hssi_offset),
                                      hex(hssi_dfh.value)))
        for x in range(43):
            offset = 0x4 * x
            reg = int(self.reg32(offset), 16)
            print("{}       :{}".format(hex(hssi_offset + offset),
                                        hex(reg)))
        self.close()

    def print_bmc_csr(self):
        print("******BMC Register ******")
        result, bmc_offset = self.find_ofs_feature(0x12)
        if not result:
            print("Not found BMC feature")
            return False
        self.open(bmc_offset)
        offset = 0x0
        bmc_dfh = dfh(int(self.reg64(offset), 16))
        print("\ndfh:{}   :{}".format(hex(bmc_offset), hex(bmc_dfh.value)))
        for x in range(43):
            offset = 0x8 * x
            reg = int(self.reg32(offset), 16)
            print("{}       :{}".format(hex(bmc_offset + offset), hex(reg)))
        self.close()

    def find_ofs_feature(self, feature_id):
        dfh_offset = 0x0
        offset = 0x0
        while True:
            self.open(dfh_offset)
            offset = 0x0
            fpga_dfh = dfh(int(self.reg64(offset), 16))
            if fpga_dfh.id == feature_id:
                self.close()
                return True, dfh_offset

            if fpga_dfh.bits.eol or not fpga_dfh.bits.next_header_offset:
                self.close()
                return False, 0

            dfh_offset += fpga_dfh.next_header_offset
            self.close()
        return False, 0


def main():
    """
    parse input arguemnts pciaddress and reg
    read mmio region and dump csr
    """
    parser = argparse.ArgumentParser()

    pcieaddress_help = 'sbdf of device to program \
                        (e.g. 0000:04:00.0).' \
                       ' Optional when one device in system.'
    parser.add_argument('--pcie-address', '-P',
                        default=None, help=pcieaddress_help)

    parser.add_argument('-r', '--reg',
                        default='pcie',
                        choices=['pcie', 'bmc', 'hssi', 'all'],
                        help='choose reg dump. Default: pcie')

    # exit if no commad line argument
    args = parser.parse_args()
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    args, left = parser.parse_known_args()

    print(args)
    print("--pcie_address:", args.pcie_address)
    if not verify_pcie_address(args.pcie_address.lower()):
        sys.exit(1)

    if not verify_fpga_device(args.pcie_address.lower()):
        sys.exit(1)

    print("reg:", args.reg)
    ofsregobj = OFSREG(args.pcie_address)

    if args.reg == 'pcie':
        ofsregobj.print_pf0_csr()

    if args.reg == 'hssi':
        ofsregobj.print_hssi_csr()

    if args.reg == 'bmc':
        ofsregobj.print_bmc_csr()

    sys.exit(0)


if __name__ == "__main__":
    main()
