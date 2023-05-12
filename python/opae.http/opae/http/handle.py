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

import base64
import json
import requests
import struct
import sys

from opae.http.conversion import to_json_obj
from opae.http.fpga_remote_id import fpga_remote_id
import opae.http.constants as constants
from opae.http.properties import properties
from opae.http.shared_buffer import shared_buffer
from opae.http.sysobject import sysobject


class handle():
    MAGIC = 0x46504741484e444c

    attrs = {
             'magic':     [],
             'token_id':  ['tokenId'],
             'handle_id': ['handleId'],
            }

    def __init__(self):
        self.__dict__['_mmio_map'] = {}
        self.__dict__['_shared_buffers'] = []

    def __getattr__(self, attr):
        if attr in handle.attrs:
            return self.__dict__[attr]
        raise AttributeError(attr + ' is not a valid handle attr')

    def __setattr__(self, attr, value):
        if attr in handle.attrs:
            self.__dict__[attr] = value
        else:
            raise AttributeError(attr + ' is not a handle attr')

    def to_json_obj(self):
        d = {}
        d['magic'] = str(handle.MAGIC)
        transforms = {'token_id': to_json_obj,
                      'handle_id': to_json_obj,
                     }
        for attr in handle.attrs:
            if attr in transforms:
                d[attr] = transforms[attr](self.__dict__[attr])
            else:
                d[attr] = self.__dict__[attr]
        return d

    def to_json_str(self):
        return json.dumps(self.to_json_obj())

    @staticmethod
    def resolve_aliases(d):
        c = d.copy()
        for attr, aliases in handle.attrs.items():
            if attr in c:
                continue
            for a in aliases:
                if a in c:
                    c[attr] = c[a]
                    del c[a]
        c['token_id'] = fpga_remote_id.resolve_aliases(c['token_id'])
        c['handle_id'] = fpga_remote_id.resolve_aliases(c['handle_id'])
        return c

    @staticmethod
    def from_json_obj(jobj):
        h = handle()
        transforms = {'token_id': fpga_remote_id.from_json_obj,
                      'handle_id': fpga_remote_id.from_json_obj,
                     }
        for k, v in handle.resolve_aliases(jobj).items():
            if k in transforms:
                setattr(h, k, transforms[k](v))
            else:
                setattr(h, k, v)
        return h

    def close(self):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/close'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj()}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()
        constants.raise_for_error(jobj['result'], 'fpgaClose returned')

    def reset(self):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/afu/reset'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj()}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()
        constants.raise_for_error(jobj['result'], 'fpgaReset returned')

    def get_properties(self):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/properties/get'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj()}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetPropertiesFromHandle returned')

        return properties.from_json_obj(jobj['properties'])

    def map_mmio(self, region_num):
        map_dict = self.__dict__['_mmio_map']
        if region_num in map_dict:
            msg = f'MMIO region {region_num} is already mapped.'
            constants.raise_for_error(constants.FPGA_BUSY, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/map'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaMapMMIO returned')

        map_dict[region_num] = fpga_remote_id.from_json_obj(jobj['mmioId'])

    def unmap_mmio(self, region_num):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/unmap'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_id': map_dict[region_num].to_json_obj(),
               'mmio_num': region_num}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaUnmapMMIO returned')
        del map_dict[region_num]

    def read32(self, region_num, offset):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/read32'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset)}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaReadMMIO32 returned')
        return jobj['value']

    def read64(self, region_num, offset):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/read64'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset)}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaReadMMIO64 returned')
        return int(jobj['value'])

    def write32(self, region_num, offset, value):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/write32'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset),
               'value': value}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaWriteMMIO32 returned')

    def write64(self, region_num, offset, value):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/write64'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset),
               'value': str(value)}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaWriteMMIO64 returned')

    def write512(self, region_num, offset, values):
        """values is interpreted as a list of 8-byte integers.
        """
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        if len(values) != 8: # 8 64-bit integers = 512 bits
            msg = 'write512: values should be a list of eight 8-byte integers'
            constants.raise_for_error(constants.FPGA_INVALID_PARAM, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/write512'
        print(url)

        order = '<' if sys.byteorder == 'little' else '>'

        packed = struct.pack(f'{order}8Q', *values)
        encoded = base64.b64encode(packed)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset),
               'values': encoded.decode()}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaWriteMMIO512 returned')

    def prepare_buffer(self, length, pre_allocated_addr=0, flags=0):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/prepare'
        print(url)

        pre = (flags & constants.FPGA_BUF_PREALLOCATED) != 0

        req = {'handle_id': self.handle_id.to_json_obj(),
               'length': str(length),
               'have_buf_addr': True,
               'pre_allocated_addr': str(pre_allocated_addr) if pre else '0',
               'flags': flags}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaPrepareBuffer returned')

        buf_id = fpga_remote_id.from_json_obj(jobj['bufId'])

        buffer = shared_buffer(self, buf_id)
        self.__dict__['_shared_buffers'].append(buffer)

        return buffer

    def release_buffer(self, buffer):
        shared = self.__dict__['_shared_buffers']
        if buffer not in shared:
            msg = f'buffer {buffer} is not tracked by {self}'
            constants.raise_for_error(constants.FPGA_INVALID_PARAM, msg)

        if buffer.handle != self:
            msg = f'buffer {buffer} / handle {self} mismatch'
            constants.raise_for_error(constants.FPGA_INVALID_PARAM, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/release'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'buf_id': buffer.buf_id.to_json_obj()}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaReleaseBuffer returned')
        shared.remove(buffer)

    def get_object(self, name, flags):
        tok = self.__dict__['_token']
        save_url = tok.__dict__['_url']
        url = save_url + '/fpga/v1/handle/sysobject/get'
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'name': name,
               'flags': flags}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaHandleGetObject returned')

        return sysobject(save_url, name, fpga_remote_id.from_json_obj(jobj['objectId']))
