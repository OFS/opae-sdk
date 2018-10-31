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
import argparse, time, os, fcntl, struct, stat, re
from common import exception_quit, FpgaFinder, PKVLCOMMON
from common import convert_argument_str2hex

# PKVL DEVICE NUMBER DEFINITION
PKVL_DEV_SIDE = {'local':4, 'remote':3}
PKVL_REG_PCS_CTL = 0xF010
PKVL_REG_PCS_25G_CNTL = 0x2000
PKVL_PCS_LANE_OFFSET_SHIFT = 0x200
PKVL_REG_LINE_PMA_CNTL = 0x0000
PKVL_REG_HOST_PMA_CNTL = 0x1000
PKVL_PMA_LANE_OFFSET_SHIFT = 0x2000

PKVL_PMA_CNTL = {'local':PKVL_REG_HOST_PMA_CNTL,
                 'remote':PKVL_REG_LINE_PMA_CNTL}

class PKVLLPBK(PKVLCOMMON):
    ports = []
    def __init__(self, args):
        self.en = args.en
        self.direction = args.direction
        self.pkvl_devs = args.pkvl_devs
        self.argport = args.port

    def pkvl_loopback_en(self, en):
        dev = PKVL_DEV_SIDE[self.direction]
        for i in range(self.PKVL_PHY_NUMBER):
            v = 0
            ports = [(p-i*self.PKVL_PHY_NUMBER) for p in self.ports \
                    if (p-i*self.PKVL_PHY_NUMBER) in range(self.PKVL_PORT_NUMBER)]
            if not ports:
                continue
            for p in ports:
                v |= (en<<p)
            self.pkvl_phy_reg_set_field(i, dev, PKVL_REG_PCS_CTL, 12, 4, v)

    def start(self):
        self.ports = self.get_port_list(self.argport, self.PKVL_PHY_NUMBER*self.PKVL_PORT_NUMBER)
        self.char_devs = self.get_pkvl_char_device(self.pkvl_devs)
        self.pkvl_loopback_en(self.en)

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
    parser.add_argument('--port',
                        nargs = '*',
                        default = 'all',
                        help = 'enable/disable loopback on specific port')
    args, left = parser.parse_known_args()

    if args.en == None:
        exception_quit('please specify --enable/--disable loopback!')
    else:
        op = 'enable' if args.en else 'disable'
        print('{} pkvl loopback'.format(op))

    args = convert_argument_str2hex(args, ['bus', 'device', 'function'])

    f = FpgaFinder(args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        print('bus:0x{bus:x} device:0x{dev:x} function:0x{func:x}'.format(**d))
    if len(devs) > 1:
        exception_quit('{} FPGAs are found\nplease choose '
                       'one FPGA'.format(len(devs)))
    if not devs:
        exception_quit('no FPGA found')
    args.pkvl_devs = f.find_node(devs[0].get('path'), 'misc/pkvl*/dev', depth=8)
    if not args.pkvl_devs:
        exception_quit('No pkvl device found')
    for dev in args.pkvl_devs:
        print('pkvl device: {}'.format(dev))

    lp = PKVLLPBK(args)
    lp.start()
    print('Done')

if __name__ == "__main__":
    main()
