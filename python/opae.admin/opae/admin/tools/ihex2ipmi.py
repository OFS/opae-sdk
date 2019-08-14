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

""" Convert ihex file into FPGA bitstream file """

import argparse
import struct
import sys
from array import array

try:
    from intelhex import IntelHex
except ImportError:
    sys.exit('Missing intelhex. Install with: sudo pip install intelhex')

RC = 'Intel PAC with Arria 10 GX FPGA'

BOOTLOADER_ID = [1, 0, 0, 0]
BMC_APP_ID = [2, 0, 0, 0]


def parse_args():
    """ Parse command-line arguments """
    descr = 'A tool to convert Rush Creek BMC ihex into a sequence of IPMI '
    descr += 'messages'

    epi = 'conversion types:\n\n'
    epi += '  bmc_bl	%s boot loader conversion\n' % (RC)
    epi += '  bmc_app	%s application conversion\n' % (RC)
    epi += 'example usage:\n\n'
    epi += '    ihex2ipmi bmc_app a10sa4-26889-fw.hex a10sa4-26889-fw.ipmi\n\n'

    fc_ = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(description=descr, epilog=epi,
                                     formatter_class=fc_)

    parser.add_argument('type', help='type of conversion',
                        choices=['bmc_bl', 'bmc_app'])
    parser.add_argument('ifile', type=argparse.FileType('rb'),
                        help='input intel hex file')
    parser.add_argument('ofile', type=argparse.FileType('wb'),
                        help='ouput ipmi file')

    return parser.parse_args()


class BittwareBmc(object):
    """ Class describing BittWare's ihex file and methods to deal with them """

    BW_ACT_APP_MAIN = 0x01
    BW_ACT_APP_BL = 0x02

    BW_DEV_FLASH = 0x00

    BW_BL_CMD_HDR = [0xB8, 0x00, 0x64, 0x18, 0x7b, 0x00]

    BW_BL_CMD_WRITE = 3

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
        self.ihex = IntelHex(ifile)
        self.ihex.padding = 0xff
        self.ofile = None
        self.jump_bl_instrs = [0x82, 0xe0, 0x8c, 0xbf, 0xe0, 0xe0, 0xf0, 0xe0,
                               0x19, 0x94, 0x0c, 0x94, 0x00, 0x00, 0xff, 0xcf]

    def set_ofile(self, ofile):
        """ Get output file from arguments """
        self.ofile = ofile

    def bl_write(self, device, offset, txdata):
        """ Write a block (IPMI command) to the output file """
        count = len(txdata)

        if count > self.BW_BL_WRITE_MAX:
            raise Exception("bad len %d > %d" %
                            (count, self.BW_BL_WRITE_MAX))

        if device != self.BW_DEV_FLASH:
            raise Exception("bad device %d" % device)

        txar = [self.BW_BL_CMD_WRITE, device,
                offset & 0xff, (offset >> 8) & 0xff,
                (offset >> 16) & 0xff, (offset >> 24) & 0xff,
                count & 0xff, (count >> 8) & 0xff]

        for data in txdata:
            txar.append(data)

        if self.ofile:
            tx_buf = array('B', self.BW_BL_CMD_HDR)

            for elem in txar:
                tx_buf.append(elem)

            self.ofile.write(struct.pack('H', len(tx_buf)))
            self.ofile.write(tx_buf)
        return

    def write_range(self, ihfile, start, end):
        """ Write bytes from start to end from ihfile """
        for offset in range(start, end, self.BW_BL_WRITE_MAX):

            data = []
            for i in range(offset, offset+self.BW_BL_WRITE_MAX):
                data.append(ihfile[i])

            # print "    0x%x - 0x%x %d" % (
            #    offset, offset + self.BW_BL_WRITE_MAX, len(data))

            if self.ofile:
                self.bl_write(self.BW_DEV_FLASH, offset, data)

    def write_page0(self):
        """ Write the BL_JUMP command into page 0 """
        ihfile = IntelHex()
        for i in range(len(self.jump_bl_instrs)):
            ihfile[i] = self.jump_bl_instrs[i]

        self.write_range(ihfile, 0, self.BW_BL_PAGE_SIZE)

    def write_partitions(self, utype):
        """ Write all partitions from ihexfile at proper locations """
        wrote_page0 = False
        for part in self.partitions:
            if part['app'] == utype:
                start = part['start']
                print("updating %s from 0x%x to 0x%x" % (
                    part['name'], start,
                    part['hdr_start'] + self.BW_BL_HDR_SIZE))

                if part['start'] == 0:
                    self.write_page0()
                    start = self.BW_BL_PAGE_SIZE
                    wrote_page0 = True

                self.write_range(self.ihex, start,
                                 part['hdr_start'] + self.BW_BL_HDR_SIZE)
        if wrote_page0:
            self.write_range(self.ihex, 0, self.BW_BL_PAGE_SIZE)

    def validate(self, ctype):
        """ Read file just written and compare to ihex file """
        self.ofile.close()
        with open(self.ofile.name, "rb") as ipmifile:
            filecontent = ipmifile.read()

        contents = struct.unpack('B' * len(filecontent), filecontent)
        if ctype == 'bmc_app':
            for index_ in range(len(self.jump_bl_instrs)):
                c_value = contents[index_+self.BW_BL_HDR_SIZE]
                j_value = self.jump_bl_instrs[index_]
                if c_value != j_value:
                    print("addr %x diff: file=%x, const=%x" %
                          (index_, c_value, j_value))

            start_ = self.BW_BL_HDR_SIZE + self.BW_BL_PAGE_SIZE
        else:
            start_ = 0

        for block_ in range(start_, len(filecontent)-1,
                            self.BW_BL_HDR_SIZE + self.BW_BL_PAGE_SIZE):
            addr_ = contents[block_+10] + (contents[block_+11] << 8) + \
                (contents[block_+12] << 16) + (contents[block_+13] << 24)
            len_ = contents[block_+14] + (contents[block_+15] << 8)

            for index_ in range(len_):
                if self.ihex[addr_+index_] != contents[block_+index_+16]:
                    print("addr %x diff: ihex=%x, ipmi=%x" %
                          (addr_+index_, self.ihex[addr_+index_],
                           contents[block_+index_+16]))


def bmc_convert(ctype, ifile, ofile):
    """ Main conversion.  Validate after writing. """
    bw_bmc = BittwareBmc(None, ifile)

    bw_bmc.set_ofile(ofile)

    if ctype == 'bmc_bl':
        bw_bmc.write_partitions(bw_bmc.BW_ACT_APP_BL)
    elif ctype == 'bmc_app':
        bw_bmc.write_partitions(bw_bmc.BW_ACT_APP_MAIN)
    else:
        raise Exception("unknown ctype: %s" % (ctype))

    pad_bytes = 1024 - (ofile.tell() - ((ofile.tell() / 1024) * 1024))
    if pad_bytes > 0:
        padding = [0x0 for _ in range(pad_bytes)]
        ofile.write(bytearray(padding))

    print("Validating")
    bw_bmc.validate(ctype)

    return 0


def main():
    """ Convert Intel hex file to FPGA bitstream format """
    args = parse_args()

    bmc_convert(args.type, args.ifile, args.ofile)

    # Insert type header into file
    with open(args.ofile.name, "rb") as ipmifile:
        filecontent = ipmifile.read()

    if args.type == 'bmc_bl':
        sig = bytearray(BOOTLOADER_ID)
    else:
        sig = bytearray(BMC_APP_ID)

    with open(args.ofile.name, "wb") as fil:
        fil.write(sig)
        fil.write(filecontent)


if __name__ == "__main__":
    main()
