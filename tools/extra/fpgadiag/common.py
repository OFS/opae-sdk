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

PATTERN = (r'.*(?P<segment>\w{4}):(?P<bus>\w{2}):'
           r'(?P<dev>\w{2})\.(?P<func>\d).*')

BDF_PATTERN = re.compile(PATTERN)

FPGA_ROOT_PATH = '/sys/class/fpga'
CHAR_DEV = '/dev/char'


def exception_quit(msg, retcode=-1):
    print(msg)
    sys.exit(retcode)


def convert_argument_str2hex(args, arg_list):
    if isinstance(arg_list, list):
        for i in arg_list:
            try:
                if getattr(args, i) is not None:
                    setattr(args, i, int(getattr(args, i), 16))
            except AttributeError:
                exception_quit(
                    'Attribute is not found {}: {}'.format(
                        i, getattr(
                            args, i)))
            except BaseException:
                exception_quit(
                    'Invalid input argument {}: {}'.format(
                        i, getattr(
                            args, i)))
    return args


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
            paths = glob.glob(os.path.join(root_path, 'intel-fpga-dev.*'))
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


class COMMON(object):
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

    def ioctl(self, file, op, data):
        if isinstance(file, str):
            with open(file, 'rw') as f:
                ret = self._ioctl(f, op, data)
        else:
            ret = self._ioctl(file, op, data)
        return ret

    def _ioctl(self, file, op, data):
        try:
            ret = fcntl.ioctl(file, op, data)
        except IOError:
            traceback.print_exc()
            file.close()
            exception_quit('ioctl IOError: {}'.format(e))
        except Exception as e:
            traceback.print_exc()
            file.close()
            exception_quit('ioctl fail: {}'.format(e))
        return ret

    # f: fpga file
    # phy: phy index
    # reg: fpga register offset
    # comp: ethernet array index
    # dev: device handle
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
            _, _, spd, phy, mac, grp = struct.unpack(self.if_fmt, ret)
            info[grp] = [phy, mac, spd, node]
        return info

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
            return range(total)
        elif isinstance(argport, list):
            ports = []
            for p in argport:
                if p.isdigit():
                    ports.append(int(p))
                elif '-' in p:
                    s, e = p.split('-')
                    s = s.strip()
                    e = e.strip()
                    if s.isdigit() and e.isdigit() and total >= (int(e) + 1):
                        ports.extend(range(int(s), int(e) + 1))
                    else:
                        exception_quit(
                            'Invalid argument port {}-{}'.format(s, e))
            ports.sort()
            return ports


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
    args, left = parser.parse_known_args()
    args = convert_argument_str2hex(
        args, ['segment', 'bus', 'device', 'function'])
    finder = FpgaFinder(args.segment, args.bus, args.device, args.function)
    finder.find()
    print('find {} node'.format(len(finder.match_dev)))
    for n in finder.match_dev:
        print(n)


if __name__ == "__main__":
    main()
