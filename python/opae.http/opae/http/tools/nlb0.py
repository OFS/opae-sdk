#!/usr/bin/env python3
# Copyright(c) 2023, Intel Corporation
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
from ctypes import c_uint64, Structure, Union
import requests
import sys
import time

from opae.http.properties import properties
import opae.http.enumerate as enum
import opae.http.constants as constants


SCRIPT_VERSION = '0.0.0'

DEFAULT_PROTOCOL = 'http://'
DEFAULT_SERVER = 'psera2-dell05.ra.intel.com'
DEFAULT_PORT = '8080'

NLB0_AFUID = 'd8424dc4-a4a3-c413-f89e-433683f9040b'
N3000_AFUID = '9aeffe5f-8457-0612-c000-c9660d824272'

CSR_AFU_DSM_BASEL = 0x0110
CSR_SRC_ADDR = 0x0120
CSR_DST_ADDR = 0x0128
CSR_NUM_LINES = 0x0130
CSR_CTL = 0x0138
CSR_CFG = 0x0140
CSR_STATUS1 = 0x0168

DSM_STATUS_TEST_COMPLETE = 0x40


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('server', nargs='?', default=DEFAULT_SERVER,
                        help='fully-qualified hostname of grpc-gateway server')

    parser.add_argument('-p', '--port', default=DEFAULT_PORT,
                        help='server port')

    parser.add_argument('-P', '--protocol', default=DEFAULT_PROTOCOL,
                        choices=['http://', 'https://'])

    parser.add_argument('-g', '--debug', default=False, action='store_true',
                        help='enable debug info')

    parser.add_argument('-v', '--version', action='version',
                        version=f'%(prog)s {SCRIPT_VERSION}')

    return parser, parser.parse_args()


def http_url(args):
    return args.protocol + args.server + ':' + args.port


def MB(x):
    return x * 1024 * 1024


def cacheline_aligned(x):
    return x >> 6


def cachelines(x):
    return x >> 6


class dfh0_register_bits(Structure):
    _fields_ = [('id', c_uint64, 12),
                ('rev', c_uint64, 4),
                ('next', c_uint64, 24),
                ('eol', c_uint64, 1),
                ('reserved', c_uint64, 19),
                ('feature_type', c_uint64, 4)]


class dfh0_register(Union):
    _fields_ = [("bits", dfh0_register_bits),
                ("value", c_uint64)]

    def __init__(self, value):
        self.value = value

    @property
    def id(self):
        return self.bits.id

    @property
    def rev(self):
        return self.bits.rev

    @property
    def next(self):
        return self.bits.next

    @property
    def eol(self):
        return self.bits.eol

    @property
    def reserved(self):
        return self.bits.reserved

    @property
    def feature_type(self):
        return self.bits.feature_type


def n3000_afu_offset(h, region=0):
    found = False
    offset = 0
    eol = False

    while not eol:
        header = h.read64(region, offset)
        guidl = h.read64(region, offset + 8)
        guidh = h.read64(region, offset + 16)

        dfh = dfh0_register(header)
        if (dfh.feature_type == 1 and
            guidl == 0xf89e433683f9040b and
            guidh == 0xd8424dc4a4a3c413):
            found = True
            break

        eol = (dfh.eol == 1)

        if dfh.next == 0xffff or dfh.next == 0:
            break # not found

        offset += dfh.next

    if found:
        return offset

    raise RuntimeError('Failed to find NLB0 for N3000')


def main():
    parser, args = parse_args()

    mmio_region = 0

    filt = properties()
    filt.objtype = constants.FPGA_ACCELERATOR

    tokens = enum.enumerate(http_url(args), [filt], 1, args.debug)
    if len(tokens) == 0:
        print('FPGA not found.')
        sys.exit(1)

    props = tokens[0].get_properties()

    if props.guid.lower() not in [NLB0_AFUID, N3000_AFUID]:
        print('Failed to find NLB0 AFU.')
        for t in tokens:
            t.destroy()
        sys.exit(1)

    print(f'Found accelerator: {props.guid}')

    h = tokens[0].open()
    h.map_mmio(mmio_region)

    nlb_afu_offset = 0
    if (props.vendor_id, props.device_id) == (0x8086, 0x0b30):
        nlb_afu_offset = n3000_afu_offset(h, mmio_region)
        print(f'Found N3000 NLB0 at 0x{nlb_afu_offset:0x}')

    print('Allocating Device Status Memory')
    device_status_memory = h.prepare_buffer(MB(2))
    print('Allocating Input Buffer')
    input_buffer = h.prepare_buffer(MB(2))
    print('Allocating Output Buffer')
    output_buffer = h.prepare_buffer(MB(2))

    print('Initializing buffers')
    device_status_memory.memset(0, 0, MB(2))
    input_buffer.memset(0, 0xaf, MB(1))
    output_buffer.memset(0, 0xbe, MB(1))

    input_buffer.write_pattern('cl_index_end')

    print('Resetting the AFU')
    h.reset()

    print('Programming DSM base address')
    dsm_ioaddr = device_status_memory.ioaddr()
    h.write64(mmio_region, nlb_afu_offset + CSR_AFU_DSM_BASEL, dsm_ioaddr)

    print('Setting CTRL CSR')
    h.write32(mmio_region, nlb_afu_offset + CSR_CTL, 0)
    h.write32(mmio_region, nlb_afu_offset + CSR_CTL, 1)

    print('Programming input base address')
    input_ioaddr = input_buffer.ioaddr()
    h.write64(mmio_region, nlb_afu_offset + CSR_SRC_ADDR, cacheline_aligned(input_ioaddr))

    print('Programming output base address')
    output_ioaddr = output_buffer.ioaddr()
    h.write64(mmio_region, nlb_afu_offset + CSR_DST_ADDR, cacheline_aligned(output_ioaddr))

    print('Programming number of cachelines')
    h.write32(mmio_region, nlb_afu_offset + CSR_NUM_LINES, cachelines(MB(1)))

    print('Programming AFU CFG')
    h.write32(mmio_region, nlb_afu_offset + CSR_CFG, 0x42000)

    print('Starting the transfer')
    h.write32(mmio_region, nlb_afu_offset + CSR_CTL, 3)

    print('Polling for completion')
    status = device_status_memory.poll(DSM_STATUS_TEST_COMPLETE, 8, 0x1, 1, 100, 30000)
    if status == constants.FPGA_NOT_FOUND:
        print('** Error ** polling timed out')
    else:
        print('Transfer complete')

    print('Halting the device')
    h.write32(mmio_region, nlb_afu_offset + CSR_CTL, 7)

    print('Waiting for traffic to complete')
    tries = 0
    while tries < 100:
        status1 = h.read64(mmio_region, nlb_afu_offset + CSR_STATUS1)

        if status1 == 0:
            break

        tries += 1
        time.sleep(0.001)

    print('Verifying transfer.. ', end='')
    result = output_buffer.memcmp(input_buffer, 0, 0, MB(1))
    if result == 0:
        print('OK')
    else:
        print('** Error ** buffer miscompare')

    print('Cleaning up')
    output_buffer.destroy()
    input_buffer.destroy()
    device_status_memory.destroy()

    h.unmap_mmio(mmio_region)
    h.close()
    for t in tokens:
        t.destroy()


if __name__ == '__main__':
    main()
