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
import argparse, time, struct, os
from common import FpgaFinder, exception_quit, COMMON
from common import convert_argument_str2hex

FPGA_PHY_GROUP_ID = {'host':1, 'line':0}

FPGA_PHY_REG_RX_SERIALLPBKEN = 0x2E1

PHY_GROUP_GET_INFO = 0xB700
PHY_GROUP_READ_REG = 0xB701
PHY_GROUP_WRITE_REG = 0xB702
PHY_GROUP_RESET_PHY = 0xB703

PHY_GROUP_ENABLE_PHY = 1
PHY_GROUP_DISABLE_PHY = 2

char_dev = '/dev/char'

grp_info_fmt = '=IIHHH'
rd_fmt = '=IIHHI'
wr_fmt = '=IIHHI'
reset_fmt = '=IIH'
info_len = struct.calcsize(grp_info_fmt)
rd_len = struct.calcsize(rd_fmt)
wr_len = struct.calcsize(wr_fmt)
reset_len = struct.calcsize(reset_fmt)

class FPGALPBK(COMMON):
    ports = []
    phy_number = 0
    phy_group_dev = ''
    def __init__(self, args):
        self.en = args.en
        self.side = args.side
        self.type = args.type
        self.direction = args.direction
        self.argport = args.port
        self.phy_grps = args.phy_grps

    def check_args(self):
        type_dict = {'line': {'local':['serial'], 'remote':['precdr', 'postcdr']},
                     'host': {'remote':['serial'], 'local':['precdr', 'postcdr']}}
        support_type = type_dict[self.side][self.direction]
        if self.type not in support_type:
            prompt_msg = ('fpga support loopback type {} at side [{}] '
                          'in the direction [{}]'.format(support_type,
                                                         self.side,
                                                         self.direction))
            exception_quit('{}\n{} is given!'.format(prompt_msg, self.type))

    # f: fpga handler
    # phy: phy index
    # reg: fpga register offset
    # idx: propose to change the register field lowest bit index
    # width: propose to change register field length
    # value: propose to change register field value
    def fpga_phy_reg_set_field(self, f, phy, reg, idx, width, value):
        print('===============================================================')
        v = struct.pack(rd_fmt, rd_len, 0, phy, reg, 0)
        ret = self.ioctl(f, PHY_GROUP_READ_REG, v)
        arg, flag, p, a, v = struct.unpack(rd_fmt, ret)
        v = self.register_field_set(v, idx, width, value)
        print('WRITE   phy: {}   register: 0x{:x}   value: 0x{:x}'.format(phy, reg, v))
        v = struct.pack(wr_fmt, wr_len, 0, phy, reg, v)
        self.ioctl(f, PHY_GROUP_WRITE_REG, v)
        # read back value to check if configuration is effective or not
        v = struct.pack(rd_fmt, rd_len, 0, phy, reg, 0)
        ret = self.ioctl(f, PHY_GROUP_READ_REG, v)
        arg, flag, p, a, v = struct.unpack(rd_fmt, ret)
        print('VERIFY  phy: {}   register: 0x{:x}   value: 0x{:x}'.format(phy, reg, v))

    def fpga_phy_read_reg(self, f, phy, reg):
        print('===============================================================')
        v = struct.pack(rd_fmt, rd_len, 0, phy, reg, 0)
        ret = self.ioctl(f, PHY_GROUP_READ_REG, v)
        arg, flag, p, a, v = struct.unpack(rd_fmt, ret)
        print('READ   phy: {}   register: 0x{:x}   value: 0x{:x}'.format(phy, reg, v))
        return v

    def fpga_phy_enable(self, f, en):
        print('===============================================================')
        flag = PHY_GROUP_ENABLE_PHY if en else PHY_GROUP_DISABLE_PHY
        op = 'enable' if en else 'disable'
        for i in self.ports:
            v = struct.pack(reset_fmt, reset_len, flag, i)
            self.ioctl(f, PHY_GROUP_RESET_PHY, v)
            print('{}   phy: {}'.format(op, i))

    def fpga_loopback_en(self, en):
        with open(self.phy_group_dev, 'rw') as f:
            self.fpga_phy_enable(f, 0)
            time.sleep(1)
            if self.type == 'serial':
                for i in self.ports:
                    self.fpga_phy_reg_set_field(f, i, FPGA_PHY_REG_RX_SERIALLPBKEN, 0, 1, en)
            elif self.type == 'precdr':
                for i in self.ports:
                    self.fpga_phy_reg_set_field(f, i, 0x137, 7, 1, en)
                    self.fpga_phy_reg_set_field(f, i, 0x13c, 7, 1, 0)
                    self.fpga_phy_reg_set_field(f, i, 0x132, 4, 2, 0)
                    self.fpga_phy_reg_set_field(f, i, 0x142, 4, 1, en)
                    self.fpga_phy_reg_set_field(f, i, 0x11d, 0, 1, en)
            elif self.type == 'postcdr':
                for i in self.ports:
                    self.fpga_phy_reg_set_field(f, i, 0x137, 7, 1, 0)
                    self.fpga_phy_reg_set_field(f, i, 0x13c, 7, 1, en)
                    self.fpga_phy_reg_set_field(f, i, 0x132, 4, 2, en)
                    self.fpga_phy_reg_set_field(f, i, 0x142, 4, 1, 0)
                    self.fpga_phy_reg_set_field(f, i, 0x11d, 0, 1, 0)
            time.sleep(1)
            self.fpga_phy_enable(f, 1)

    def get_port_group_info(self):
        for phy_grp in self.phy_grps:
            with open(phy_grp, 'r') as f:
                node = f.readline().strip()
            data = struct.pack(grp_info_fmt, info_len, *[0]*4)
            ret = self.ioctl(os.path.join(char_dev, node), PHY_GROUP_GET_INFO, data)
            arg, flag, spd, num, grp = struct.unpack(grp_info_fmt, ret)
            if grp == FPGA_PHY_GROUP_ID[self.side]:
                self.speed = spd
                self.phy_number = num
                self.phy_group_dev = os.path.join(char_dev, node)
        if not self.phy_group_dev:
            exception_quit('FPGA phy group device not find')

    def start(self):
        self.check_args()
        self.get_port_group_info()
        self.ports = self.get_port_list(self.argport, self.phy_number)
        self.fpga_loopback_en(self.en)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bus', '-B',
                        help = 'Bus number of PCIe device')
    parser.add_argument('--device', '-D',
                        help = 'Device number of PCIe device')
    parser.add_argument('--function', '-F',
                        help = 'Function number of PCIe device')
    parser.add_argument('--direction',
                        required = True,
                        choices = ['local', 'remote'],
                        help = 'choose loopback direction from chip view')
    parser.add_argument('--enable',
                        action = 'store_const',
                        dest = 'en',
                        const = 1,
                        help = 'loopback enable')
    parser.add_argument('--disable',
                        action = 'store_const',
                        dest = 'en',
                        const = 0,
                        help = 'loopback disable')
    parser.add_argument('--type',
                        choices = ['serial', 'precdr', 'postcdr'],
                        help = 'choose loopback type')
    parser.add_argument('--port',
                        nargs = '*',
                        default = 'all',
                        help = 'enable/disable loopback on specific port')
    parser.add_argument('--side',
                        required = True,
                        choices = ['line', 'host'],
                        help = 'choose loopback on which side')
    args, left = parser.parse_known_args()

    if args.en == None:
        exception_quit('please specify --enable/--disable loopback!')
    else:
        op = 'enable' if args.en else 'disable'
        print('{} fpga loopback'.format(op))

    args = convert_argument_str2hex(args, ['bus', 'device', 'function'])

    if not args.type:
        if ((args.side == 'line' and args.direction == 'local') or
            (args.side == 'host' and args.direction == 'remote')):
            args.type = 'serial'
        else:
            exception_quit('missing argument --type [serial | precdr | postcdr]')

    f = FpgaFinder(args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        print('bus:0x{bus:x} device:0x{dev:x} function:0x{func:x}'.format(**d))
    if len(devs) > 1:
        exception_quit('{} FPGAs are found\nplease choose '
                        'one FPGA'.format(len(devs)))
    if not devs:
        exception_quit('no FPGA found')
    args.phy_grps = f.find_node(devs[0].get('path'), 'phy_group*/dev', depth=3)
    if not args.phy_grps:
        exception_quit('No phy group found')
    for g in args.phy_grps:
        print('phy group device: {}'.format(g))

    lp = FPGALPBK(args)
    lp.start()
    print('Done')

if __name__ == "__main__":
    main()
