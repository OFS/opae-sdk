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
import requests

from opae.http.conversion import to_json_obj
from opae.http.fpga_remote_id import fpga_remote_id
import opae.http.constants as constants


class shared_buffer():
    def __init__(self, h, buf_id, length, debug):
        self.handle = h
        self.buf_id = buf_id
        self.length = length
        self.debug = debug

    def destroy(self):
        shared = self.handle.__dict__['_shared_buffers']
        if self not in shared:
            msg = f'buffer {self} is not tracked by {self.handle}'
            constants.raise_for_error(constants.FPGA_INVALID_PARAM, msg)

        tok = self.handle.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/release'
        if self.debug:
            print(url)

        req = {'handle_id': self.handle.handle_id.to_json_obj(),
               'buf_id': self.buf_id.to_json_obj()}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaReleaseBuffer returned')
        shared.remove(self)

    def ioaddr(self):
        tok = self.handle.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/ioaddress'
        if self.debug:
            print(url)

        req = {'handle_id': self.handle.handle_id.to_json_obj(),
               'buf_id': self.buf_id.to_json_obj()}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetIOAddress returned')
        return int(jobj['ioaddr'])

    def memset(self, offset, c, n):
        tok = self.handle.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/memset'
        if self.debug:
            print(url)

        req = {'handle_id': self.handle.handle_id.to_json_obj(),
               'buf_id': self.buf_id.to_json_obj(),
               'offset': str(offset),
               'c': c,
               'n': str(n)}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaBufMemSet returned')

    def memcpy(self, dest_offset, src_data, num_bytes):
        tok = self.handle.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/memcpy'
        if self.debug:
            print(url)

        encoded = base64.b64encode(src_data)

        req = {'handle_id': self.handle.handle_id.to_json_obj(),
               'dest_buf_id': self.buf_id.to_json_obj(),
               'dest_offset': str(dest_offset),
               'src': encoded.decode(),
               'n': str(num_bytes)}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaBufMemCpyToRemote returned')

    def poll(self, offset, width, mask, expected_value, sleep_interval, loops_timeout):
        tok = self.handle.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/poll'
        if self.debug:
            print(url)

        req = {'handle_id': self.handle.handle_id.to_json_obj(),
               'buf_id': self.buf_id.to_json_obj(),
               'offset': str(offset),
               'width': width,
               'mask': str(mask),
               'expected_value': str(expected_value),
               'sleep_interval': str(sleep_interval),
               'loops_timeout': str(loops_timeout)}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        result = constants.fpga_result_from_str(jobj['result'])

        if result not in [constants.FPGA_OK, constants.FPGA_NOT_FOUND]:
            constants.raise_for_error(result, 'fpgaBufPoll returned')

        return result

    def memcmp(self, other, bufa_offset, bufb_offset, n):
        tok = self.handle.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/compare'
        if self.debug:
            print(url)

        req = {'handle_id': self.handle.handle_id.to_json_obj(),
               'bufa_id': self.buf_id.to_json_obj(),
               'bufa_offset': str(bufa_offset),
               'bufb_id': other.buf_id.to_json_obj(),
               'bufb_offset': str(bufb_offset),
               'n': str(n)}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaBufMemCmp returned')

        return jobj['cmpResult']

    def write_pattern(self, pattern_name):
        tok = self.handle.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/pattern/write'
        if self.debug:
            print(url)

        req = {'handle_id': self.handle.handle_id.to_json_obj(),
               'buf_id': self.buf_id.to_json_obj(),
               'pattern_name': pattern_name}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaBufWritePattern returned')
