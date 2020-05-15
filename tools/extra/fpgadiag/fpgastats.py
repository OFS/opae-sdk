#! /usr/bin/env python
# Copyright(c) 2018-2019, Intel Corporation
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
from common import exception_quit, FpgaFinder, COMMON, hexint
import argparse
import time


BUILD_FLAG_MAC_LIGHTWEIGHT = 0x2
BUILD_FLAG_LIGHTWEIGHT = 0x8

AFU_DATAPATH_OFFSET = 0x100000


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
    fifo_stats_mux = (('MUX_CDC_FIFO_CNTR_FULL', 0x1),
                      ('MUX_CDC_FIFO_CNTR_ERROR', 0x2),
                      ('MUX_CDC_FIFO_CNTR_SOP_MISSED', 0x3),
                      ('MUX_CDC_FIFO_CNTR_EOP_MISSED', 0x4))
    fifo_stats_demux = (('DEMUX_CDC_FIFO_CNTR_FULL', 0x1),
                        ('DEMUX_CDC_FIFO_CNTR_ERROR', 0x2),
                        ('DEMUX_CDC_FIFO_CNTR_SOP_MISSED', 0x3),
                        ('DEMUX_CDC_FIFO_CNTR_EOP_MISSED', 0x4))
    fifo_stats_ingress = (('ING_CDC_FIFO_CNTR_FULL', 0x1),
                          ('ING_CDC_FIFO_CNTR_ERROR', 0x2),
                          ('ING_CDC_FIFO_CNTR_SOP_MISSED', 0x3),
                          ('ING_CDC_FIFO_CNTR_EOP_MISSED', 0x4),
                          ('ING_CDC_FIFO_CNTR_PKT_IN', 0x5),
                          ('ING_CDC_FIFO_CNTR_PKT_OUT', 0x6))
    fifo_stats_egress = (('EGR_CDC_FIFO_CNTR_FULL', 0x1),
                         ('EGR_CDC_FIFO_CNTR_ERROR', 0x2),
                         ('EGR_CDC_FIFO_CNTR_SOP_MISSED', 0x3),
                         ('EGR_CDC_FIFO_CNTR_EOP_MISSED', 0x4),
                         ('EGR_CDC_FIFO_CNTR_PKT_IN', 0x5),
                         ('EGR_CDC_FIFO_CNTR_PKT_OUT', 0x6))

    def __init__(self, args):
        if not hasattr(args, "build_flags"):
            setattr(args, "build_flags", 0)
        self.mac_lightweight = (True if args.build_flags &
                                BUILD_FLAG_MAC_LIGHTWEIGHT else False)
        self.lightweight = (True if args.build_flags & BUILD_FLAG_LIGHTWEIGHT
                            else False)
        self.eth_grps = args.eth_grps
        self.mac_number = 0
        self.demux_offset = {8: 0x100, 4: 0x80, 2: 0x40}
        if self.lightweight:
            self.sbdf = args.sbdf
            self.get_upl_base()

    def print_mac_stats(self, handler, stats, reg, length):
        print("{0: <32}".format(stats), end=' | ')
        for i in range(self.mac_number):
            data = 0
            for l in range(length):
                v = self.fpga_eth_reg_read(handler, 'mac', i, reg + l)
                data += (v & 0xffffffff) << (32 * l)
            print("{0: >12}".format(data), end=' | ')
        print()

    def print_fifo_stats(self, handler, stats, reg):
        print("{0: <32}".format(stats), end=' | ')
        for i in range(self.mac_number):
            h_addr = (reg & 0x100) + i * 8
            if self.lightweight:
                v = self.upl_indirect_rw(AFU_DATAPATH_OFFSET + reg + i * 8)
                data = v & 0xffffffff
                v = self.upl_indirect_rw(AFU_DATAPATH_OFFSET + h_addr)
                data += (v & 0xffffffff) << 32
            else:
                v = self.fpga_eth_reg_read(handler, 'eth', 0, reg + i * 8)
                data = v & 0xffffffff
                v = self.fpga_eth_reg_read(handler, 'eth', 0, h_addr)
                data += (v & 0xffffffff) << 32
            print("{0: >12}".format(data), end=' | ')
        print()

    def print_stats(self, info):
        if not self.mac_lightweight or not self.lightweight:
            for w in info:
                _, self.mac_number, spd, node = info[w]
                print('=' * (34 + self.mac_number * 15))
                s = 'MAC wrapper {}, Speed {}g'.format(w, spd)
                print("{0: <32}".format(s), end=' | ')
                for i in range(self.mac_number):
                    print('mac {}'.format(i).rjust(12, ' '), end=' | ')
                print()
                with open(node, 'r') as handler:
                    stats = (self.stats_25_40g if spd in [25, 40] else
                             self.stats_10g)
                    offset = self.demux_offset.get(self.mac_number, 0x100)
                    demux = ((n, r+offset) for n, r in self.fifo_stats_demux)
                    fifo_regs = self.fifo_stats_mux + tuple(demux)
                    if not self.mac_lightweight:
                        for s, reg, length in stats:
                            self.print_mac_stats(handler, s, reg, length)
                    if not self.lightweight:
                        for s, reg in fifo_regs:
                            self.print_fifo_stats(handler, s, reg)

        if self.lightweight:
            _, self.mac_number, spd, node = info[0]
            egress_offset = self.demux_offset.get(self.mac_number, 0x100)
            print('=' * (34 + self.mac_number * 15))
            print("{0: <32}".format('ingress fifo stats'), end=' | ')
            for i in range(self.mac_number):
                print('mac {}'.format(i).rjust(12, ' '), end=' | ')
            print()
            for s, reg in self.fifo_stats_ingress:
                self.print_fifo_stats(None, s, reg)
            print('=' * (34 + self.mac_number * 15))
            print("{0: <32}".format('egress fifo stats'), end=' | ')
            for i in range(self.mac_number):
                print('mac {}'.format(i).rjust(12, ' '), end=' | ')
            print()
            for s, reg in self.fifo_stats_egress:
                self.print_fifo_stats(None, s, egress_offset + reg)

    def clear_stats(self):
        info = self.get_eth_group_info(self.eth_grps)
        if self.lightweight:
            _, self.mac_number, _, node = info[0]
            offset = self.demux_offset.get(self.mac_number, 0x100)
            for i in range(self.mac_number):
                reg = 0x1 + i * 8
                self.upl_indirect_rw(AFU_DATAPATH_OFFSET + reg, 0x0)
                self.upl_indirect_rw(AFU_DATAPATH_OFFSET + offset + reg, 0x0)
                time.sleep(0.1)
        else:
            for w in info:
                _, self.mac_number, _, node = info[w]
                offset = self.demux_offset.get(self.mac_number, 0x100)
                with open(node, 'r') as fd:
                    for i in range(self.mac_number):
                        if self.mac_number == 8:
                            self.fpga_eth_reg_write(fd, 'mac', i, 0x140, 0x1)
                            self.fpga_eth_reg_write(fd, 'mac', i, 0x1C0, 0x1)
                        else:
                            self.fpga_eth_reg_write(fd, 'mac', i, 0x845, 0x1)
                            self.fpga_eth_reg_write(fd, 'mac', i, 0x945, 0x1)
                        reg = 0x1 + i * 8
                        self.fpga_eth_reg_write(fd, 'eth', 0, reg, 0x0)
                        self.fpga_eth_reg_write(fd, 'eth', 0, offset+reg, 0x0)
                        time.sleep(0.1)

    def start(self):
        info = self.get_eth_group_info(self.eth_grps)
        self.print_stats(info)


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
    parser.add_argument('--clear', '-c', action='store_true',
                        help='Clear statistics')
    parser.add_argument('--debug', '-d', action='store_true',
                        help='Output debug information')
    args, left = parser.parse_known_args()

    f = FpgaFinder(args.segment, args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        if args.debug:
            s = 'bdf: {segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**d)
            print(s)
    if len(devs) > 1:
        exception_quit('{} FPGAs are found\nplease choose '
                       'one FPGA'.format(len(devs)))
    if not devs:
        exception_quit('no FPGA found')

    args.sbdf = '{segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**devs[0])
    bitstream_id_path = f.find_node(devs[0].get('path'),
                                    'intel-fpga-fme.*/bitstream_id', depth=1)
    with open(bitstream_id_path[0], 'r') as fd:
        bitstream_id = fd.read().strip()
    args.build_flags = (int(bitstream_id, 16) >> 24) & 0xff
    args.eth_grps = f.find_node(devs[0].get('path'), 'eth_group*/dev', depth=4)
    if not args.eth_grps:
        exception_quit('No ethernet group found')
    for g in args.eth_grps:
        if args.debug:
            print('ethernet group device: {}'.format(g))

    f = FPGASTATS(args)
    if args.clear:
        f.clear_stats()
    else:
        f.start()


if __name__ == "__main__":
    main()
