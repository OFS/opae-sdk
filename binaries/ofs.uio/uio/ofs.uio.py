#! /usr/bin/env python3
# Copyright(c) 2022, Intel Corporation
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

import re
import os
import glob
import argparse
import sys
import time
import mmap
import binascii
import struct
from ctypes import c_uint64, Structure, Union, c_uint32
from pyopaeuio import pyopaeuio

MAPSIZE = mmap.PAGESIZE
MAPMASK = MAPSIZE-1

PATTERN = (r'.*(?P<segment>\w{4}):(?P<bus>\w{2}):'
           r'(?P<dev>\w{2})\.(?P<func>\d).*')

FPGA_ROOT_PATH = '/sys/class/fpga_region'

BDF_PATTERN = re.compile(PATTERN, re.IGNORECASE)

DEFAULT_BDF = 'ssss:bb:dd.f'

# PCIe SS Feature id and Mailbox csr offset
PCIESS_FEATURE_ID = 0x20
MAILBOX_CMD_CSR = 0x28
MAILBOX_RD_DATA_CSR = 0x30
MAILBOX_WR_DATA_CSR = 0x31

# mailbox register poll interval 1 microseconds
MAILBOX_POLL_SLEEP_TIME = 1/1000000

# mailbox register poll timeout 100 microseconds
MAILBOX_POLL_TIMEOUT = 1/10000


def verify_pcie_address(pcie_address):
    m = BDF_PATTERN.match(pcie_address)
    if m is None:
        print("Invalid pcie address format{}".format(pcie_address))
        return False
    return True


class verify_input_hex(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        regx = re.compile("0[xX][0-9a-fA-F]+")
        # hex single value
        if isinstance(values, str):
            try:
                if re.fullmatch(regx, values) is None:
                    parser.error(f"Invalid input: {values}")
                setattr(namespace, self.dest, int(values, 16))
            except ValueError:
                parser.error(f"Invalid input: {values}")
        # input hex list
        elif isinstance(values, list):
            for value in values:
                try:
                    if re.fullmatch(regx, value) is None:
                        parser.error(f"Invalid input: {values}")
                except ValueError:
                    parser.error(f"Invalid input: {values}")
            setattr(namespace, self.dest, values)

        else:
            parser.error(f"Invalid input: {values}")
            setattr(namespace, self.dest, values)


def verify_uio(str):
    regx = re.compile("uio[0-9]+")
    try:
        if re.search(regx, str) is not None:
            return str
        else:
            raise argparse.ArgumentTypeError('Invalid input:', str)
    except ValueError:
        raise argparse.ArgumentTypeError('Invalid input:', str)


class FpgaFinder(object):
    """
    Enum FPGA devices and Feature ids
    """
    def __init__(self, pcie_address):
        self._pice_address = pcie_address
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
                sbdf = self.read_bdf(os.path.join(p, 'device'))
                if sbdf:
                    sbdf['path'] = p
                    pcie_address = None
                    sbdf_str = '{0:04x}:{1:02x}:{2:02x}.{3:01x}'
                    pcie_address = sbdf_str.format(sbdf.get('segment'),
                                                   sbdf.get('bus'),
                                                   sbdf.get('dev'),
                                                   sbdf.get('func'))
                    sbdf['pcie_address'] = pcie_address
                    if self._pice_address == pcie_address:
                        self.all_devs.append(sbdf)
                    if self._pice_address is None:
                        self.all_devs.append(sbdf)

    def enum(self):
        if not self.all_devs:
            print('No FPGA device found at {}'.format(FPGA_ROOT_PATH))
        for dev in self.all_devs:
            self.match_dev.append(dev)
        return self.match_dev

    def find_node(self, root, node, depth=5):
        paths = []
        for x in range(depth):
            r = glob.glob(os.path.join(os.path.join(root, *['*'] * x), node))
            paths.extend(r)
        return paths

    def find_uio_feature(self, pci_address, uio_feature_id):
        uio = []
        paths = glob.glob(os.path.join("/sys/bus/pci/devices/",
                                       pci_address,
                                       "fpga_region/region*/dfl-fme*/dfl_dev*"))
        feature_id = 0
        uio_path = 0
        for path in paths:
            with open(os.path.join(path, 'feature_id'), 'r') as fd:
                feature_id = fd.read().strip()
                feature_id = int(feature_id, 16)

            if feature_id != uio_feature_id:
                continue

            uio_path = glob.glob(os.path.join(path, "uio/uio*"))
            if len(uio_path) == 0:
                continue
            m = re.search('dfl_dev(.*)', path)
            if m:
                uio.append((m.group(0), uio_path, uio_path[0].rsplit('/', 1)[1], pci_address, feature_id))
        return uio


class mailbox_cmd_sts_bits(Structure):
    """
    Mailbox Command/Status CSR bits
    """
    _fields_ = [
                   ("read_cmd", c_uint64, 1),
                   ("write_cmd", c_uint64, 1),
                   ("ack_trans", c_uint64, 1),
                   ("reserved", c_uint64, 29),
                   ("cmd_addr", c_uint64, 32)
    ]


class mailbox_cmd_sts(Union):
    """
    Mailbox Command/Status
    Addressing Mode: 64 bits
    """
    _fields_ = [("bits", mailbox_cmd_sts_bits),
                ("value", c_uint64)]

    def __init__(self, value):
        self.value = value

    @property
    def read_cmd(self):
        return self.bits.read_cmd

    @property
    def write_cmd(self):
        return self.bits.write_cmd

    @property
    def ack_trans(self):
        return self.bits.ack_trans

    @property
    def cmd_addr(self):
        return self.bits.cmd_addr

    @read_cmd.setter
    def read_cmd(self, value):
        self.bits.read_cmd = value

    @write_cmd.setter
    def write_cmd(self, value):
        self.bits.write_cmd = value

    @cmd_addr.setter
    def cmd_addr(self, value):
        self.bits.cmd_addr = value


class mailbox_data_bits(Structure):
    """
    Mailbox read/write CSR bits
    """
    _fields_ = [
                   ("read_data", c_uint64, 32),
                   ("write_data", c_uint64, 32),
    ]


class mailbox_data(Union):
    """
    Mailbox data read write
    Addressing Mode: 64 bits
    """
    _fields_ = [("bits", mailbox_data_bits),
                ("value", c_uint64)]

    def __init__(self, value):
        self.value = value

    @property
    def read_data(self):
        return self.bits.read_data

    @property
    def write_data(self):
        return self.bits.write_data

    @write_data.setter
    def write_data(self, value):
        self.bits.write_data = value


class UIO(object):
    """
    enum uio dev info name, offset, size
    read/write uio csr
    Mailbox read/write
    """
    def __init__(self, uio, bit_size, region_index, mailbox_cmdcsr):
        self.uio = uio
        self.pyopaeuio_inst = pyopaeuio()
        self.num_regions = 0
        self.region = []
        self.bit_size = bit_size
        self.region_index = region_index
        self.MAILBOX_CMD_CSR = mailbox_cmdcsr
        self.MAILBOX_RD_DATA_CSR = mailbox_cmdcsr + 0x8
        self.MAILBOX_WR_DATA_CSR = mailbox_cmdcsr + 0xC

    def verify_uio(self):
        uio_path = os.path.join("/sys/class/uio/", self.uio)
        if os.path.exists(uio_path):
            with open(os.path.join(uio_path, 'name'), 'r') as fd:
                print("UIO Name:", fd.read().strip())
        else:
            print("Invalid input", self.uio)
            return False

        uio_maps = glob.glob(os.path.join("/sys/class/uio/",
                                          self.uio,
                                          "maps/map*"))
        self.num_regions = len(uio_maps)
        if self.num_regions == 0:
            print("Invalid memory regions", self.num_regions)
            return False

        for map in uio_maps:
            map_dict = dict()
            with open(os.path.join(map, 'name'), 'r') as fd:
                name = fd.read().strip()
                map_dict["name"] = name

            with open(os.path.join(map, 'offset'), 'r') as fd:
                offset = fd.read().strip()
                map_dict["offset"] = int(offset, 16)

            with open(os.path.join(map, 'size'), 'r') as fd:
                size = fd.read().strip()
                map_dict["size"] = int(size, 16)

            self.region.append(map_dict)

        print("UIO Regions", self.region)
        return True

    def open(self):
        ret = self.pyopaeuio_inst.open(self.uio)
        return ret

    def close(self):
        ret = self.pyopaeuio_inst.close()
        return ret

    def read8(self, region_index, offset):

        if offset >= self.region[region_index]['size']:
            raise Exception("Invalid Input offset", hex(offset))
        value = self.pyopaeuio_inst.read8(region_index, offset)
        return value

    def read16(self, region_index, offset):

        if offset >= self.region[region_index]['size']:
            raise Exception("Invalid Input offset", hex(offset))
        value = self.pyopaeuio_inst.read16(region_index, offset)
        return value

    def read32(self, region_index, offset):

        if offset >= self.region[region_index]['size']:
            raise Exception("Invalid Input offset", hex(offset))
        value = self.pyopaeuio_inst.read32(region_index, offset)
        return value

    def read64(self, region_index, offset):
        if offset >= self.region[region_index]['size']:
            raise Exception("Invalid Input offset", hex(offset))
        value = self.pyopaeuio_inst.read64(region_index, offset)
        return value

    def write8(self, region_index, offset, value):

        if offset >= self.region[region_index]['size']:
            raise Exception("Invalid Input offset", hex(offset))
        ret = self.pyopaeuio_inst.write8(region_index, offset, value)
        return ret

    def write16(self, region_index, offset, value):

        if offset >= self.region[region_index]['size']:
            raise Exception("Invalid Input offset", hex(offset))
        ret = self.pyopaeuio_inst.write16(region_index, offset, value)
        return ret

    def write32(self, region_index, offset, value):
        if offset >= self.region[region_index]['size']:
            raise Exception("Invalid Input offset", hex(offset))
        ret = self.pyopaeuio_inst.write32(region_index, offset, value)
        return ret

    def write64(self, region_index, offset, value):
        if offset >= self.region[region_index]['size']:
            raise Exception("Invalid Input offset", hex(offset))
        ret = self.pyopaeuio_inst.write64(region_index, offset, value)
        return ret

    def register_field_set(self, reg_data, idx, width, value):
        mask = 0
        for x in range(width):
            mask |= (1 << x)
        value &= mask
        reg_data &= ~(mask << idx)
        reg_data |= (value << idx)
        return reg_data

    def register_field_get(self, reg_data, idx):
        value = ((reg_data >> idx) & (1))
        return value

    def register_get_bits(self, reg_data, idx, width):
        value = 0
        for x in range(width):
            value |= (reg_data & (1 << (idx + x)))
        return value

    def read_poll_timeout(self,
                          region_index,
                          reg_offset,
                          bit_index):
        """
        poll CMD ack
        """
        total_time = 0
        while True:
            reg_data = self.read32(region_index, reg_offset)
            if ((reg_data >> bit_index) & 1) == 1:
                return True
            time.sleep(MAILBOX_POLL_SLEEP_TIME)
            if total_time > MAILBOX_POLL_TIMEOUT:
                return False
            total_time = MAILBOX_POLL_SLEEP_TIME + total_time

        return False

    def mailbox_read(self, region_index, address):
        """
        clear cmd sts csr
        Set read bit and write cmd address
        poll ack bits
        read data
        clear cmd sts csr
        """

        # clear cmd sts csr
        self.write64(region_index, self.MAILBOX_CMD_CSR, 0)
        time.sleep(MAILBOX_POLL_TIMEOUT)

        # Set read bit and write cmd address
        mbox_cmd_sts = mailbox_cmd_sts(0x1)
        mbox_cmd_sts.cmd_addr = address
        self.write64(region_index, self.MAILBOX_CMD_CSR, mbox_cmd_sts.value)

        # Poll for ACK_TRANS bit index 2, wdith 2
        if not self.read_poll_timeout(region_index,
                                      self.MAILBOX_CMD_CSR,
                                      2):
            print("MailBox cmd sts fails to update ACK")
            return False, -1

        # Read data
        value = self.read32(region_index, self.MAILBOX_RD_DATA_CSR)
        # clear cmd sts csr
        self.write64(region_index, self.MAILBOX_CMD_CSR, 0)
        time.sleep(MAILBOX_POLL_TIMEOUT)
        return True, value

    def mailbox_write(self, region_index, address, value):
        """
        clear cmd sts csr
        Set write bit and write cmd address
        poll ack bits
        write data
        clear cmd sts csr
        """

        # clear cmd sts csr
        self.write64(region_index, self.MAILBOX_CMD_CSR, 0x0)
        time.sleep(MAILBOX_POLL_TIMEOUT)

        # Set write bit and write cmd address
        mbox_cmd_sts = mailbox_cmd_sts(0x2)
        mbox_cmd_sts.cmd_addr = address
        self.write64(region_index, self.MAILBOX_CMD_CSR, mbox_cmd_sts.value)

        # write value
        self.write32(region_index, self.MAILBOX_WR_DATA_CSR, value)

        # Poll for ACK_TRANS bit index 2, wdith 2
        if not self.read_poll_timeout(region_index,
                                      self.MAILBOX_CMD_CSR,
                                      2):
            print("MailBox cmd sts fails to update ACK")
            return False

        # clear cmd sts csr
        self.write64(region_index, self.MAILBOX_CMD_CSR, 0x0)
        time.sleep(MAILBOX_POLL_TIMEOUT)
        return True

    def peek(self, region_index, bit_size, offset):
        if bit_size == 8:
            return self.read8(region_index, offset)
        elif bit_size == 16:
            return self.read16(region_index, offset)
        elif bit_size == 32:
            return self.read32(region_index, offset)
        elif bit_size == 64:
            return self.read64(region_index, offset)
        else:
            return self.read64(region_index, offset)

    def poke(self, region_index, bit_size, offset, value):
        if bit_size == 8:
            return self.write8(region_index, offset, value)
        elif bit_size == 16:
            return self.write16(region_index, offset, value)
        elif bit_size == 32:
            return self.write32(region_index, offset, value)
        elif bit_size == 64:
            return self.write64(region_index, offset, value)
        else:
            return self.write64(region_index, offset, value)


def parse_args():
    """
    parse input arguments pciaddress, bit filed size, peek, poke
    uio, feature id, region index, mailbox csr offset,
    mailbox read, mailbox bump and mailbox write
    """
    parser = argparse.ArgumentParser()

    pcieaddress_help = 'sbdf of device to program \
                        (e.g. 0000:04:00.0).' \
                       ' Optional when one device in system.'
    parser.add_argument('--pcie-address', '-P',
                        default=None, help=pcieaddress_help)

    parser.add_argument('--uio', default=None, type=verify_uio,
                        help='uio dev name (e.g.--uio uio0 --peek 0x8 )',
                        metavar='uio')

    feature_id_help = 'DFL UIO Feature id\
                       (e.g.--feature-id 0x20 --peek 0x8 )'
    parser.add_argument('--feature-id', '-f', action=verify_input_hex,
                        default=PCIESS_FEATURE_ID,
                        help=feature_id_help)

    region_index_help = 'dfl uio region index \
                         (e.g.--uio uio0 --region-index 0x0 --peek 0x8 ).\
                         (e.g.--uio uio0 --region-index 0x1 --poke 0x8 0x123)'
    parser.add_argument('--region-index', '-r', type=int,
                        default=0, help=region_index_help)

    mailbox_cmd_csr_help = 'Mailbox command CSR offset \
                            (e.g.--mailbox-cmdcsr 0x40 --mailbox-read 0x8 ).\
                            (e.g.--mailbox-cmdcsr 0x40 --mailbox-write 0x8 0x123)'
    parser.add_argument('--mailbox-cmdcsr', action=verify_input_hex,
                        default=MAILBOX_CMD_CSR, metavar='offset',
                        help=mailbox_cmd_csr_help)

    bit_help = 'peek/poke bit field size \
                (e.g.--bit-size 64 --peek 0x8).\
                (e.g.--bit-size 32 --poke 0x8 0xabc) '
    parser.add_argument('--bit-size', '-b', type=int,
                        default=64, choices=[8, 16, 32, 64],
                        help=bit_help)

    poke_help = 'Peek CSR offset \
                 (e.g.--peek 0x8).'
    parser.add_argument('--peek', action=verify_input_hex,
                        default=None, metavar='offset',
                        help=poke_help)

    peek_help = 'Poke CSR offset value \
                 (e.g.--poke 0x8 0xabcd).'
    parser.add_argument('--poke', action=verify_input_hex,
                        default=None, nargs=2, metavar=('offset', 'value'),
                        help=peek_help)

    mailbox_read_help = 'Read Mailbox CSR address \
                        (e.g.--mailbox-read 0x8).'
    parser.add_argument('--mailbox-read', action=verify_input_hex,
                        default=None, metavar='offset',
                        help=mailbox_read_help)

    mailbox_dump_help = 'Reads Mailbox CSR start address size \
                        (e.g.--mailbox-dump 0x0 0x20).'
    parser.add_argument('--mailbox-dump', action=verify_input_hex,
                        default=None, nargs=2, metavar=('address', 'size'),
                        help=mailbox_dump_help)

    mailbox_write_help = 'Write Mailbox CSR address Value \
                        (e.g.--mailbox-write 0x8 0x1234).'
    parser.add_argument('--mailbox-write', action=verify_input_hex,
                        default=None, nargs=2, metavar=('address', 'value'),
                        help=mailbox_write_help)

    return parser, parser.parse_args()


def main():
    """
    parse input arguments,enum fpga pcie devices and
    finds match uio feature id
    supports peek/poke csr,mailbox read/mailbox write/mailbox dump
    """
    parser, args = parse_args()
    print("\n*****************************")
    print('args:', args)
    if all(arg is None for arg in [args.peek, args.poke, args.mailbox_read,
                                   args.mailbox_write, args.mailbox_dump]):
        print('Error: please pass the proper arguments\n\n')
        parser.print_help(sys.stderr)
        sys.exit(1)

    if args.pcie_address and not verify_pcie_address(args.pcie_address.lower()):
        sys.exit(1)

    args.uio_grps = []
    # enum FPGA
    f = FpgaFinder(args.pcie_address.lower() if args.pcie_address else None)
    devs = f.enum()
    if not devs:
        print('no FPGA found')
        sys.exit(1)

    if len(devs) > 1:
        s = '{} FPGAs are found\nplease choose one FPGA'.format(len(devs))
        sys.exit(1)

    # enum FPGA UIO Features if args.uio is none
    if args.uio is None:
        for d in devs:
            print('sbdf: {segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**d))
            args.uio_grps += f.find_uio_feature(d['pcie_address'], args.feature_id)
        if len(args.uio_grps) == 0:
            print("Failed to find HSSI feature")
            sys.exit(1)
        if len(args.uio_grps) > 1:
            print('{} FPGAs UIO feature matchs are found: {}'
                  .format(len(args.uio_grps), [d[0] for d in args.uio_grps]))
            sys.exit(1)

    try:
        # open uio
        if args.uio is not None:
            uio = args.uio
        else:
            uio = args.uio_grps[1][2]
        # create UIO instance
        uio = UIO(uio, args.bit_size, args.region_index,
                  args.mailbox_cmdcsr)
        if not uio.verify_uio():
            print('Invalid input')
            sys.exit(1)
        print("*****************************\n")
        uio.open()
        # peek/read csr
        if args.peek is not None:
            value = uio.peek(args.region_index, args.bit_size, args.peek)
            print('peek({}): {}'.format(hex(args.peek), hex(value)))

        # mailbox read
        elif args.mailbox_read is not None:
            status, value = uio.mailbox_read(args.region_index, args.mailbox_read)
            if not status:
                print('Failed to Read Mailbox CSR address {}'
                      .format(hex(args.mailbox_read)))
            else:
                print('MailboxRead({}): {}'
                      .format(hex(args.mailbox_read), hex(value)))

        # poke/write csr
        elif args.poke is not None:
            uio.poke(args.region_index, args.bit_size, int(args.poke[0], 16),
                     int(args.poke[1], 16))
            value = uio.read64(0, int(args.poke[0], 16))
            print('poke({}):{}'.format(args.poke[0], hex(value)))

        # mailbox write
        elif args.mailbox_write is not None:
            if not uio.mailbox_write(args.region_index, int(args.mailbox_write[0], 16),
                                     int(args.mailbox_write[1], 16)):
                print('Failed to write Mailbox CSR address {}'
                      .format(int(args.mailbox_write[0], 16)))
            else:
                status, value = uio.mailbox_read(0,
                                                 int(args.mailbox_write[0], 16))
                print('MailboxWrite({}):{}'
                      .format(int(args.mailbox_write[0], 16), hex(value)))

        # mailbox dump
        elif args.mailbox_dump is not None:
            for i in range(int(args.mailbox_dump[1], 16)):
                addr = int(args.mailbox_dump[0], 16) + i * 0x4
                status, value = uio.mailbox_read(args.region_index, addr)
                if not status:
                    print('Failed to Dump Mailbox CSR address {}'
                          .format(addr))
                else:
                    print('MailboxDump({}): {}'
                          .format(hex(addr), hex(value)))

    except Exception as e:
        print(e)
    finally:
        # close uio
        uio.close()
    return 0


if __name__ == "__main__":
    main()
