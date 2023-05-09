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
import json
import requests

from opae.http.version import version
from opae.http.properties import fpga_properties, accelerator_properties, properties
import opae.http.constants as constants
from opae.http.conversion import to_json_str
from opae.http.token import fpga_remote_id, token
import opae.http.enumerate as enum


SCRIPT_VERSION = '0.0.0'

DEFAULT_PROTOCOL = 'http://'
DEFAULT_SERVER = 'psera2-dell05.ra.intel.com'
DEFAULT_PORT = '8080'


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('server', nargs='?', default=DEFAULT_SERVER,
                        help='fully-qualified hostname of grpc-gateway server')

    parser.add_argument('-p', '--port', default=DEFAULT_PORT,
                        help='server port')

    parser.add_argument('-P', '--protocol', default=DEFAULT_PROTOCOL,
                        choices=['http://', 'https://'])

    parser.add_argument('-v', '--version', action='version',
                        version=f'%(prog)s {SCRIPT_VERSION}')

    return parser, parser.parse_args()


def http_url(args):
    return args.protocol + args.server + ':' + args.port


def test_version():
    v = version.from_json_obj({'major': 4, 'minor': 5, 'patch': 6})
    print(v)
    print(to_json_str(v))


def test_properties():

    values = [ 'D8424DC4-A4A3-C413-F89E-433683F9040B', # 'guid': GUID_IDX,
               None, # 'parent': PARENT_IDX,
               constants.FPGA_ACCELERATOR, # 'objtype': OBJTYPE_IDX,
               0, # 'segment': SEGMENT_IDX,
               0x5e, # 'bus': BUS_IDX,
               0, # 'device': DEVICE_IDX,
               0, # 'function': FUNCTION_IDX,
               0, # 'socket_id': SOCKETID_IDX,
               0x115500, # 'object_id': OBJECTID_IDX,
               0x8086, # 'vendor_id': VENDORID_IDX,
               0xbcce, # 'device_id': DEVICEID_IDX,
               0, # 'num_errors': NUM_ERRORS_IDX,
               constants.FPGA_IFC_DFL, # 'interface': INTERFACE_IDX,
               0x8086, # 'subsystem_vendor_id': SUB_VENDORID_IDX,
               0x1d74, # 'subsystem_device_id': SUB_DEVICEID_IDX,
               'host@net.com', # 'hostname': HOSTNAME_IDX,
            ]


    fpga_values = [ 0, # 'num_slots': NUM_SLOTS_IDX,
                    0xfeedbeef, # 'bbs_id': BBSID_IDX,
                    version(1, 2, 3), # 'bbs_version': BBSVERSION_IDX,
                  ]

    accel_values = [ constants.FPGA_ACCELERATOR_UNASSIGNED, # 'state': ACCELERATOR_STATE_IDX,
                     2, # 'num_mmio': NUM_MMIO_IDX,
                     1, # 'num_interrupts': NUM_INTERRUPTS_IDX,
                   ]


    p = properties()
    for i, k in enumerate(properties.attrs):
        setattr(p, k, values[i])
        assert getattr(p, k) == values[i]
    print(to_json_str(p))

    fpga = properties()
    fpga.objtype = constants.FPGA_DEVICE
    for i, k in enumerate(fpga_properties.attrs):
        setattr(fpga.fpga, k, fpga_values[i])
        assert getattr(fpga.fpga, k) == fpga_values[i]
    print(to_json_str(fpga))

    accel = properties()
    accel.objtype = constants.FPGA_ACCELERATOR
    for i, k in enumerate(accelerator_properties.attrs):
        setattr(accel.accelerator, k, accel_values[i])
        assert getattr(accel.accelerator, k) == accel_values[i]
    print(to_json_str(accel))


    p = properties.from_json_obj({
        'objtype': "FPGA_DEVICE",
        'fpga': {
          'num_slots': 2,
          'bbs_id': 0xbaddecaf,
          'bbs_version': {'major': 7, 'minor': 8, 'patch': 9 }
        }
        })
    print(to_json_str(p))

    assert p.objtype == constants.FPGA_DEVICE
    assert p.fpga.num_slots == 2
    assert p.fpga.bbs_id == 0xbaddecaf
    assert p.fpga.bbs_version.major == 7
    assert p.fpga.bbs_version.minor == 8
    assert p.fpga.bbs_version.patch == 9

    p = properties.from_json_obj({
        'objtype': "FPGA_ACCELERATOR",
        'accelerator': {
          'state': "FPGA_ACCELERATOR_ASSIGNED",
          'num_mmio': 2,
          'num_interrupts': 4 }
        })
    print(to_json_str(p))

    assert p.objtype == constants.FPGA_ACCELERATOR
    assert p.accelerator.state == constants.FPGA_ACCELERATOR_ASSIGNED
    assert p.accelerator.num_mmio == 2
    assert p.accelerator.num_interrupts == 4


def test_fpga_remote_id():
    i = fpga_remote_id('host@lab.net', 1)

    assert i.to_json_obj() == {'hostname': 'host@lab.net',
                               'unique_id': '1'}
    print(i.to_json_str())

    i = fpga_remote_id.from_json_obj({'hostname': 'foo@bar.net',
                                      'unique_id': '2'})
    assert i.hostname == 'foo@bar.net'
    assert i.unique_id == 2


def test_token():
    values = [0x8086,
              0xbcce,
              0x0000,
              0x5e,
              0x00,
              0,
              constants.FPGA_IFC_VFIO,
              constants.FPGA_ACCELERATOR,
              0x11ff1155,
              'D8424DC4-A4A3-C413-F89E-433683F9040B',
              0x8086,
              0x17d4,
              fpga_remote_id('host@lab.net', 1),
             ]

    t = token()
    for i, k in enumerate(token.attrs):
        setattr(t, k, values[i])
        assert getattr(t, k) == values[i]
    print(to_json_str(t))

    t = token.from_json_obj({
        'vendor_id': 0x8086,
        'device_id': 0xbcce,
        'segment': 0x0000,
        'bus': 0x5e,
        'device': 0x00,
        'function': 0,
        'interface': "FPGA_IFC_VFIO",
        'objtype': "FPGA_ACCELERATOR",
        'object_id': "301928789",
        'guid': 'D8424DC4-A4A3-C413-F89E-433683F9040B',
        'subsystem_vendor_id': 0x8086,
        'subsystem_device_id': 0x17d4,
        'token_id': {'hostname': 'foo@bar.net', 'unique_id': '2'},
        })
    print(to_json_str(t))


def test_enumerate(url):
    p = properties()
    p.objtype = constants.FPGA_ACCELERATOR

    tokens = enum.enumerate(url, [p], 1)

    for t in tokens:
        print(t.to_json_str())

    return tokens


def test_open(tok):
    return tok.open(0)


def test_close(h):
    h.close()


def test_destroy(tok):
    tok.destroy()


def main():
    parser, args = parse_args()

    #test_version()
    #test_properties()
    #test_fpga_remote_id()
    #test_token()
    tokens = test_enumerate(http_url(args))
    h = test_open(tokens[0])

    test_close(h)
    test_destroy(tokens[0])



if __name__ == '__main__':
    main()
