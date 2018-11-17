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

pattern = (r'.*/\d+:(?P<bus>\w{2}):'
           r'(?P<dev>\d{2})\.(?P<func>\d).*')

bdf_pattern = re.compile(pattern)

ROOT_PATH = '/sys/class/fpga'
char_dev = '/dev/char'


def exception_quit(msg, retcode=-1):
    print(msg)
    sys.exit(retcode)


def convert_argument_str2hex(args, arg_list):
    if isinstance(arg_list, list):
        for i in arg_list:
            try:
                if getattr(args, i) is not None:
                    setattr(args, i, int(getattr(args, i), 16))
            except BaseException:
                exception_quit(
                    'Invlid argument {}: {}'.format(
                        i, getattr(
                            args, i)))
    return args


class FpgaFinder(object):
    def __init__(self, bus, dev, func):
        self.bus = bus
        self.dev = dev
        self.func = func
        self.all_devs = []
        self.match_dev = []
        self.get_fpga_device_list()

    def read_bdf(self, path):
        symlink = os.readlink(path)
        m = bdf_pattern.match(symlink)
        data = m.groupdict() if m else {}
        return dict([(k, int(v, 16)) for (k, v) in data.items()])

    def get_fpga_device_list(self):
        if os.path.exists(ROOT_PATH):
            paths = glob.glob(os.path.join(ROOT_PATH, 'intel-fpga-dev.*'))
            for p in paths:
                bdf = self.read_bdf(os.path.join(p, 'device'))
                if bdf:
                    bdf['path'] = p
                    self.all_devs.append(bdf)

    def find(self):
        if not self.all_devs:
            print('No FPGA device find at {}'.format(ROOT_PATH))
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
    def ioctl(self, handler, op, data):
        if isinstance(handler, str):
            with open(handler, 'rw') as f:
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


class PKVLCOMMON(COMMON):
    PKVL_PHY_NUMBER = 2
    PKVL_PORT_NUMBER = 4
    # IOCTL command
    PKVL_READ_PHY_REG = 0xB800
    PKVL_WRITE_PHY_REG = 0xB801

    PKVL_REG_WIDTH = 16
    data_fmt = '=IIHHH'
    data_len = struct.calcsize(data_fmt)
    char_devs = {}

    def get_pkvl_char_device(self, pkvl_devs):
        for d in pkvl_devs:
            with open(d, 'r') as f:
                dev = f.readline().strip()
            pkvl_dev = os.path.join(char_dev, dev)
            pn = os.path.basename(os.path.dirname(d))
            m = re.match(r'pkvl(\d+)', pn)
            if m:
                phy = int(m.group(1))
                if phy not in self.char_devs:
                    print('PKVL phy: {} device: {}'.format(phy, pkvl_dev))
                self.char_devs[phy] = pkvl_dev
        if not self.char_devs:
            exception_quit('Not found pkvl char device to do ioctl')
        m = [i for i in range(self.PKVL_PHY_NUMBER) if i not in self.char_devs]
        if m:
            exception_quit('pkvl phy {} not found'.format(m))

    def pkvl_phy_reg_set_field(self, phy, dev, reg, idx, width, value):
        with open(self.char_devs[phy], 'rw') as f:
            v = struct.pack(self.data_fmt, self.data_len, 0, dev, reg, 0)
            r = self.ioctl(f, self.PKVL_READ_PHY_REG, v)
            l, a, d, r, v = struct.unpack(self.data_fmt, r)
            v = self.register_field_set(v, idx, width, value)
            v = struct.pack(self.data_fmt, self.data_len, 0, dev, reg, v)
            self.ioctl(f, self.PKVL_WRITE_PHY_REG, v)
            time.sleep(0.001)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bus')
    parser.add_argument('--device')
    parser.add_argument('--function')
    args, left = parser.parse_known_args()
    args = convert_argument_str2hex(args, ['bus', 'device', 'function'])
    finder = FpgaFinder(args.bus, args.device, args.function)
    finder.find()
    print('find {} node'.format(len(finder.match_dev)))
    for n in finder.match_dev:
        print(n)


if __name__ == "__main__":
    main()
