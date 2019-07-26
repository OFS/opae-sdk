#! /usr/bin/env python
# Copyright(c) 2019, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
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

import argparse
import struct
import fnmatch
import glob
import sys
import tempfile
import os
import fcntl
import filecmp
import stat
import subprocess
import re
import time
import datetime
from array import array
from subprocess import check_output, CalledProcessError

try:
    from intelhex import IntelHex
except ImportError:
    sys.exit('Missing intelhex. Install by: sudo pip install intelhex')

RC_PVID = '0x8086:0x09c4'
DC_PVID = '0x8086:0x0b2b'
VC_PVID = '0x8086:0x0b30'

KNOWN_PVIDS = [RC_PVID, DC_PVID, VC_PVID]

RC = 'Intel PAC with Arria 10 GX FPGA'

VC_EEPROM_SIZE = 65536

VC_FPGA_IMAGE_MAGIC_NUM1 = 0x2000862a
VC_FPGA_IMAGE_MAGIC_NUM2 = 0x0040666a
VC_FPGA_IMAGE_LENGTH = 0x77f0000

VC_MAX10_IMAGE_MAGIC_NUM = 0x56565656
VC_MAX10_IMAGE_LENGTH = 0xa8000

VC_DC_DTB_BLOCK_OFFSET = 0x3810000
VC_DC_DTB_BLOCK_SIZE = 0x7e0000

DTB_REVERSED_MAGIC_NUM = 0x0bb07fb7

VC_DC_FACTORY_BLOCK_OFFSET = 0x10000
VC_DC_FACTORY_BLOCK_SIZE = 0x3800000

BMC_FW_HEAD_SIZE = 256


def assert_not_running(programs):
    try:
        check_output(["pidof"] + programs)
    except CalledProcessError:
        return 0

    errmsg = "\nERROR: Please shut down {} before running fpgaflash\n".format(
        ", ".join(programs))
    raise SystemExit(errmsg)


def check_rpd(ifile):
    data = ifile.read(0x20)
    pof_hdr = struct.unpack('IIIIIIII', data)
    for i in range(0, 3):
        if pof_hdr[i] != 0xffffffff:
            print "invalid rpd file"
            raise Exception

    if pof_hdr[3] != 0x6a6a6a6a:
        print "invalid rpd file"
        raise Exception

    return pof_hdr[4]


def reverse_bits(x, n):
    result = 0
    for i in xrange(n):
        if (x >> i) & 1:
            result |= 1 << (n - 1 - i)
    return result


def reverse_bits_in_file(ifile, ofile):
    bit_rev = array('B')
    for i in range(0, 256):
        bit_rev.append(reverse_bits(i, 8))

    while True:
        ichunk = ifile.read(4096)
        if not ichunk:
            break

        ochunk = ''
        for b in ichunk:
            ochunk += chr(bit_rev[ord(b)])
        ofile.write(ochunk)


def get_flash_size(dev):

    MEMGETINFO = 0x80204d01

    ioctl_data = struct.pack('BIIIIIQ', 0, 0, 0, 0, 0, 0, 0)

    with os.fdopen(os.open(dev, os.O_SYNC | os.O_RDONLY), 'r') as file_:
        ret = fcntl.ioctl(file_.fileno(), MEMGETINFO, ioctl_data)

    ioctl_odata = struct.unpack_from('BIIIIIQ', ret)

    return ioctl_odata[2]


def flash_erase(dev, start, nbytes):
    MEMERASE = 0x40084d02

    print "%s erasing 0x%08x bytes starting at 0x%08x" % (
        datetime.datetime.now(), nbytes, start)

    ioctl_data = struct.pack('II', start, nbytes)
    with os.fdopen(os.open(dev, os.O_SYNC | os.O_RDWR), 'w') as file_:
        fcntl.ioctl(file_.fileno(), MEMERASE, ioctl_data)


def flash_write(dev, start, nbytes, ifile):
    print "%s writing 0x%08x bytes to 0x%08x" % (
        datetime.datetime.now(), nbytes, start)

    last_write_position = start
    with os.fdopen(os.open(dev, os.O_SYNC | os.O_RDWR), 'a') as file_:
        os.lseek(file_.fileno(), start, os.SEEK_SET)

        while nbytes > 0:
            if nbytes > 4096:
                rbytes = 4096
            else:
                rbytes = nbytes

            ichunk = ifile.read(rbytes)

            if not ichunk:
                raise Exception("read of flash failed")

            if all([c == '\xFF' for c in ichunk]):
                os.lseek(file_.fileno(), rbytes, os.SEEK_CUR)
            else:
                os.write(file_.fileno(), ichunk)
                last_write_position = file_.tell()

            nbytes -= rbytes

    bytes_written = last_write_position - start
    print "%s actual bytes written 0x%x - 0x%x = 0x%x" % (
        datetime.datetime.now(), last_write_position, start, bytes_written)

    return bytes_written


def flash_read(dev, start, nbytes, ofile):
    print "%s reading 0x%08x bytes from 0x%08x" % (
        datetime.datetime.now(), nbytes, start)

    with os.fdopen(os.open(dev, os.O_RDONLY), 'r') as file_:
        os.lseek(file_.fileno(), start, os.SEEK_SET)

        while nbytes > 0:
            if nbytes > 4096:
                rbytes = 4096
            else:
                rbytes = nbytes

            ichunk = os.read(file_.fileno(), rbytes)

            if not ichunk:
                raise Exception("read of flash failed")

            ofile.write(ichunk)
            nbytes -= rbytes


def parse_args():
    descr = 'A tool to help update the flash used to configure an '
    descr += 'Intel FPGA at power up.'

    epi = 'flash programming types:\n\n'
    epi += '  user		FPGA user image update\n'
    epi += '  factory	FPGA factory image update\n'
    epi += '  factory_only	FPGA factory image (only) update\n'
    epi += '  rsu		Remote System Update (Board Reboot)\n'
    epi += '  dtb		Device tree blob update\n'
    epi += '  eeprom	EEPROM update\n'
    epi += '  phy_eeprom	PHY EEPROM update\n'
    epi += '  bmc_bl	%s boot loader update\n' % (RC)
    epi += '  bmc_app	%s application update\n' % (RC)
    epi += '  bmc_fw	BMC firmware update\n'
    epi += '  bmc_img	BMC user image update\n'
    epi += '  bmc_factory	BMC factory image update\n\n'
    epi += 'example usages:\n\n'
    epi += '    fpgaflash user new_image.rpd 0000:04:00.0\n\n'
    epi += '    fpgaflash rsu dcp_2_0.bin 0000:08:00.0\n\n'

    fc_ = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(description=descr, epilog=epi,
                                     formatter_class=fc_)

    parser.add_argument('type', help='type of flash programming',
                        choices=['user', 'factory', 'factory_only', 'rsu',
                                 'eeprom', 'phy_eeprom', 'bmc_bl', 'bmc_app',
                                 'bmc_fw', 'bmc_img', 'bmc_factory', 'dtb'])
    parser.add_argument('file', type=argparse.FileType('rb'),
                        help='file to program into flash')

    bdf_help = "bdf of device to program (e.g. 04:00.0 or 0000:04:00.0)"
    bdf_help += " optional when one device in system"

    parser.add_argument('bdf', nargs='?', help=bdf_help)

    rsu_help = "perform remote system update after update"
    rsu_help += " causing the board to be rebooted"

    parser.add_argument('-r', '--rsu', action='store_true', help=rsu_help)

    no_verify_help = "do not read back flash and verify after writing"
    parser.add_argument('-n', '--no-verify', default=False,
                        action='store_true', help=no_verify_help)

    yes_help = "answer Yes to all confirmation prompts"
    parser.add_argument('-y', '--yes', default=False,
                        action='store_true', help=yes_help)

    return parser.parse_args()


def get_pvid(bdf):
    pci_path = os.path.join('/sys/bus/pci/devices', bdf)

    with open(os.path.join(pci_path, 'vendor'), 'r') as fd:
        vendor = fd.read().strip()

    with open(os.path.join(pci_path, 'device'), 'r') as fd:
        device = fd.read().strip()

    return "{}:{}".format(vendor, device)


def get_bdf_pvid_mapping():
    bdf_map = dict()
    for fpga in glob.glob('/sys/class/fpga/*'):
        bdf = os.path.basename(os.readlink(os.path.join(fpga, "device")))
        if not bdf:
            continue

        bdf_map[bdf] = get_pvid(bdf)

    return bdf_map


def get_bdf_mtd_mapping():
    bdf_map = dict()
    for fpga in glob.glob('/sys/class/fpga/*'):
        bdf = os.path.basename(os.readlink(os.path.join(fpga, "device")))
        if not bdf:
            continue

        mtds = glob.glob(os.path.join(fpga, 'intel-fpga-fme.*',
                                      'altr-asmip2*', 'mtd', 'mtd*'))
        for mtd in mtds:
            if not fnmatch.fnmatchcase(mtd, "*ro"):
                bdf_map[bdf] = os.path.join('/dev', os.path.basename(mtd))
                break

    return bdf_map


def get_bdf_spi_mapping():
    bdf_map = dict()
    for fpga in glob.glob('/sys/class/fpga/*'):
        bdf = os.path.basename(os.readlink(os.path.join(fpga, "device")))
        if not bdf:
            continue

        spi_path = glob.glob(os.path.join(fpga, 'intel-fpga-fme.*',
                                          'spi-altera*.*.auto', 'spi_master',
                                          'spi*', 'spi*.*'))

        if len(spi_path) == 0:
            continue

        bdf_map[bdf] = spi_path[0]

    return bdf_map


def print_bdf_mtd_mapping(bdf_map):
    print "\nFPGA cards available for flashing:"

    for key in bdf_map.keys():
        print "    {}".format(key)

    print

    sys.exit(1)


def normalize_bdf(bdf):

    bdf = bdf.lower()

    pat = r'[0-9a-f]{4}:[0-9a-f]{2}:[0-9a-f]{2}\.[0-9a-f]$'
    if re.match(pat, bdf):
        return bdf

    if re.match(r'[0-9a-f]{2}:[0-9a-f]{2}\.[0-9a-f]$', bdf):
        return "0000:{}".format(bdf)


def update_flash(ifile, mtd_dev, target_offset, input_offset, erase_len,
                 no_verify):

    ofile = tempfile.NamedTemporaryFile(mode='wb', delete=False)

    ifile.seek(input_offset)

    print "%s reversing bits" % (datetime.datetime.now())
    reverse_bits_in_file(ifile, ofile)

    ifile.close()
    ofile.close()

    flash_erase(mtd_dev, target_offset, erase_len)

    nbytes = os.path.getsize(ofile.name)

    with open(ofile.name, 'rb') as rfile:
        nbytes = flash_write(mtd_dev, target_offset, nbytes, rfile)

    with open(ofile.name, 'rb+') as rfile:
        rfile.truncate(nbytes)

    if not no_verify:
        vfile = tempfile.NamedTemporaryFile(mode='wb', delete=False)

        flash_read(mtd_dev, target_offset, nbytes, vfile)

        vfile.close()

        print "%s verifying flash" % (datetime.datetime.now())

        retval = filecmp.cmp(ofile.name, vfile.name)

        os.remove(ofile.name)
        os.remove(vfile.name)

        if retval:
            print "%s flash successfully verified" % (datetime.datetime.now())
        else:
            print "failed to verify flash"
            raise Exception


def fpga_update(ifile, mtd_dev, target_offset, input_offset, erase_len,
                no_verify):

    if not mtd_dev:
        raise Exception("null mtd_dev")

    try:
        mode = os.stat(mtd_dev).st_mode
    except Exception as ex:
        print ex
        return 1

    if not stat.S_ISCHR(mode):
        print "{} is not a device file.".format(mtd_dev)
        return 1

    update_flash(ifile, mtd_dev, target_offset, input_offset, erase_len,
                 no_verify)
    return 0


def get_dev_bmc(bdf):
    path = os.path.join('/sys/bus/pci/devices/', bdf,
                        'fpga/intel-fpga-dev.*/intel-fpga-fme.*',
                        'avmmi-bmc.*.auto')
    dirs = glob.glob(path)
    if len(dirs) < 1:
        print "The avmmi-bmc driver was not found."
        print "Driver or FIM may need to be upgraded."
        sys.exit(1)

    if len(dirs) > 1:
        print "Catastrophic error! More than one avmmi-bmc driver found."
        sys.exit(1)

    dev = os.path.join('/dev', os.path.basename(dirs[0]))

    mode = os.stat(dev).st_mode

    if not stat.S_ISCHR(mode):
        raise Exception("{} is not a device file.".format(dev))

    return dev


class BittwareBmc(object):

    BMC_IOCTL = 0xc0187600

    BW_ACT_APP_MAIN = 0x01
    BW_ACT_APP_BL = 0x02

    BW_DEV_FLASH = 0x00
    BW_DEV_EEPROM = 0x01

    BW_BL_CMD_HDR = [0xB8, 0x00, 0x64, 0x18, 0x7b, 0x00]
    BW_BL_MIN_RSP_LEN = 8  # includes completion code and BL result code

    BW_BL_CMD_VER = 0
    BW_BL_CMD_JUMP = 1
    BW_BL_CMD_READ = 2
    BW_BL_CMD_WRITE = 3

    BW_BL_READ_MAX = 512
    BW_BL_WRITE_MAX = 512
    BW_BL_PAGE_SIZE = 512

    BW_BL_HDR_SIZE = 16

    partitions = [
        {
            'name': 'main',
            'app': BW_ACT_APP_MAIN,
            'start': 0x00000,
            'hdr_start': 0x34FF0,
        },
        {
            'name': 'cfg',
            'app': BW_ACT_APP_MAIN,
            'start': 0x35000,
            'hdr_start': 0x38ff0,
        },
        {
            'name': 'bootloader',
            'app': BW_ACT_APP_BL,
            'start': 0x39000,
            'hdr_start': 0x40ff0,
        },
        {
            'name': 'bootloader_boot',
            'app': BW_ACT_APP_BL,
            'start': 0x41000,
            'hdr_start': 0x41770,
        },
        {
            'name': 'main_boot',
            'app': BW_ACT_APP_MAIN,
            'start': 0x41800,
            'hdr_start': 0x41ff0,
        },
    ]

    def __init__(self, dev_path, ifile):
        self.dev = dev_path
        self.fd_ = os.open(self.dev, os.O_SYNC | os.O_RDWR)
        self.ihex = IntelHex(ifile)
        self.ihex.padding = 0xff

    def verify_segments(self, utype):
        print self.ihex.segments()
        for (start, size) in self.ihex.segments():
            print "0x%x 0x%x" % (start, size)

    def bw_xact(self, txarray, rxlen):
        tx_buf = array('B', txarray)

        rx_buf = array('B')
        for _ in range(rxlen):
            rx_buf.append(0)

        xact = struct.pack('IHHQQ', 24,
                           tx_buf.buffer_info()[1], rx_buf.buffer_info()[1],
                           tx_buf.buffer_info()[0], rx_buf.buffer_info()[0])

        fcntl.ioctl(self.fd_, 0xc0187600, xact)

        if rx_buf[3] != 0:
            raise Exception("bad completion code 0x%x" % rx_buf[3])

        return rx_buf

    def bw_bl_xact(self, bltx, blrx):
        tx_buf = array('B', self.BW_BL_CMD_HDR)

        for elem in bltx:
            tx_buf.append(elem)

        rx_buf = self.bw_xact(tx_buf, blrx + self.BW_BL_MIN_RSP_LEN)

        if rx_buf[self.BW_BL_MIN_RSP_LEN - 1] != 0:
            raise Exception("bad BL result code 0x%x" %
                            rx_buf[self.BW_BL_MIN_RSP_LEN - 1])

        for _ in range(self.BW_BL_MIN_RSP_LEN):
            rx_buf.pop(0)

        return rx_buf

    def bl_version(self):

        rx_buf = self.bw_bl_xact([self.BW_BL_CMD_VER], 3)

        ver = rx_buf.pop(0)
        ver |= rx_buf.pop(0) << 8

        act = rx_buf.pop(0)

        if (act != self.BW_ACT_APP_MAIN) and (act != self.BW_ACT_APP_BL):
            raise Exception("bad active application 0x%x" % act)

        return ver, act

    def bl_jump_other(self, app):
        if (app != self.BW_ACT_APP_MAIN) and (app != self.BW_ACT_APP_BL):
            raise Exception("bad app %d" % app)

        self.bw_bl_xact([self.BW_BL_CMD_JUMP, app], 0)

        time.sleep(1)

        [ver, active] = self.bl_version()

        if active != app:
            raise Exception("failed to jump to app {}".format(app))
        else:
            print "successfully jumped to app {}".format(active)

    def bl_read(self, device, offset, count):
        if count > self.BW_BL_READ_MAX:
            raise Exception("bad count %d > %d" % (count, self.BW_BL_READ_MAX))

        if (device != self.BW_DEV_FLASH) and (device != self.BW_DEV_EEPROM):
            raise Exception("bad device %d" % device)

        txar = [self.BW_BL_CMD_READ, device,
                offset & 0xff, (offset >> 8) & 0xff,
                (offset >> 16) & 0xff, (offset >> 24) & 0xff,
                count & 0xff, (count >> 8) & 0xff]

        rx_buf = self.bw_bl_xact(txar, 2 + count)

        rx_cnt = rx_buf.pop(0)
        rx_cnt |= rx_buf.pop(0) << 8

        if (rx_cnt != count) or (rx_cnt != len(rx_buf)):
            raise Exception("bad rx_cnt 0x%x != 0x%x != 0x%x" %
                            (rx_cnt, count, len(rx_buf)))

        return rx_buf

    def bl_write(self, device, offset, txdata):
        count = len(txdata)

        if count > self.BW_BL_WRITE_MAX:
            raise Exception("bad len %d > %d" %
                            (count, self.BW_BL_WRITE_MAX))

        if (device != self.BW_DEV_FLASH) and (device != self.BW_DEV_EEPROM):
            raise Exception("bad device %d" % device)

        txar = [self.BW_BL_CMD_WRITE, device,
                offset & 0xff, (offset >> 8) & 0xff,
                (offset >> 16) & 0xff, (offset >> 24) & 0xff,
                count & 0xff, (count >> 8) & 0xff]

        for data in txdata:
            txar.append(data)

        rx_buf = self.bw_bl_xact(txar, 2)

        tx_cnt = rx_buf.pop(0)
        tx_cnt |= rx_buf.pop(0) << 8

        if tx_cnt != count:
            raise Exception("bad tx_cnt 0x%x != 0x%x" %
                            (tx_cnt, count))

    def verify_partition(self, part):
        print "verifying %s from 0x%x to 0x%x" % (
            part['name'], part['start'],
            part['hdr_start'] + self.BW_BL_HDR_SIZE)

        valid = True
        for offset in range(part['start'],
                            part['hdr_start'] + self.BW_BL_HDR_SIZE,
                            self.BW_BL_READ_MAX):

            rx_buf = self.bl_read(self.BW_DEV_FLASH, offset,
                                  self.BW_BL_READ_MAX)

            for i in range(self.BW_BL_READ_MAX):
                if rx_buf[i] != self.ihex[offset+i]:
                    print "mismatch at offset 0x%x 0x%x != 0x%x" % (
                        offset+i, rx_buf[i], self.ihex[offset+i])
                    valid = False

        return valid

    def verify_partitions(self, utype):
        valid = True
        for part in self.partitions:
            if part['app'] == utype:
                if not self.verify_partition(part):
                    valid = False

        return valid

    def write_range(self, ih, start, end):
        for offset in range(start, end, self.BW_BL_WRITE_MAX):

            data = []
            for i in range(offset, offset+self.BW_BL_WRITE_MAX):
                data.append(ih[i])

            print "    0x%x - 0x%x %d" % (
                offset, offset + self.BW_BL_WRITE_MAX, len(data))

            valid = False
            j = 0

            while (not valid and (j < 3)):
                self.bl_write(self.BW_DEV_FLASH, offset, data)
                rx_buf = self.bl_read(self.BW_DEV_FLASH, offset,
                                      self.BW_BL_WRITE_MAX)
                valid = True
                for i in range(len(data)):
                    if (data[i] != rx_buf[i]):
                        valid = False
                        j += 1
                        break
            if (not valid):
                raise Exception("failed to write range 0x%x 0x%x" % (
                    start, end))

    def write_page0(self):
        ih = IntelHex()
        jump_bl_instrs = [0x82, 0xe0, 0x8c, 0xbf, 0xe0, 0xe0, 0xf0, 0xe0,
                          0x19, 0x94, 0x0c, 0x94, 0x00, 0x00, 0xff, 0xcf]
        for i in range(len(jump_bl_instrs)):
            ih[i] = jump_bl_instrs[i]

        self.write_range(ih, 0, self.BW_BL_PAGE_SIZE)

    def write_partitions(self, utype):
        wrote_page0 = False
        for part in self.partitions:
            if part['app'] == utype:
                start = part['start']
                print "updating %s from 0x%x to 0x%x" % (
                    part['name'], start,
                    part['hdr_start'] + self.BW_BL_HDR_SIZE)

                if part['start'] == 0:
                    self.write_page0()
                    start = self.BW_BL_PAGE_SIZE
                    wrote_page0 = True

                self.write_range(self.ihex, start,
                                 part['hdr_start'] + self.BW_BL_HDR_SIZE)
        if wrote_page0:
            self.write_range(self.ihex, 0, self.BW_BL_PAGE_SIZE)


def bmc_update(utype, ifile, bdf):
    dev = get_dev_bmc(bdf)

    bw_bmc = BittwareBmc(dev, ifile)

    ver, active = bw_bmc.bl_version()

    print "ver %d act %d" % (ver, active)

    if utype == 'bmc_bl':
        bw_bmc.verify_segments(bw_bmc.BW_ACT_APP_BL)

        if active != bw_bmc.BW_ACT_APP_MAIN:
            bw_bmc.bl_jump_other(bw_bmc.BW_ACT_APP_MAIN)

        bw_bmc.write_partitions(bw_bmc.BW_ACT_APP_BL)
    elif utype == 'bmc_app':
        bw_bmc.verify_segments(bw_bmc.BW_ACT_APP_MAIN)

        if active != bw_bmc.BW_ACT_APP_BL:
            bw_bmc.bl_jump_other(bw_bmc.BW_ACT_APP_BL)
        bw_bmc.write_partitions(bw_bmc.BW_ACT_APP_MAIN)
        bw_bmc.bl_jump_other(bw_bmc.BW_ACT_APP_MAIN)
    else:
        raise Exception("unknown utype: %s" % utype)

    return 0


def write_file(fname, data):
    with open(fname, 'wb') as wfile:
        wfile.write(data)


def get_mtd_from_spi_path(mode_path, spi_path):
    write_file(mode_path, "1")

    for i in range(20):

        time.sleep(1)

        mtds = glob.glob(os.path.join(spi_path, '*.*.auto',
                                      'mtd', 'mtd*'))

        for mtd in mtds:
            if not fnmatch.fnmatchcase(mtd, "*ro"):
                mtd_dev = os.path.join('/dev', os.path.basename(mtd))
                print "using %s" % (mtd_dev)
                return mtd_dev

    raise Exception("no mtd node found for mode %s" % (mode_path))


def bmc_fw_wait_for_nios(ver_file):
    for _idx in range(100):
        with open(ver_file, 'rb') as vfile:
            version = vfile.read()
            if version.strip() == 'ffffffff':
                time.sleep(1)
            else:
                return
    raise Exception("NIOS FW version not found")


def bmc_fw_update(ifile, spi_path, no_verify):
    mode_path = os.path.join(spi_path, 'bmcfw_flash_ctrl', 'bmcfw_flash_mode')

    ihex = IntelHex(ifile)

    num_segs = len(ihex.segments())
    if num_segs > 1:
        raise Exception("Expected only one segment but found %d" % (num_segs))

    start, size = ihex.segments()[0]

    mtd_dev = get_mtd_from_spi_path(mode_path, spi_path)

    bdata = ihex.tobinarray()

    SECTOR_SIZE = 0x10000

    for i in range(0, size, SECTOR_SIZE):
        flash_erase(mtd_dev, i, SECTOR_SIZE)

    ret = 0
    try:
        with os.fdopen(os.open(mtd_dev, os.O_SYNC | os.O_RDWR), 'a') as file_:
            # write from offset 256 bytes to end
            os.lseek(file_.fileno(), start + BMC_FW_HEAD_SIZE, os.SEEK_SET)
            print "%s writing 0x%08x bytes to 0x%08x" % \
                  (datetime.datetime.now(), size - BMC_FW_HEAD_SIZE,
                   start + BMC_FW_HEAD_SIZE)
            os.write(file_.fileno(), bdata[BMC_FW_HEAD_SIZE:])
            # write first 256 bytes
            os.lseek(file_.fileno(), start, os.SEEK_SET)
            print "%s writing 0x%08x bytes to 0x%08x" % \
                  (datetime.datetime.now(), BMC_FW_HEAD_SIZE, start)
            os.write(file_.fileno(), bdata[:BMC_FW_HEAD_SIZE])

            if not no_verify:
                os.lseek(file_.fileno(), start, os.SEEK_SET)
                read_back_data = os.read(file_.fileno(), size)
                if read_back_data != ihex.tobinstr():
                    print "failed to verify data"
                    ret = 1
                else:
                    print "%s flash successfully verified" % \
                          (datetime.datetime.now())
    except (Exception, KeyboardInterrupt), ex:
        print ex
        ret = 1

    write_file(mode_path, "0")

    ver_file = os.path.join(spi_path, 'bmcfw_flash_ctrl', 'bmcfw_version')

    bmc_fw_wait_for_nios(ver_file)

    return ret


def validate_bdf(bdf, bdf_pvid_map):
    if not bdf:
        if len(bdf_pvid_map) > 1:
            print "Must specify a bdf. More than one device found."
            print_bdf_mtd_mapping(bdf_pvid_map)
        else:
            bdf = bdf_pvid_map.keys()[0]
    else:
        bdf = normalize_bdf(bdf)
        if not bdf:
            print "{} is an invalid bdf".format(bdf)
            sys.exit(1)
        elif bdf not in bdf_pvid_map.keys():
            print "Could not find fpga device for {}".format(bdf)
            print_bdf_mtd_mapping(bdf_pvid_map)

    if bdf_pvid_map[bdf] not in KNOWN_PVIDS:
        print "{} is unsupported PCIE device: {}".format(
            bdf, bdf_pvid_map[bdf])
        sys.exit(1)

    return bdf


def rc_fpga_update(ifile, utype, bdf, no_verify):
    bdf_map = get_bdf_mtd_mapping()
    mtd_dev = bdf_map[bdf]

    target_offset = check_rpd(ifile)

    if utype == 'factory':
        target_offset = 0

    flash_size = get_flash_size(mtd_dev)

    erase_len = flash_size - target_offset

    return fpga_update(ifile, mtd_dev, target_offset, target_offset, erase_len,
                       no_verify)


def get_aer(bdf):
    cmd = 'setpci -s {} ECAP_AER+0x08.l'.format(bdf)

    output = subprocess.check_output(cmd, shell=True)
    val1 = int(output, 16)

    cmd = 'setpci -s {} ECAP_AER+0x14.l'.format(bdf)

    output = subprocess.check_output(cmd, shell=True)
    val2 = int(output, 16)

    return [val1, val2]


def set_aer(bdf, val1, val2):
    cmd = 'setpci -s %s ECAP_AER+0x08.l=0x%x' % (bdf, val1)

    output = subprocess.check_output(cmd, shell=True)

    cmd = 'setpci -s %s ECAP_AER+0x14.l=0x%x' % (bdf, val2)

    output = subprocess.check_output(cmd, shell=True)


def get_upstream_bdf(bdf):
    pci_path = os.path.join('/sys/bus/pci/drivers/intel-fpga-pci', bdf)

    # DC's link like '../../../../devices/pci0000:00/0000:00:03.0/0000:03:00.0'
    # VC's link like '../../../../devices/pci0000:20/0000:20:00.0/0000:21:00.0/
    #                                                0000:22:09.0/0000:25:00.0'
    link = os.readlink(pci_path)

    bdf_pattern = re.compile(r'(\d{4}):(\w{2}):(\d{2})\.(\d)')
    found_bdfs = bdf_pattern.findall(link)
    normalized_bdfs = []
    for n in found_bdfs:
        normalized_bdfs.append('{}:{}:{}.{}'.format(*n))

    pvid = get_pvid(bdf)
    if pvid == DC_PVID:
        # at least two pci devices should be found:
        # -> upstream_port -> fpga
        if len(normalized_bdfs) < 2:
            raise Exception("pci path to fpga is short than expected")
        upstream_bdf = normalized_bdfs[-2]
        target_bdf = normalized_bdfs[-1]
        if target_bdf != bdf:
            raise Exception("target bdf is not expected")
    elif pvid == VC_PVID:
        # at least four pci devices should be found:
        # -> upstream_port -> switch_up_port -> switch_down_port -> fpga
        if len(normalized_bdfs) < 4:
            raise Exception("pci path to fpga is short than expected")
        upstream_bdf = normalized_bdfs[-4]
        target_bdf = normalized_bdfs[-3]
    else:
        raise Exception("unrecognized pvid {}".format(pvid))

    return (upstream_bdf, target_bdf)


def rescan_pci_bus(bdf, bus):
    # after device(s) removed, its connected bus's power may suspend, so turn
    # on the bus power is necessary before rescanning the bus
    power_path = '/sys/bus/pci/devices/{}/power/control'.format(bdf)
    with open(power_path, 'r') as fd:
        ctrl = fd.read().strip()
    if ctrl != "on":
        write_file(power_path, "on")

    rescan_path = '/sys/bus/pci/devices/{}/pci_bus/{}/rescan'.format(bdf, bus)
    write_file(rescan_path, "1")

    if ctrl != "on":    # revert power control
        write_file(power_path, ctrl)


def dc_vc_reconfigure(bdf, ld_path, boot_page):
    print "%s performing remote system update" % (datetime.datetime.now())

    (upstream_bdf, target_bdf) = get_upstream_bdf(bdf)

    orig_aer = get_aer(upstream_bdf)

    set_aer(upstream_bdf, 0xffffffff, 0xffffffff)

    rm_path = os.path.join('/sys/bus/pci/devices', target_bdf, 'remove')

    try:
        write_file(ld_path, boot_page)
    except Exception:
        time.sleep(5)

    write_file(rm_path, "1")

    print "%s waiting for FPGA reconfiguration" % (datetime.datetime.now())

    time.sleep(10)

    rescan_pci_bus(upstream_bdf, target_bdf[:7])

    time.sleep(1)

    cmd = 'setpci -s %s ECAP_AER+0x10.l=0x00002000' % (bdf)

    subprocess.check_call(cmd, shell=True)

    set_aer(upstream_bdf, 0, 0)

    print "%s pci bus rescanned" % (datetime.datetime.now())

    return 0


def board_rsu(bdf, spi_path, boot_page):
    ld_path = os.path.join(spi_path, 'bmcimg_flash_ctrl', 'bmcimg_image_load')
    return dc_vc_reconfigure(bdf, ld_path, boot_page)


def dc_vc_fpga_update(ifile, utype, bdf, spi_path, rsu, boot_page, no_verify):

    if utype in ['bmc_img', 'bmc_factory']:
        mode_path = os.path.join(spi_path,
                                 'bmcimg_flash_ctrl', 'bmcimg_flash_mode')
    else:
        mode_path = os.path.join(spi_path,
                                 'fpga_flash_ctrl', 'fpga_flash_mode')

    mtd_dev = get_mtd_from_spi_path(mode_path, spi_path)

    pvid = get_pvid(bdf)
    if utype == 'factory':
        target_offset = 0x10000
        input_offset = 0
        erase_len = 0x7ff0000
    elif utype == 'factory_only':
        target_offset = 0x20000
        input_offset = 0
        erase_len = VC_DC_FACTORY_BLOCK_SIZE
    elif utype == 'bmc_img':
        input_offset = 0
        if pvid == DC_PVID:
            target_offset = 0x10000
            erase_len = 0x60000
            flash_erase(mtd_dev, 0x70000, 0x48000)
        elif pvid == VC_PVID:
            target_offset = 0xb8000
            erase_len = 0xa8000
        else:
            raise Exception("{} not supported on {}".format(utype, pvid))
    elif utype == 'bmc_factory':
        input_offset = 0
        if pvid == VC_PVID:
            target_offset = 0x10000
            erase_len = 0x60000
            flash_erase(mtd_dev, 0x70000, 0x48000)
        elif pvid == DC_PVID:
            target_offset = 0xb8000
            erase_len = 0xa8000
        else:
            raise Exception("{} not supported on {}".format(utype, pvid))
    elif utype == 'dtb':
        target_offset = 0x3820000
        input_offset = 0
        erase_len = VC_DC_DTB_BLOCK_SIZE
    else:
        target_offset = 0x4000000
        input_offset = 0x3ff0000
        erase_len = 0x3800000

    try:
        ret = fpga_update(ifile, mtd_dev, target_offset, input_offset,
                          erase_len, no_verify)
    except (Exception, KeyboardInterrupt), ex:
        print ex
        ret = 1

    write_file(mode_path, "0")

    if not ret and rsu:
        ret = board_rsu(bdf, spi_path, boot_page)

    return ret


def vc_update_eeprom(ifile, bdf):
    ret = 0

    eeprom_path = glob.glob(os.path.join('/sys/bus/pci/devices', bdf,
                                         'fpga', 'intel-fpga-dev.*',
                                         'intel-fpga-fme.*',
                                         'altera-i2c.*.auto', 'i2c-*',
                                         '*-*', 'eeprom'))

    if len(eeprom_path) != 1:
        print "eeprom_path length %d != 1" % (len(eeprom_path))
        ret = 1
    else:
        with open(eeprom_path[0], 'wb') as wfile:
            wfile.write(ifile.read(VC_EEPROM_SIZE))

    return ret


def dc_update_eeprom(ifile, spi_path):
    ret = 0

    mode_path = os.path.join(spi_path, 'i2c_master2_ctrl', 'i2c_master2_mode')

    write_file(mode_path, "1")

    time.sleep(1)

    eeprom_path = glob.glob(os.path.join(spi_path, 'altera-i2c*auto',
                            'i2c-*', '*-0057/', 'eeprom'))

    if len(eeprom_path) != 1:
        print "eeprom_path length %d != 1" % (len(eeprom_path))
        ret = 1
    else:
        with open(eeprom_path[0], 'wb') as wfile:
            wfile.write(ifile.read())

    write_file(mode_path, "1")

    return ret


def get_flash_mode(spi_path):
    modes = []

    for prefix in ['fpga', 'bmcfw', 'bmcimg']:
        path = os.path.join(spi_path, '{}_flash_ctrl'.format(prefix),
                            '{}_flash_mode'.format(prefix))
        with open(path, 'r') as fd:
            modes.append(int(fd.read(1)))

    # return 1 if any type of flash is in use
    return 1 if any(modes) else 0


def check_file(ifile, utype):
    ret = 0

    image_len = os.path.getsize(ifile.name)
    if utype in ['factory', 'factory_only', 'user', 'dtb']:
        if image_len == VC_FPGA_IMAGE_LENGTH:
            ifile.seek(0)
            data = ifile.read(8)
            bin_hdr = struct.unpack('>II', data)
            if bin_hdr[0] != VC_FPGA_IMAGE_MAGIC_NUM1 or \
               bin_hdr[1] != VC_FPGA_IMAGE_MAGIC_NUM2:
                print "invalid fpga image file"
                ret = 1
            if utype == 'dtb':
                ifile.seek(VC_DC_DTB_BLOCK_OFFSET)
                data = ifile.read(4)
                dtb_hdr = struct.unpack('>I', data)
                if dtb_hdr[0] != DTB_REVERSED_MAGIC_NUM:
                    print "invalid dtb magic number"
                    ret = 1
        else:
            print "invalid fpga image length {}".format(image_len)
            ret = 1
    elif utype in ['bmc_img', 'bmc_factory']:
        if image_len == VC_MAX10_IMAGE_LENGTH:
            ifile.seek(0x8c)
            data = ifile.read(4)
            rpd_hdr = struct.unpack('I', data)
            if rpd_hdr[0] != VC_MAX10_IMAGE_MAGIC_NUM:
                print "invalid max10 image file"
                ret = 1
        else:
            print "invalid max10 image length {}".format(image_len)
            ret = 1
    else:
        ret = 0

    ifile.seek(0)
    return ret


def check_file_extension(ifile, utype):
    ret = 0

    extension = ifile.name.split('.')[-1].lower()
    if utype in ['factory', 'factory_only', 'user', 'dtb']:
        if extension != 'bin':
            print "invalid fpga image file extension"
            ret = 1
    elif utype in ['bmc_img', 'bmc_factory']:
        if extension != 'rpd':
            print "invalid max10 image file extension"
            ret = 1
    elif utype in ['bmc_fw']:
        if extension != 'ihex':
            print "invalid nios firmware file extension"
            ret = 1
    else:
        ret = 0

    return ret


def vc_phy_eeprom_update(ifile, spi_path, no_verify):
    ret = bmc_fw_update(ifile, spi_path, no_verify)
    if ret == 0:
        print "%s updating phy eeprom" % (datetime.datetime.now())
        load_path = os.path.join(spi_path, 'pkvl', 'eeprom_load')
        try:
            write_file(load_path, "1")
        except (Exception, KeyboardInterrupt):
            ret = 1
        if ret == 0:
            print "%s successful" % (datetime.datetime.now())
        else:
            print "%s failed" % (datetime.datetime.now())
    else:
        print "%s abort updating phy eeprom" % (datetime.datetime.now())

    return ret



def vc_phy_eeprom_update(ifile, spi_path, no_verify):
    ret = bmc_fw_update(ifile, spi_path, no_verify)
    if ret == 0:
        print "%s updating phy eeprom" % (datetime.datetime.now())
        load_path = os.path.join(spi_path, 'pkvl', 'eeprom_load')
        try:
            write_file(load_path, "1")
        except (Exception, KeyboardInterrupt):
            ret = 1
        if ret == 0:
            print "%s successful" % (datetime.datetime.now())
        else:
            print "%s failed" % (datetime.datetime.now())
    else:
        print "%s abort updating phy eeprom" % (datetime.datetime.now())

    return ret


def validate_bdf(bdf, bdf_pvid_map):
    if not bdf:
        if len(bdf_pvid_map) > 1:
            print "Must specify a bdf. More than one device found."
            print_bdf_mtd_mapping(bdf_pvid_map)
        else:
            bdf = bdf_pvid_map.keys()[0]
    else:
        bdf = normalize_bdf(bdf)
        if not bdf:
            print "{} is an invalid bdf".format(bdf)
            sys.exit(1)
        elif bdf not in bdf_pvid_map.keys():
            print "Could not find fpga device for {}".format(bdf)
            print_bdf_mtd_mapping(bdf_pvid_map)


def main():
    args = parse_args()


    bdf_pvid_map = get_bdf_pvid_mapping()

    if len(bdf_pvid_map) == 0:
        print "No FPGA devices found"
        sys.exit(1)

    bdf = validate_bdf(args.bdf, bdf_pvid_map)

    # check if another fpgaflash instance is programming on specified card
    if (bdf_pvid_map[bdf] == DC_PVID) or (bdf_pvid_map[bdf] == VC_PVID):
        bdf_map = get_bdf_spi_mapping()
        spi_path = bdf_map[bdf]
        if get_flash_mode(spi_path) == 1:
            print "fpgaflash on {} is in progress, try later".format(bdf)
            sys.exit(1)
        if check_file_extension(args.file, args.type) == 1:
            sys.exit(1)

    if args.type != 'eeprom':
        assert_not_running(["pacd", "fpgad", "fpgainfo"])

    if args.type == 'bmc_fw':
        if (bdf_pvid_map[bdf] == DC_PVID) or (bdf_pvid_map[bdf] == VC_PVID):
            sys.exit(bmc_fw_update(args.file, spi_path, args.no_verify))
        else:
            print "bmc_fw only supported on {} and {}".format(DC_PVID, VC_PVID)
            sys.exit(1)
    elif (args.type == 'bmc_bl') or (args.type == 'bmc_app'):
        if bdf_pvid_map[bdf] == RC_PVID:
            sys.exit(bmc_update(args.type, args.file, bdf))
        else:
            print "{} only supported on {}".format(args.type, RC_PVID)
            sys.exit(1)
    elif args.type == 'phy_eeprom':
        if bdf_pvid_map[bdf] == VC_PVID:
            sys.exit(vc_phy_eeprom_update(args.file, spi_path, args.no_verify))
        else:
            print "{} only supported on {}".format(args.type, VC_PVID)
            sys.exit(1)

    if not args.yes and \
       args.type in ['factory', 'factory_only', 'bmc_factory']:
        msg = "Are you sure you want to perform a factory update? [Yes/No]"
        line = raw_input(msg)
        if line != "Yes":
            sys.exit(1)

    if bdf_pvid_map[bdf] == RC_PVID:
        if args.type in ['rsu', 'bmc_img', 'eeprom', 'bmc_factory']:
            print "%s not supported on %s" % (args.type, RC)
            sys.exit(1)

        ret = rc_fpga_update(args.file, args.type, bdf, args.no_verify)
    else:
        if bdf_pvid_map[bdf] == DC_PVID:
            boot_page = "1"
        elif bdf_pvid_map[bdf] == VC_PVID:
            boot_page = "0"
        else:
            raise Exception("bad pvid %s" % (bdf_pvid_map[bdf]))

        if bdf_pvid_map[bdf] == DC_PVID:
            boot_page = "1"
        elif bdf_pvid_map[bdf] == VC_PVID:
            boot_page = "0"
        else:
            raise Exception("bad pvid %s" % (bdf_pvid_map[bdf]))

        if args.type == 'rsu':
            ret = board_rsu(bdf, spi_path, boot_page)
        elif args.type == 'eeprom':
            if bdf_pvid_map[bdf] == DC_PVID:
                ret = dc_update_eeprom(args.file, spi_path)
            elif bdf_pvid_map[bdf] == VC_PVID:
                ret = vc_update_eeprom(args.file, bdf)
            else:
                print "eeprom only supported on {} and {}".format(DC_PVID,
                                                                  VC_PVID)
                ret = 1
        else:
            # file validation check for Vista Creek
            if bdf_pvid_map[bdf] == VC_PVID:
                ret = check_file(args.file, args.type)
                if ret != 0:
                    sys.exit(ret)

            if args.type in ['bmc_img', 'bmc_factory']:
                sfile = tempfile.NamedTemporaryFile(mode='wb+', delete=False)
                a = array("I", args.file.read())
                for elem in a:
                    sfile.write(struct.pack('>I', elem))
                sfile.close()
                sfile = open(sfile.name, 'rb')
            elif args.type == 'dtb':
                sfile = tempfile.NamedTemporaryFile(mode='wb+', delete=False)
                args.file.seek(VC_DC_DTB_BLOCK_OFFSET)
                block = args.file.read(VC_DC_DTB_BLOCK_SIZE)
                sfile.write(block)
                sfile.close()
                sfile = open(sfile.name, 'rb')
            elif args.type == 'factory_only':
                sfile = tempfile.NamedTemporaryFile(mode='wb+', delete=False)
                args.file.seek(VC_DC_FACTORY_BLOCK_OFFSET)
                block = args.file.read(VC_DC_FACTORY_BLOCK_SIZE)
                sfile.write(block)
                sfile.close()
                sfile = open(sfile.name, 'rb')
            else:
                sfile = args.file

            ret = dc_vc_fpga_update(sfile, args.type, bdf, spi_path,
                                    args.rsu, boot_page, args.no_verify)

    sys.exit(ret)


if __name__ == "__main__":
    main()
