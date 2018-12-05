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
from opae import fpga
from common import exception_quit, COMMON
from common import convert_argument_str2hex
import argparse

enum_props = ['bus', 'device', 'function']
mac_number = 8
wrapper_number = 2
fifo_shift = 0x20
dev_sel_bit = 45


class FPGASTATS(COMMON):
    wrapper_0_base = 0x0000
    wrapper_0_info = wrapper_0_base + 0x8
    wrapper_0_ctl = wrapper_0_base + 0x10
    wrapper_0_stat = wrapper_0_base + 0x18
    wrapper_1_base = 0x8000
    wrapper_1_info = wrapper_1_base + 0x8
    wrapper_1_ctl = wrapper_1_base + 0x10
    wrapper_1_stat = wrapper_1_base + 0x18
    stats = {'tx_stats_framesOK': 0x142,
             'rx_stats_framesOK': 0x1c2,
             'tx_stats_pauseMACCtrl_Frames': 0x14a,
             'rx_stats_pauseMACCtrl_Frames': 0x1ca,
             'tx_stats_framesErr': 0x144,
             'rx_stats_framesErr': 0x1c4,
             'rx_stats_framesCRCErr': 0x1c6,
             'tx_stats_ifErrors': 0x14c,
             'rx_stats_ifErrors': 0x1cc}
    fifo_stats = {'MUX_CDC_FIFO_CNTR_FULL': 0x4,
                  'MUX_CDC_FIFO_CNTR_ERROR': 0x8,
                  'MUX_CDC_FIFO_CNTR_SOP_MISSED': 0xc,
                  'MUX_CDC_FIFO_CNTR_EOP_MISSED': 0x10,
                  'DEMUX_CDC_FIFO_CNTR_FULL': 0x204,
                  'DEMUX_CDC_FIFO_CNTR_ERROR': 0x208,
                  'DEMUX_CDC_FIFO_CNTR_SOP_MISSED': 0x20c,
                  'DEMUX_CDC_FIFO_CNTR_EOP_MISSED': 0x210}

    def __init__(self, args):
        self.guid = args.guid
        self.props = {}
        for i in enum_props:
            if getattr(args, i):
                self.props[i] = getattr(args, i)

    def fpga_enumerate(self):
        if self.guid:
            self.props['guid'] = self.guid
        props = fpga.properties(**self.props)
        self.toks = fpga.enumerate([props])

    # wrapper : mac wrapper index
    def indirect_read(self, handler, wrapper, dev_sel, dev, addr):
        rd = 0x4000000000000000
        addr = addr | (dev << 10)
        addr = addr << 32
        data = rd | addr
        if dev_sel:
            data |= (1 << dev_sel_bit)
        ctl = getattr(self, 'wrapper_{}_ctl'.format(wrapper))
        handler.write_csr64(ctl, data)
        stat = getattr(self, 'wrapper_{}_stat'.format(wrapper))
        data = handler.read_csr64(stat)
        return (data & 0xFFFFFFFF)

    def print_device_stats(self, handler, stats, wrapper, reg, fifo=False):
        print("{0: <30}".format(stats), end=' | ')
        for i in range(mac_number):
            reg = (reg + i * fifo_shift) if fifo else reg
            dev = 0 if fifo else i
            print("{0: <12}".format(self.indirect_read(handler,
                                                       wrapper,
                                                       fifo,
                                                       dev,
                                                       reg)), end=' | ')
        print()

    def print_stats(self, handler):
        for w in range(wrapper_number):
            print('=' * 100)
            print('MAC wrapper {}'.format(w))
            print("{0: <30}".format('stats'), end=' | ')
            for i in range(mac_number):
                print('mac {:<8}'.format(i), end=' | ')
            print()
            for stats, reg in self.stats.items():
                self.print_device_stats(handler, stats, w, reg)
            for stats, reg in self.fifo_stats.items():
                self.print_device_stats(handler, stats, w, reg, True)

    def start(self):
        self.fpga_enumerate()
        if not self.toks:
            exception_quit('No fpga found')
        with fpga.open(self.toks[0], fpga.OPEN_SHARED) as handler:
            self.print_stats(handler)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bus', '-B',
                        help='Bus number of PCIe device')
    parser.add_argument('--device', '-D',
                        help='Device number of PCIe device')
    parser.add_argument('--function', '-F',
                        help='Function number of PCIe device')
    parser.add_argument('--guid', '-G',
                        default='9aeffe5f-8457-0612-c000-c9660d824272',
                        help='AFU id')
    args, left = parser.parse_known_args()
    args = convert_argument_str2hex(args, ['bus', 'device', 'function'])
    f = FPGASTATS(args)
    f.start()


if __name__ == "__main__":
    main()
