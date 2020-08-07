#! /usr/bin/env python3
# Copyright(c) 2018-2020, Intel Corporation
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
import re
import os
import glob
import argparse
import sys
import time
import traceback
import fcntl
import stat
import struct
import mmap
import eth_group
from eth_group import *

PATTERN = (r'.*(?P<segment>\w{4}):(?P<bus>\w{2}):'
           r'(?P<dev>\w{2})\.(?P<func>\d).*')

BDF_PATTERN = re.compile(PATTERN)

FPGA_ROOT_PATH = '/sys/class/fpga_region'
CHAR_DEV = '/dev/char'
ETH_GROUP_IOMMU_GROUPS = "/sys/kernel/iommu_groups/*[0-9]/devices/*-*-*-*"

MAPSIZE = mmap.PAGESIZE
MAPMASK = MAPSIZE - 1

UPL_INDIRECT_CTRL_REG = 0x18
UPL_INDIRECT_DATA_REG = 0x20

DFH_TYPE_SHIFT = 60
DFH_TYPE_MASK = 0xf
DFH_TYPE_AFU = 0x1
DFH_TYPE_FIU = 0x4

DFH_ID_SHIFT = 0
DFH_ID_MASK = 0xfff
DFH_ID_UPL = 0x1f

NEXT_AFU_OFFSET_REG = 0x18
NEXT_AFU_OFFSET_MASK = 0xffffff


def exception_quit(msg, retcode=-1):
    print(msg)
    sys.exit(retcode)


def hexint(val):
    return int(val, 16)


class FpgaFinder(object):
    def __init__(self, segment, bus, dev, func):
        self.segment = segment
        self.bus = bus
        self.dev = dev
        self.func = func
        self.all_devs = []
        self.match_dev = []
        self.get_fpga_device_list()

    def read_bdf(self, path):
        symlink = os.readlink(path)
        m = BDF_PATTERN.match(symlink)
        data = m.groupdict() if m else {}
        return dict([(k, int(v, 16)) for (k, v) in data.items()])

    def get_fpga_device_list(self):
        if os.path.exists(FPGA_ROOT_PATH):
            paths = glob.glob(os.path.join(FPGA_ROOT_PATH, 'region*'))
            for p in paths:
                bdf = self.read_bdf(os.path.join(p, 'device'))
                if bdf:
                    bdf['path'] = p
                    self.all_devs.append(bdf)

    def find(self):
        if not self.all_devs:
            print('No FPGA device find at {}'.format(FPGA_ROOT_PATH))
        for dev in self.all_devs:
            r = True
            for i in dev:
                r &= (getattr(self, i) == dev.get(i)
                      if hasattr(self, i) and getattr(self, i) is not None
                      else True)
            if r:
                self.match_dev.append(dev)
        return self.match_dev

    def find_node(self, root, node, depth=5):
        paths = []
        for x in range(depth):
            r = glob.glob(os.path.join(os.path.join(root, *['*'] * x), node))
            paths.extend(r)
        return paths

    def find_eth_group(self, root):
        eth_group = {}
        paths = glob.glob(ETH_GROUP_IOMMU_GROUPS)
        i = 0
        for path in paths:
            print("fpga vfio iommu group:", path)
            one, guid = os.path.split(path)
            regex = re.compile(r'(/sys/kernel/iommu_groups/\d+)', re.I)
            one, group_id = os.path.split(regex.findall(path)[0])
            fpga_path = glob.glob(os.path.join(
                                  root,
                                  'dfl-fme*/dfl-fme*/',
                                  guid))
            if len(fpga_path) == 0:
                continue
            eth_group[i] = [group_id, guid]
            i = i + 1
        return eth_group


class COMMON(object):
    sbdf = None
    upl_base = 0x40000
    mac_lightweight = False
    eth_comp = {'phy': 1,
                'mac': 2,
                'eth': 3}

    ETH_GROUP_GET_INFO = 0xB702
    ETH_GROUP_READ_REG = 0xB703
    ETH_GROUP_WRITE_REG = 0xB704

    if_fmt = '=IIBBBB'
    rd_fmt = '=IIBBHI'
    wr_fmt = '=IIBBHI'
    if_len = struct.calcsize(if_fmt)
    rd_len = struct.calcsize(rd_fmt)
    wr_len = struct.calcsize(wr_fmt)

    eth_group_inst = None

    def eth_group_info(self, eth_grps):
        info = {}
        for keys, values in eth_grps.items():
            eth_group_inst = eth_group()
            ret = eth_group_inst.eth_group_open(int(values[0]), values[1])
            if ret != 0:
                return None
            print("eth groups: \n")
            print("direction:", eth_group_inst.direction)
            print("speed:", eth_group_inst.speed)
            print("phy_num:", eth_group_inst.phy_num)
            print("group_id:", eth_group_inst.group_id)
            print("df_id:", eth_group_inst.df_id)
            print("eth_lwmac:", eth_group_inst.eth_lwmac)
            self.mac_lightweight \
                = \
                self.mac_lightweight \
                or (eth_group_inst.eth_lwmac & 1) == 1
            info[eth_group_inst.group_id] = [eth_group_inst.phy_num,
                                             eth_group_inst.phy_num,
                                             eth_group_inst.speed]
            eth_group_inst.eth_group_close()
        return info

    def eth_group_reg_write(self, eth_group, comp, dev, reg, v):
        ret = eth_group.write_reg(self.eth_comp[comp], dev, 0, reg, v)
        return ret

    def eth_group_reg_read(self, eth_group, comp, dev, reg):
        ret = eth_group.read_reg(self.eth_comp[comp], dev, 0, reg)
        return ret

    def eth_group_reg_set_field(self, eth_group, comp,
                                dev, reg, idx, width, value):
        v = self.eth_group_reg_read(eth_group, comp, dev, reg)
        v = self.register_field_set(v, idx, width, value)
        self.eth_group_reg_write(eth_group, comp, dev, reg, v)

    def ioctl(self, handler, op, data):
        if isinstance(handler, str):
            with open(handler, 'w') as f:
                ret = self._ioctl(f, op, data)
        else:
            ret = self._ioctl(handler, op, data)
        return ret

    def _ioctl(self, handler, op, data):
        try:
            ret = fcntl.ioctl(handler, op, data)
        except Exception as e:
            traceback.print_exc()
            handler.close()
            exception_quit('ioctl fail: {}'.format(e))
        return ret

    # f: fpga handler
    # phy: phy index
    # reg: fpga register offset
    # idx: propose to change the register field lowest bit index
    # width: propose to change register field length
    # value: propose to change register field value
    def fpga_eth_reg_set_field(self, f, comp, dev, reg, idx, width, value):
        v = self.fpga_eth_reg_read(f, comp, dev, reg)
        v = self.register_field_set(v, idx, width, value)
        self.fpga_eth_reg_write(f, comp, dev, reg, v)

    def fpga_eth_reg_write(self, f, comp, dev, reg, v):
        v = struct.pack(
            self.wr_fmt,
            self.wr_len,
            0,
            self.eth_comp[comp],
            dev,
            reg,
            v)
        self.ioctl(f, self.ETH_GROUP_WRITE_REG, v)

    def fpga_eth_reg_read(self, f, comp, dev, reg):
        v = struct.pack(
            self.rd_fmt,
            self.rd_len,
            0,
            self.eth_comp[comp],
            dev,
            reg,
            0)
        ret = self.ioctl(f, self.ETH_GROUP_READ_REG, v)
        _, _, _, _, _, v = struct.unpack(self.rd_fmt, ret)
        return v

    def get_eth_group_info(self, eth_grps):
        info = {}
        for eth_grp in eth_grps:
            with open(eth_grp, 'r') as f:
                node = os.path.join(CHAR_DEV, f.readline().strip())
            data = struct.pack(self.if_fmt, self.if_len, *[0] * 5)
            ret = self.ioctl(
                os.path.join(
                    CHAR_DEV,
                    node),
                self.ETH_GROUP_GET_INFO,
                data)
            _, flags, spd, phy, mac, grp = struct.unpack(self.if_fmt, ret)
            self.mac_lightweight = self.mac_lightweight or (flags & 1) == 1
            info[grp] = [phy, mac, spd, node]
        return info

    def is_mac_lightweight_image(self, eth_grps):
        self.eth_group_info(eth_grps)
        return self.mac_lightweight

    def is_char_device(self, dev):
        m = os.stat(dev).st_mode
        return stat.S_ISCHR(m)

    # set value to reg_data
    # idx is field lowest index
    def register_field_set(self, reg_data, idx, width, value):
        mask = 0
        for x in range(width):
            mask |= (1 << x)
        value &= mask
        reg_data &= ~(mask << idx)
        reg_data |= (value << idx)
        return reg_data

    def get_port_list(self, argport, total):
        if 'all' in argport:
            return list(range(total))
        elif isinstance(argport, list):
            ports = []
            for p in argport:
                if p.isdigit():
                    if int(p) < total:
                        ports.append(int(p))
                    else:
                        exception_quit('Invalid argument port {}'.format(p))
                elif '-' in p:
                    s, e = p.split('-')
                    s = s.strip()
                    e = e.strip()
                    if s.isdigit() and e.isdigit() and total >= (int(e) + 1):
                        ports.extend(list(range(int(s), int(e) + 1)))
                    else:
                        exception_quit(
                            'Invalid argument port {}-{}'.format(s, e))
            ports.sort()
            return ports

    def pci_read(self, pci_dev_path, addr):
        base = addr & ~MAPMASK
        offset = addr & MAPMASK
        data = b'\xff'*8

        with open(pci_dev_path, "rb", 0) as f:
            mm = mmap.mmap(f.fileno(), MAPSIZE, mmap.MAP_SHARED,
                           mmap.PROT_READ, 0, base)
            # read data (64 bit)
            data = mm[offset:offset+8]
            value, = struct.unpack('<Q', data)
            # close mapping
            mm.close()
        return value

    def pci_write(self, pci_dev_path, addr, value):
        base = addr & ~MAPMASK
        offset = addr & MAPMASK
        data = struct.pack('<Q', value)

        # mmap PCI resource
        with open(pci_dev_path, "r+b", 0) as f:
            mm = mmap.mmap(f.fileno(), MAPSIZE, mmap.MAP_SHARED,
                           mmap.PROT_WRITE, 0, base)
            # write data (64 bit)
            mm[offset:offset+8] = data
            # close mapping
            mm.close()

    def pci_bar2_rw(self, addr, data=None):
        pci_bar2_path = '/sys/bus/pci/devices/{}/resource2'
        if data is None:    # read
            return self.pci_read(pci_bar2_path.format(self.sbdf), addr)
        else:   # write
            self.pci_write(pci_bar2_path.format(self.sbdf), addr, data)

    def upl_indirect_rw(self, addr, data=None):
        addr &= 0xffffff
        rdata = 0
        if data is None:      # read
            cmd = (1 << 62) | (addr << 32)
            self.pci_bar2_rw(self.upl_base + UPL_INDIRECT_CTRL_REG, cmd)
            while (rdata >> 32) != 0x1:     # waiting for read valid
                rdata = self.pci_bar2_rw(self.upl_base + UPL_INDIRECT_DATA_REG)
            return rdata & 0xffffffff
        else:       # write
            cmd = (2 << 62) | (addr << 32) | (data & 0xffffffff)
            self.pci_bar2_rw(self.upl_base + UPL_INDIRECT_CTRL_REG, cmd)
            if (rdata >> 32) != 0x1:     # waiting for write complete
                rdata = self.pci_bar2_rw(self.upl_base + UPL_INDIRECT_DATA_REG)

    def get_upl_base(self):
        addr = 0
        while True:
            header = self.pci_bar2_rw(addr)
            feature_type = (header >> DFH_TYPE_SHIFT) & DFH_TYPE_MASK
            feature_id = (header >> DFH_ID_SHIFT) & DFH_ID_MASK
            if feature_type == DFH_TYPE_AFU and feature_id == DFH_ID_UPL:
                self.upl_base = addr
                break
            if feature_type in [DFH_TYPE_AFU, DFH_TYPE_FIU]:
                next_afu_offset = self.pci_bar2_rw(addr+NEXT_AFU_OFFSET_REG)
                next_afu_offset &= NEXT_AFU_OFFSET_MASK
            if next_afu_offset == 0 or (next_afu_offset & 0xffff) == 0xffff:
                print("Use default UPL base address {:#x}".format(
                                                                self.upl_base))
                break
            else:
                addr += next_afu_offset
                next_afu_offset = 0


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
    args, left = parser.parse_known_args()

    finder = FpgaFinder(args.segment, args.bus, args.device, args.function)
    finder.find()
    print('find {} node'.format(len(finder.match_dev)))
    for n in finder.match_dev:
        print(n)


if __name__ == "__main__":
    main()
