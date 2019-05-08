#! /usr/bin/env python
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
from common import exception_quit, FpgaFinder, COMMON
from common import convert_argument_str2hex
import argparse


class FPGASTATS(COMMON):
    stats_10g = (('tx_stats_framesOK', 0x142, 1),
                 ('rx_stats_framesOK', 0x1c2, 1),
                 ('tx_stats_pauseMACCtrl_Frames', 0x14a, 1),
                 ('rx_stats_pauseMACCtrl_Frames', 0x1ca, 1),
                 ('tx_stats_framesErr', 0x144, 1),
                 ('rx_stats_framesErr', 0x1c4, 1),
                 ('rx_stats_framesCRCErr', 0x1c6, 1),
                 ('tx_stats_ifErrors', 0x14c, 1),
                 ('rx_stats_ifErrors', 0x1cc, 1))
    stats_25_40g = (('CNTR_TX_FRAGMENTS', 0x800, 2),
                    ('CNTR_TX_JABBERS', 0x802, 2),
                    ('CNTR_TX_FCS', 0x804, 2),
                    ('CNTR_TX_CRCERR', 0x806, 2),
                    ('CNTR_TX_MCAST_DATA_ERR', 0x808, 2),
                    ('CNTR_TX_BCAST_DATA_ERR', 0x80a, 2),
                    ('CNTR_TX_UCAST_DATA_ERR', 0x80c, 2),
                    ('CNTR_TX_MCAST_CTRL_ERR', 0x80e, 2),
                    ('CNTR_TX_BCAST_CTRL_ERR', 0x810, 2),
                    ('CNTR_TX_UCAST_CTRL_ERR', 0x812, 2),
                    ('CNTR_TX_PAUSE_ERR', 0x814, 2),
                    ('CNTR_TX_64B', 0x816, 2),
                    ('CNTR_TX_65to127B', 0x818, 2),
                    ('CNTR_TX_128to255B', 0x81a, 2),
                    ('CNTR_TX_256to511B', 0x81c, 2),
                    ('CNTR_TX_512to1023B', 0x81e, 2),
                    ('CNTR_TX_1024to1518B', 0x820, 2),
                    ('CNTR_TX_1519toMAXB', 0x822, 2),
                    ('CNTR_TX_OVERSIZE', 0x824, 2),
                    ('CNTR_TX_MCAST_DATA_OK', 0x826, 2),
                    ('CNTR_TX_BCAST_DATA_OK', 0x828, 2),
                    ('CNTR_TX_UCAST_DATA_OK', 0x82a, 2),
                    ('CNTR_TX_MCAST_CRTL', 0x82c, 2),
                    ('CNTR_TX_BCAST_CTRL', 0x82e, 2),
                    ('CNTR_TX_UCAST_CTRL', 0x830, 2),
                    ('CNTR_TX_PAUSE', 0x832, 2),
                    ('CNTR_TX_RUNT', 0x834, 2),
                    ('CNTR_RX_FRAGMENTS', 0x900, 2),
                    ('CNTR_RX_JABBERS', 0x902, 2),
                    ('CNTR_RX_FCS', 0x904, 2),
                    ('CNTR_RX_CRCERR', 0x906, 2),
                    ('CNTR_RX_MCAST_DATA_ERR', 0x908, 2),
                    ('CNTR_RX_BCAST_DATA_ERR', 0x90a, 2),
                    ('CNTR_RX_UCAST_DATA_ERR', 0x90c, 2),
                    ('CNTR_RX_MCAST_CTRL_ERR', 0x90e, 2),
                    ('CNTR_RX_BCAST_CTRL_ERR', 0x910, 2),
                    ('CNTR_RX_UCAST_CTRL_ERR', 0x912, 2),
                    ('CNTR_RX_PAUSE_ERR', 0x914, 2),
                    ('CNTR_RX_64B', 0x916, 2),
                    ('CNTR_RX_65to127B', 0x918, 2),
                    ('CNTR_RX_128to255B', 0x91a, 2),
                    ('CNTR_RX_256to511B', 0x91c, 2),
                    ('CNTR_RX_512to1023B', 0x91e, 2),
                    ('CNTR_RX_1024to1518B', 0x920, 2),
                    ('CNTR_RX_1519toMAXB', 0x922, 2),
                    ('CNTR_RX_OVERSIZE', 0x924, 2),
                    ('CNTR_RX_MCAST_DATA_OK', 0x926, 2),
                    ('CNTR_RX_BCAST_DATA_OK', 0x928, 2),
                    ('CNTR_RX_UCAST_DATA_OK', 0x92a, 2),
                    ('CNTR_RX_MCAST_CRTL', 0x92c, 2),
                    ('CNTR_RX_BCAST_CTRL', 0x92e, 2),
                    ('CNTR_RX_UCAST_CTRL', 0x930, 2),
                    ('CNTR_RX_PAUSE', 0x932, 2),
                    ('CNTR_RX_RUNT', 0x934, 2))
    fifo_stats_10g = (('MUX_CDC_FIFO_CNTR_FULL', 0x1, 1),
                      ('MUX_CDC_FIFO_CNTR_ERROR', 0x2, 1),
                      ('MUX_CDC_FIFO_CNTR_SOP_MISSED', 0x3, 1),
                      ('MUX_CDC_FIFO_CNTR_EOP_MISSED', 0x4, 1),
                      ('DEMUX_CDC_FIFO_CNTR_FULL', 0x101, 1),
                      ('DEMUX_CDC_FIFO_CNTR_ERROR', 0x102, 1),
                      ('DEMUX_CDC_FIFO_CNTR_SOP_MISSED', 0x103, 1),
                      ('DEMUX_CDC_FIFO_CNTR_EOP_MISSED', 0x104, 1))
    fifo_stats_25_40g = (('MUX_CDC_FIFO_CNTR_FULL', 0x1, 1),
                         ('MUX_CDC_FIFO_CNTR_ERROR', 0x2, 1),
                         ('MUX_CDC_FIFO_CNTR_SOP_MISSED', 0x3, 1),
                         ('MUX_CDC_FIFO_CNTR_EOP_MISSED', 0x4, 1),
                         ('DEMUX_CDC_FIFO_CNTR_FULL', 0x41, 1),
                         ('DEMUX_CDC_FIFO_CNTR_ERROR', 0x42, 1),
                         ('DEMUX_CDC_FIFO_CNTR_SOP_MISSED', 0x43, 1),
                         ('DEMUX_CDC_FIFO_CNTR_EOP_MISSED', 0x44, 1))

    def __init__(self, args):
        self.eth_grps = args.eth_grps
        self.mac_number = 0

    def print_device_stats(self, handler, stats, reg, length, fifo=False):
        print("{0: <30}".format(stats), end=' | ')
        comp = 'eth' if fifo else 'mac'
        for i in range(self.mac_number):
            data = 0
            for l in range(length):
                addr = reg + l * 8 if fifo else (reg + l)
                dev = 0 if fifo else i
                v = self.fpga_eth_reg_read(handler, comp, dev, addr)
                data += (0xffffffff & v) << (32 * l)
            print("{0: <12}".format(data), end=' | ')

    def print_stats(self, info):
        for w in info:
            _, self.mac_number, spd, node = info[w]
            print('=' * 100)
            print('MAC wrapper {}, Speed {}g'.format(w, spd))
            print("{0: <30}".format('stats'), end=' | ')
            for i in range(self.mac_number):
                print('mac {:<8}'.format(i), end=' | ')

            with open(node, 'rw') as handler:
                stats, fifo_regs = ((self.stats_25_40g, self.fifo_stats_25_40g)
                                    if spd in [25, 40] else
                                    (self.stats_10g, self.fifo_stats_10g))
                for s, reg, length in stats:
                    self.print_device_stats(handler, s, reg, length)
                for s, reg, length in fifo_regs:
                    self.print_device_stats(handler, s, reg, length, True)

    def start(self):
        info = self.get_eth_group_info(self.eth_grps)
        self.print_stats(info)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bus', '-B',
                        help='Bus number of PCIe device')
    parser.add_argument('--device', '-D',
                        help='Device number of PCIe device')
    parser.add_argument('--function', '-F',
                        help='Function number of PCIe device')
    args, left = parser.parse_known_args()

    args = convert_argument_str2hex(args, ['bus', 'device', 'function'])

    f = FpgaFinder(args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        print('bus:{bus:#x} device:{dev:#x} function:{func:#x}'.format(**d))
    if len(devs) > 1:
        exception_quit('{} FPGAs are found\nplease choose '
                       'one FPGA'.format(len(devs)))
    if not devs:
        exception_quit('no FPGA found')
    args.eth_grps = f.find_node(devs[0].get('path'), 'eth_group*/dev', depth=3)
    if not args.eth_grps:
        exception_quit('No ethernet group found')
    for g in args.eth_grps:
        print('ethernet group device: {}'.format(g))

    f = FPGASTATS(args)
    f.start()


if __name__ == "__main__":
    main()
