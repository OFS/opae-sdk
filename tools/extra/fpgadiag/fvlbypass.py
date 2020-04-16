#! /usr/bin/env python3
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

from __future__ import absolute_import
from __future__ import print_function
from __future__ import division
from common import exception_quit, FpgaFinder, COMMON, hexint
from fpgastats import FPGASTATS
from fpgalpbk import FPGALPBK
import argparse
import mmap
import sys
import struct
import glob
import os
import time


MAPSIZE = mmap.PAGESIZE
MAPMASK = MAPSIZE - 1

DFH_TYPE_SHIFT = 60
DFH_TYPE_MASK = 0xf
DFH_TYPE_AFU = 0x1
DFH_TYPE_FIU = 0x4

DFH_ID_SHIFT = 0
DFH_ID_MASK = 0xfff
DFH_ID_UPL = 0x1f

UPL_UUID_L = 0xA013D76F19C4D8D1
UPL_UUID_H = 0xFA00A55CCA8C4C4B

UUID_L_OFFSET_REG = 0x08
UUID_H_OFFSET_REG = 0x10
NEXT_AFU_OFFSET_REG = 0x18
NEXT_AFU_OFFSET_MASK = 0xffffff

VC_MODE_8x10G = 0
VC_MODE_2x1x25G = 2
VC_MODE_2x2x25G = 4

VC_MODE_NAME = {0: '8x10G',
                1: '4x25G',
                2: '2x1x25G',
                3: '6x25G',
                4: '2x2x25G'}

INDIRECT_CTRL_REG = 0x3
INDIRECT_DATA_REG = 0x4

MIN_TEST_PKT_NUM = 1
MAX_TEST_PKT_NUM = 100000000
DEFAULT_TEST_PKT_NUM = 100000

MIN_TEST_PKT_LEN = 46
MAX_TEST_PKT_LEN = 1500
DEFAULT_TEST_PKT_LEN = 128

VC_INFO = {}


def pci_read(pci_dev_path, addr):
    base = addr & ~MAPMASK
    offset = addr & MAPMASK
    data = b'\xff'*8

    with open(pci_dev_path, "rb", 0) as f:
        mm = mmap.mmap(f.fileno(), MAPSIZE, mmap.MAP_SHARED, mmap.PROT_READ,
                       0, base)
        # read data (64 bit)
        data = mm[offset:offset+8]
        value, = struct.unpack('<Q', data)

        # close mapping
        mm.close()

    return value


def pci_write(pci_dev_path, addr, value):
    base = addr & ~MAPMASK
    offset = addr & MAPMASK
    data = struct.pack('<Q', value)

    # mmap PCI resource
    with open(pci_dev_path, "r+b", 0) as f:
        mm = mmap.mmap(f.fileno(), MAPSIZE, mmap.MAP_SHARED, mmap.PROT_WRITE,
                       0, base)
        # write data (64 bit)
        mm[offset:offset+8] = data

        # close mapping
        mm.close()


# args: 0 - sbdf, 1 - device offset, 2 - addr, 3 - data, 4 - write check
def rw_data(*args):
    upl_base = VC_INFO.get('upl_base', None)
    if upl_base is None:
        exception_quit("Error: UPL not found in FPGA {}".format(args[0]), 6)
    pci_dev_path = '/sys/bus/pci/devices/{}/resource2'.format(args[0])
    addr = upl_base | (args[1] << 12) | (args[2] << 3)
    if len(args) == 3:      # read
        return pci_read(pci_dev_path, addr)
    elif len(args) == 5:    # write
        pci_write(pci_dev_path, addr, args[3])
    else:
        exception_quit("Error: Bad arguments number", 7)


# args: 0 - sbdf, 1 - device_offset, 2 - addr, 3 - data, 4 - write check
def indir_rw_data(*args):
    addr = ((args[1] << 17) | args[2]) << 32
    rdata = 0
    if len(args) == 3:      # read
        cmd = 0x4 << 60
        rw_data(args[0], 0x0, INDIRECT_CTRL_REG, cmd | addr, 0)
        while (rdata >> 32) != 0x1:     # waiting for read valid
            rdata = rw_data(args[0], 0x0, INDIRECT_DATA_REG)      # rdata valid
        return rdata & 0xffffffff
    elif len(args) == 5:    # write
        cmd = 0x8 << 60
        rw_data(args[0], 0x0, INDIRECT_CTRL_REG, cmd | addr | args[3], 0)
        while (rdata >> 32) != 0x1:     # waiting for write complete
            rdata = rw_data(args[0], 0x0, INDIRECT_DATA_REG)      # rdata valid
        rdata = 0
        if args[4]:
            cmd = 0x4 << 60
            rw_data(args[0], 0x0, INDIRECT_CTRL_REG, cmd | addr, 0)
            while (rdata >> 32) != 0x1:     # waiting for read valid
                rdata = rw_data(args[0], 0x0, INDIRECT_DATA_REG)  # rdata valid
            if args[3] != (rdata & 0xffffffff):
                print('{:#x} {:#x}'.format(args[3], rdata))
                exception_quit("Error: failed comparison of wrote data", 8)
    else:
        exception_quit("Error: Bad arguments number", 7)


def get_sbdf_mode_mapping(sbdf, args):
    global VC_INFO

    sysfs_path = glob.glob(os.path.join('/sys/bus/pci/devices', sbdf,
                                        'fpga', 'intel-fpga-dev.*',
                                        'intel-fpga-fme.*', 'bitstream_id'))
    if len(sysfs_path) == 0:
        exception_quit("Error: bitstream_id not found", 4)

    with open(sysfs_path[0], 'r') as f:
        bitstream_id = f.read().strip()

    build_flags = (int(bitstream_id, 16) >> 24) & 0xff
    if (build_flags & 0x01) == 0x00:
        exception_quit("FPGA {} does not support bypass mode".format(sbdf), 5)

    vc_info['mode'] = (int(bitstream_id, 16) >> 32) & 0xf
    print('Mode: {}'.format(VC_MODE_NAME.get(vc_info['mode'], 'unknown')))

    if vc_info['mode'] == VC_MODE_8x10G:
        vc_info['total_mac'] = 8
        vc_info['demux_offset'] = 0x100
    elif vc_info['mode'] == VC_MODE_2x1x25G:
        vc_info['total_mac'] = 2
        vc_info['demux_offset'] = 0x40
    elif vc_info['mode'] == VC_MODE_2x2x25G:
        vc_info['total_mac'] = 4
        vc_info['demux_offset'] = 0x80
    else:
        exception_quit("FPGA {} not support bypass mode".format(sbdf), 5)

    c = COMMON()
    args.ports = c.get_port_list(args.port, vc_info.get('total_mac'))

    sysfs_path = glob.glob(os.path.join('/sys/bus/pci/devices', sbdf,
                                        'fpga', 'intel-fpga-dev.*',
                                        'intel-fpga-fme.*',
                                        'bitstream_metadata'))
    if len(sysfs_path) == 0:
        exception_quit("Error: bitstream_id not found", 4)

    with open(sysfs_path[0], 'r') as f:
        bitstream_md = int(f.read().strip(), 16)
    seed = (bitstream_md >> 4) & 0xfff
    print("Seed: {:#x}".format(seed))


def get_sbdf_upl_mapping(sbdf):
    global vc_info

    pci_dev_path = '/sys/bus/pci/devices/{}/resource2'.format(sbdf)
    addr = 0
    while True:
        header = pci_read(pci_dev_path, addr)
        feature_type = (header >> DFH_TYPE_SHIFT) & DFH_TYPE_MASK
        feature_id = (header >> DFH_ID_SHIFT) & DFH_ID_MASK
        if feature_type == DFH_TYPE_AFU and feature_id == DFH_ID_UPL:
            uuid_l = pci_read(pci_dev_path, addr+UUID_L_OFFSET_REG)
            uuid_h = pci_read(pci_dev_path, addr+UUID_H_OFFSET_REG)
            if uuid_l == UPL_UUID_L and uuid_h == UPL_UUID_H:
                vc_info['upl_base'] = addr
                break
            else:
                msg = "FPGA {} has no packet generator for test".format(sbdf)
                exception_quit(msg, 6)
        if feature_type in [DFH_TYPE_AFU, DFH_TYPE_FIU]:
            next_afu_offset = pci_read(pci_dev_path, addr+NEXT_AFU_OFFSET_REG)
            next_afu_offset &= NEXT_AFU_OFFSET_MASK
        if next_afu_offset == 0:
            exception_quit("Error: UPL not found in FPGA {}".format(sbdf), 6)
        else:
            addr += next_afu_offset
            next_afu_offset = 0


def clear_stats(f, info, args):
    global vc_info

    if args.clear:
        print('Clearing statistics of MACs ...')

        vc_mode = vc_info.get('mode', None)
        if vc_mode is None:
            exception_quit("FPGA is not in bypass mode", 5)
        offset = vc_info.get('demux_offset', 0x100)

        for w in info:
            _, mac_total, _, node = info[w]
            with open(node, 'r') as fd:
                for i in args.ports:
                    if vc_mode == VC_MODE_8x10G:
                        f.fpga_eth_reg_write(fd, 'mac', i, 0x140, 0x1)
                        f.fpga_eth_reg_write(fd, 'mac', i, 0x1C0, 0x1)
                    else:
                        f.fpga_eth_reg_write(fd, 'mac', i, 0x845, 0x1)
                        f.fpga_eth_reg_write(fd, 'mac', i, 0x945, 0x1)
                    reg = 0x1 + i * 8
                    f.fpga_eth_reg_write(fd, 'eth', 0, reg, 0x0)
                    f.fpga_eth_reg_write(fd, 'eth', 0, offset + reg, 0x0)
                    time.sleep(0.1)


def enable_loopback(args):
    if args.loopback:
        args.direction = 'local'
        args.side = 'line'
        args.type = 'serial'
        args.en = 1
        fl = FPGALPBK(args)
        fl.start()
        if args.debug:
            print('Loopback enabled')
        time.sleep(0.1)


def disable_loopback(args):
    if args.loopback:
        args.direction = 'local'
        args.side = 'line'
        args.type = 'serial'
        args.en = 0
        fl = FPGALPBK(args)
        fl.start()
        if args.debug:
            print('Loopback disabled')
        time.sleep(0.1)


def test_wait(sbdf, timeout, args):
    left_time = timeout
    while left_time > 0:
        sleep_time = 0.1 if left_time > 0.1 else left_time
        time.sleep(sleep_time)
        left_time -= sleep_time
        if indir_rw_data(sbdf, 0, 0x100F) == 1:
            if args.debug:
                period = timeout - left_time
                rate = (args.number * args.length * 8) / period / 1e9
                print('Time consuming {}s, data rate {:.2f}Gbps'.format(period,
                                                                        rate))
            break


def fvl_bypass_mode_test(sbdf, args):
    global vc_info

    get_sbdf_upl_mapping(sbdf)

    fs = FPGASTATS(args)
    info = fs.get_eth_group_info(fs.eth_grps)
    clear_stats(fs, info, args)

    indir_rw_data(sbdf, 0, 0x1011, 0xff, 0)         # clear monitor statistic
    indir_rw_data(sbdf, 0, 0x1000, 0xAAAAAAAA, 0)   #
    indir_rw_data(sbdf, 0, 0x1001, 0xAAAAAAAA, 0)   # DST MAC to generator
    indir_rw_data(sbdf, 0, 0x1002, 0xBBBBBBBB, 0)   #
    indir_rw_data(sbdf, 0, 0x1003, 0xBBBBBBBB, 0)   # SRC MAC to generator
    indir_rw_data(sbdf, 0, 0x1009, 0xAAAAAAAA, 0)   #
    indir_rw_data(sbdf, 0, 0x100A, 0xAAAAAAAA, 0)   # DST MAC to monitor
    indir_rw_data(sbdf, 0, 0x100B, 0xBBBBBBBB, 0)   #
    indir_rw_data(sbdf, 0, 0x100C, 0xBBBBBBBB, 0)   # SRC MAC to monitor
    indir_rw_data(sbdf, 0, 0x1005, args.length, 0)  # set GEN_PKT_LENGTH
    indir_rw_data(sbdf, 0, 0x1006, 0x1, 0)          # set EN_PKT_DELAY
    time.sleep(0.1)

    mac_total = vc_info.get('total_mac', 2)
    if 'all' in args.port:
        pkt_num = mac_total * args.number   # packets num to be sent on all mac
        wait_time = pkt_num * args.length / 1e9
        if args.debug:
            print('Timeout is set to {}s'.format(wait_time))
        print('Sending packets to all ports ...')
        indir_rw_data(sbdf, 0, 0x1004, pkt_num, 0)    # set GEN_PKT_NUMBER
        indir_rw_data(sbdf, 0, 0x100D, pkt_num, 0)    # set MON_PKT_NUMBER
        indir_rw_data(sbdf, 0, 0x100E, 0x5, 0)        # set MON_PKT_CTRL
        indir_rw_data(sbdf, 0, 0x1010, 0x0, 0)        # set CH_GEN_CH_FUNC
        indir_rw_data(sbdf, 0, 0x1007, 0x5, 0)        # set GEN_PKT_CTRL
        test_wait(sbdf, wait_time, args)              # wait sending
    else:
        pkt_num = args.number
        wait_time = pkt_num * args.length / 5e8
        if args.debug:
            print('Timeout is set to {}s'.format(wait_time))
        for i in args.ports:                # generate packets on each channel
            print('Sending packets to port {} ...'.format(i))
            indir_rw_data(sbdf, 0, 0x1004, pkt_num, 0)  # set GEN_PKT_NUMBER
            indir_rw_data(sbdf, 0, 0x100D, pkt_num, 0)  # set MON_PKT_NUMBER
            pkt_ch = 1 | (i << 4)                       # cnt channels
            indir_rw_data(sbdf, 0, 0x100E, 0x5, 0)      # set MON_PKT_CTRL
            indir_rw_data(sbdf, 0, 0x1010, pkt_ch, 0)   # set CH_GEN_CH_FUNC
            indir_rw_data(sbdf, 0, 0x1007, 0x5, 0)      # set GEN_PKT_CTRL
            test_wait(sbdf, wait_time, args)      # waiting sending per channel

    for i in args.ports:                    # checking each channel statistic
        pkt_ch = 0x1012 + i                       # address pkt_cnt for channel
        pkt_cnt = indir_rw_data(sbdf, 0, pkt_ch)  # read channel statistic
        if pkt_cnt != args.number:          # if statistic don't equal sent pkt
            print('port {} rx {}'.format(i, pkt_cnt))
            print('bypass mode test failed !!!')
            ret = 9
            break
    else:
        print('bypass mode test successed !!!')
        ret = 0

    if args.debug:
        fs.print_stats(info)
    clear_stats(fs, info, args)

    sys.exit(ret)


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
    parser.add_argument('--number', '-n', default=DEFAULT_TEST_PKT_NUM,
                        help='Number of the test packets to send per MAC')
    parser.add_argument('--length', '-s', default=DEFAULT_TEST_PKT_LEN,
                        help='Length of each test packet')
    parser.add_argument('--loopback', '-l', action='store_true',
                        help='Configure loopback automatically.'
                             'Loopback is not configured by default.')
    parser.add_argument('--clear', '-c', action='store_true',
                        help='Clear statistics automatically.'
                             'Statistics are not cleared by default.')
    parser.add_argument('--port', '-p', nargs='*', default='all',
                        help='Test on selected MACs')
    parser.add_argument('--debug', '-d', action='store_true',
                        help='Output debug information')
    args, left = parser.parse_known_args()


    setattr(args, 'number', int(getattr(args, 'number')))
    if args.number < MIN_TEST_PKT_NUM or args.number > MAX_TEST_PKT_NUM:
        setattr(args, 'number', DEFAULT_TEST_PKT_NUM)
        print(('The number of test packets is out of range ({}~{})'
              ', use {} instead'.format(MIN_TEST_PKT_NUM, MAX_TEST_PKT_NUM,
                                        DEFAULT_TEST_PKT_NUM)))
    setattr(args, 'length', int(getattr(args, 'length')))
    if args.length < MIN_TEST_PKT_LEN or args.length > MAX_TEST_PKT_LEN:
        setattr(args, 'length', DEFAULT_TEST_PKT_LEN)
        print(('The length of test packet is out of range ({}~{})'
              ', use {} instead'.format(MIN_TEST_PKT_LEN, MAX_TEST_PKT_LEN,
                                        DEFAULT_TEST_PKT_LEN)))

    f = FpgaFinder(args.segment, args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        sbdf = '{segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**d)
        print('DUT: {}'.format(sbdf))
    if len(devs) > 1:
        exception_quit('{} FPGAs are found\n'
                       'please choose one FPGA'.format(len(devs)), 1)
    if not devs:
        exception_quit('no FPGA found', 2)
    args.eth_grps = f.find_node(devs[0].get('path'), 'eth_group*/dev', depth=4)
    if not args.eth_grps:
        exception_quit('No ethernet group found', 3)
    if args.debug:
        for g in args.eth_grps:
            print('ethernet group device: {}'.format(g))

    get_sbdf_mode_mapping(sbdf, args)
    lock_file = '/tmp/DUT{}'.format(sbdf)
    if os.path.exists(lock_file):
        exception_quit("FPGA {} is already in test".format(sbdf), 0)

    try:
        with open(lock_file, 'w') as fd:
            fd.write(sbdf)
        enable_loopback(args)
        fvl_bypass_mode_test(sbdf, args)
    finally:
        disable_loopback(args)
        if os.path.exists(lock_file):
            os.remove(lock_file)
        print('Done')


if __name__ == "__main__":
    main()
