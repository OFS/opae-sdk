"""
Copyright(c) 2020, Intel Corporation

Redistribution  and  use  in source  and  binary  forms,  with  or  without
modification, are permitted provided that the following conditions are met:

* Redistributions of  source code  must retain the  above copyright notice,
  this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name  of Intel Corporation  nor the names of its contributors
  may be used to  endorse or promote  products derived  from this  software
  without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
"""

import argparse
import os.path
import sys


class RegmapDebugfs():
    """RegmapDebugfs is a class for accessing the debugfs
       inteface of regmaps in the linux kernel"""

    def __init__(self, regmapdir):

        self.registers_path = os.path.join(regmapdir, 'registers')
        self.range_path = os.path.join(regmapdir, 'range')
        self.ranges = []
        self.record_len = 0
        self.register_stride = 0

    def get_register_info(self):
        """get information about regmap"""
        with open(self.registers_path, 'r') as _file:
            line0 = _file.readline()
            line1 = _file.readline()
            self.record_len = len(line0)
            list0 = line0.strip().split(': ')
            list1 = line1.strip().split(': ')
            self.register_stride = int(list1[0], 16) - int(list0[0], 16)

    def get_range_info(self):
        """get the list of address ranges for the regmap"""
        with open(self.range_path, 'r') as _file:
            for line in _file.readlines():
                list0 = line.strip().split('-')
                range_dict = {
                    'min': int(list0[0], 16),
                    'max': int(list0[1], 16),
                    'num_records': ((int(list0[1], 16) - int(list0[0], 16)) /
                                    self.register_stride) + 1,
                }
                self.ranges.append(range_dict)

    def reg_read(self, addr):
        """reg_read returns the value read from the given
           address in the regmap"""

        num_records = 0
        for range_dict in self.ranges:
            if range_dict['min'] <= addr <= range_dict['max']:
                offset = (addr - range_dict['min'])
                num_records += offset / self.register_stride
                break
            else:
                num_records += range_dict['num_records']
        else:
            sys.exit("bad read address")

        with open(self.registers_path, 'r') as _file:
            _file.seek(num_records * self.record_len)
            line0 = _file.readline()

        list0 = line0.strip().split(': ')
        raddr = int(list0[0], 16)

        if raddr != addr:
            msg = "bad addr read back 0x{:x} != 0x{:x}".format(raddr, addr)
            sys.exit(msg)

        return int(list0[1], 16)

    def reg_write(self, addr, val):
        """reg_write writes val to the addr in the regmap"""

        for range_dict in self.ranges:
            if range_dict['min'] <= addr <= range_dict['max']:
                break

        else:
            sys.exit("bad write address")

        with open(self.registers_path, 'rb+') as _file:
            _file.write("{:x} {:x}".format(addr, val).encode())


def is_regmap_dir(regmapdir):
    """check for a valid regmap directory in debugfs"""
    if not os.path.isdir(regmapdir):
        msg = f"{regmapdir} is not a valid directory"
        raise argparse.ArgumentTypeError(msg)

    range_path = os.path.join(regmapdir, 'range')
    if not os.path.isfile(range_path):
        msg = f"{range_path} is not a valid file"
        raise argparse.ArgumentTypeError(msg)

    registers_path = os.path.join(regmapdir, 'registers')
    if not os.path.isfile(registers_path):
        msg = f"{registers_path} is not a valid file"
        raise argparse.ArgumentTypeError(msg)

    return regmapdir


def main():
    """main entry point"""

    descr = "access to debugfs interface of regmaps"
    parser = argparse.ArgumentParser(description=descr)

    parser.add_argument('regmap', type=is_regmap_dir,
                        help='path to debugfs directory of regmap')

    parser.add_argument('cmd', help='regmap transaction',
                        choices=['read', 'write'])

    parser.add_argument('addr', help='hex address in regmap to access')

    parser.add_argument('val', nargs='?',
                        help='hex value to write to register')

    args = parser.parse_args()

    reg_map = RegmapDebugfs(args.regmap)
    reg_map.get_register_info()
    reg_map.get_range_info()

    addr = int(args.addr, 16)

    if args.cmd == 'read':
        val = reg_map.reg_read(addr)
        print("{:x}".format(val))
    elif args.cmd == 'write':
        reg_map.reg_write(addr, int(args.val, 16))


if __name__ == "__main__":
    main()
