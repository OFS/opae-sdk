#! /usr/bin/env python3
# Copyright(c) 2021-2022, Intel Corporation
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
import struct
import mmap
import sys
import binascii
import glob
import stat
import time
from ctypes import c_uint64, Structure, Union, c_uint32

from opae.admin.config import Config

MAPSIZE = mmap.PAGESIZE
MAPMASK = MAPSIZE-1

PATTERN = (r'.*(?P<segment>\w{4}):(?P<bus>\w{2}):'
           r'(?P<dev>\w{2})\.(?P<func>\d).*')

BDF_PATTERN = re.compile(PATTERN, re.IGNORECASE)

HSSI_FEATURE_ID = 0x15
PCIE_DFH_CSR_LEN = 20
HSSI_DFH_CSR_LEN = 43
ACCELERATOR_CSR_LEN = 30


def verify_pcie_address(pcie_address):
    """checks format of PCIe address string of the form bb:dd.f or
    ssss:bb:dd.f, Returns true if  PCIe address of the form ssss:bb:dd.f,
    or False if addr is not formatted correctly.
    """
    m = BDF_PATTERN.match(pcie_address)
    if m is None:
        print(f"Invalid pcie address format: {pcie_address}")
        return False

    if int(m.group('func'), 16) != 0:
        print(f"Not a valid pf0 pcie address: {pcie_address}")
        return False

    return True


def verify_fpga_device(pcie_address):
    """checks PCIe address device id in FPGA Device id's list
    Returns true if found in list, or
    False if not found in device list.
    """
    path = os.path.join("/sys/bus/pci/devices/", pcie_address)

    try:
        with open(os.path.join(path, 'vendor'), 'r') as fd:
            vendor = fd.read().strip()
        with open(os.path.join(path, 'device'), 'r') as fd:
            device = fd.read().strip()
        with open(os.path.join(path, 'subsystem_vendor'), 'r') as fd:
            subsystem_vendor = fd.read().strip()
        with open(os.path.join(path, 'subsystem_device'), 'r') as fd:
            subsystem_device = fd.read().strip()
    except FileNotFoundError as err:
        print("Not found vendor or device id")
        return False

    vendor = int(vendor, 16)
    device = int(device, 16)
    subsystem_vendor = int(subsystem_vendor, 16)
    subsystem_device = int(subsystem_device, 16)

    ID = (vendor, device, subsystem_vendor, subsystem_device)

    if Config.fpgareg_is_supported(*ID):
        print(f"fpga 0x{vendor:04x}:0x{device:04x} "
              f"0x{subsystem_vendor:04x}:0x{subsystem_device:04x}")
        print(Config.fpgareg_platform_for(*ID))
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


class FPGAREG(object):
    """
    FPGAREG reads pcie, hssi, bmc resources and dumps registers.
    """
    def __init__(self, addr):
        self.pcie_addr = addr
        self.handle = None
        self.mm = None

    def open(self, address, pcie_address=None):
        """
        opens pcie resources and mmap mmio region.
        """
        if pcie_address is None:
            pcie_address = self.pcie_addr
        paths = glob.glob(os.path.join("/sys/bus/pci/devices/",
                          pcie_address,
                          "resource0"))
        try:
            self.handle = open(paths[0], "rb", 0)
            base = address & ~MAPMASK
            self.mm = mmap.mmap(self.handle.fileno(),
                                MAPSIZE, mmap.MAP_SHARED,
                                mmap.PROT_READ, 0, base)
            return True
        except Exception:
            print("Failed to open & mmap fpga resource")
            return False

    def close(self):
        """
        closes release registers mmio region and close pcie resources.
        """
        self.mm.close()
        self.handle.close()

    def reg32(self, offset):
        """
        reads 32-bit registers from mmio region.
        """
        data_be = self.mm[offset:offset + 4]
        # binary representation of the data to hexadecimal.
        data = binascii.hexlify(data_be[::-1])
        return data

    def reg64(self, offset):
        """
        reads 64-bit registers from mmio region.
        """
        data_be = self.mm[offset:offset + 8]
        # binary representation of the data to hexadecimal.
        data = binascii.hexlify(data_be[::-1])
        return data

    def pf0_registers(self):
        """
        walks DFL and prints registers.
        """
        print("****** PCIe Register ******")
        dfh_offset = 0x0
        offset = 0x0
        while True:
            if not self.open(dfh_offset):
                return False
            offset = 0x0
            fpga_dfh = dfh(int(self.reg64(offset), 16))
            print("\ndfh:{}   :{}".format(hex(dfh_offset),
                                          hex(fpga_dfh.value)))

            if fpga_dfh.bits.eol or not fpga_dfh.bits.next_header_offset:
                self.mm.close()
                return True

            for x in range(PCIE_DFH_CSR_LEN):
                offset = 0x8 * x
                reg = int(self.reg64(offset), 16)
                print("{}       :{}".format(hex(dfh_offset + offset),
                                            hex(reg)))

            dfh_offset += fpga_dfh.next_header_offset
            self.mm.close()
        return True

    def hssi_registers(self):
        """
        walks DFL, finds hssi feature and prints registers.
        """
        print("****** HSSI Register ******")
        result, hssi_offset = self.find_ofs_feature(0x15)
        if not result:
            print("No HSSI feature found")
            return False
        if not self.open(hssi_offset):
            return False
        offset = 0x0
        hssi_dfh = dfh(int(self.reg64(offset), 16))
        print("\ndfh:{}   :{}".format(hex(hssi_offset),
                                      hex(hssi_dfh.value)))

        for x in range(HSSI_DFH_CSR_LEN):
            offset = 0x4 * x
            reg = int(self.reg32(offset), 16)
            print("{}       :{}".format(hex(hssi_offset + offset),
                                        hex(reg)))
        self.close()
        return True

    def find_ofs_feature(self, feature_id):
        """
        finds feature, return true or false and offset.
        """
        dfh_offset = 0x0
        offset = 0x0
        while True:
            if not self.open(dfh_offset):
                return False
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

    def bmc_registers(self):
        """
        prints BMC registers.
        """
        print("****** BMC Register ******")
        for path in sorted(glob.glob("/sys/kernel/debug/regmap/dfl_dev.*/*")):
            if path.find("name") >= 0:
                with open(path, 'r') as fd:
                    print("name:", fd.read().strip())

            if path.find("registers") >= 0:
                with open(path, 'r') as fd:
                    print("registers:\n", fd.read().strip())

    def accelerator_registers(self):
        """
        prints accelerator registers.
        """
        print("****** Accelerator Register ******")
        str = self.pcie_addr[:-1] + "*"
        for path in sorted(glob.glob(os.path.join("/sys/bus/pci/devices/",
                                     str))):
            pcie_addr = path.rsplit('/', 1)[1]
            if self.pcie_addr == pcie_addr:
                continue

            print("\npcie_address:", pcie_addr)
            offset = 0x0
            if not self.open(offset, pcie_addr):
                return False
            fpga_dfh = dfh(int(self.reg64(offset), 16))
            print("dfh:{}   :{}".format(hex(offset),
                                        hex(fpga_dfh.value)))

            for x in range(ACCELERATOR_CSR_LEN):
                offset = 0x8 * x
                reg = int(self.reg64(offset), 16)
                print("{}       :{}".format(hex(offset),
                                            hex(reg)))

            self.mm.close()
        return True


def main():
    """
    parse input arguments pci address and reg
    prints registers
    """
    parser = argparse.ArgumentParser()

    parser.add_argument('addr', nargs='?',
                        help='pcie address of the device')

    parser.add_argument('reg', nargs='?',
                        default='pcie',
                        choices=['pcie', 'bmc', 'hssi', 'acc', 'all'],
                        help='choose reg dump, default: pcie')

    # exit if no commad line argument
    args = parser.parse_args()
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    print(args)

    if not verify_pcie_address(args.addr.lower()):
        sys.exit(1)

    if not verify_fpga_device(args.addr.lower()):
        print("Invalid fpga device")
        sys.exit(1)

    fpgareg = FPGAREG(args.addr)

    """ print pf0/fme registers """
    if args.reg == 'pcie' and not fpgareg.pf0_registers():
        sys.exit(1)

    """ print hssi feature registers """
    if args.reg == 'hssi' and not fpgareg.hssi_registers():
        sys.exit(1)

    """ print bmc registers """
    if args.reg == 'bmc'and not fpgareg.bmc_registers():
        sys.exit(1)

    """ print all accelerator pf/vf's registers """
    if args.reg == 'acc' and not fpgareg.accelerator_registers():
        sys.exit(1)

    """ print all registers """
    if args.reg == 'all':
        fpgareg.pf0_registers()
        fpgareg.hssi_registers()
        fpgareg.bmc_registers()
        fpgareg.accelerator_registers()

    sys.exit(0)


if __name__ == "__main__":
    main()
