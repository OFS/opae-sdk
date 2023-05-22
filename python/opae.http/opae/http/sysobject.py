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

import requests

import opae.http.constants as constants
from opae.http.fpga_remote_id import fpga_remote_id


class sysobject():
    def __init__(self, url, name, object_id, debug):
        self.url = url
        self.name = name
        self.object_id = object_id
        self.debug = debug

    def destroy(self):
        url = self.url + '/fpga/v1/sysobject/destroy'

        req = {'object_id': self.object_id.to_json_obj()}

        if self.debug:
            print(url)
            print(f'fpgaDestroyObject Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.debug:
            print(f'fpgaDestroyObject Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaDestroyObject returned')

    def get_type(self):
        url = self.url + '/fpga/v1/sysobject/type/get'

        req = {'object_id': self.object_id.to_json_obj()}

        if self.debug:
            print(url)
            print(f'fpgaObjectGetType Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.debug:
            print(f'fpgaObjectGetType Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaObjectGetType returned')

        return constants.fpga_sysobject_type_from_str(jobj['type'])

    def get_name(self):
        url = self.url + '/fpga/v1/sysobject/name/get'

        req = {'object_id': self.object_id.to_json_obj()}

        if self.debug:
            print(url)
            print(f'fpgaObjectGetName Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.debug:
            print(f'fpgaObjectGetName Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaObjectGetName returned')

        return jobj['name']

    def get_size(self, flags):
        url = self.url + '/fpga/v1/sysobject/size/get'

        req = {'object_id': self.object_id.to_json_obj(),
               'flags': flags}

        if self.debug:
            print(url)
            print(f'fpgaObjectGetSize Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.debug:
            print(f'fpgaObjectGetSize Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaObjectGetSize returned')

        return jobj['value']

    def read(self, offset, length, flags):
        url = self.url + '/fpga/v1/sysobject/read'

        req = {'object_id': self.object_id.to_json_obj(),
               'offset': str(offset),
               'length': str(length),
               'flags': flags}

        if self.debug:
            print(url)
            print(f'fpgaObjectRead Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.debug:
            print(f'fpgaObjectRead Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaObjectRead returned')

        return jobj['value']

    def read64(self, flags):
        url = self.url + '/fpga/v1/sysobject/read64'

        req = {'object_id': self.object_id.to_json_obj(),
               'flags': flags}

        if self.debug:
            print(url)
            print(f'fpgaObjectRead64 Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.debug:
            print(f'fpgaObjectRead64 Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaObjectRead64 returned')

        return int(jobj['value'])

    def write64(self, value, flags):
        url = self.url + '/fpga/v1/sysobject/write64'

        req = {'object_id': self.object_id.to_json_obj(),
               'value': str(value),
               'flags': flags}

        if self.debug:
            print(url)
            print(f'fpgaObjectWrite64 Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.debug:
            print(f'fpgaObjectWrite64 Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaObjectWrite64 returned')

    def get_object(self, name, flags):
        url = self.url + '/fpga/v1/sysobject/get'

        req = {'object_id': self.object_id.to_json_obj(),
               'name': name,
               'flags': flags}

        if self.debug:
            print(url)
            print(f'fpgaObjectGetObject Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.debug:
            print(f'fpgaObjectGetObject Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaObjectGetObject returned')

        return sysobject(self.url,
                         name,
                         fpga_remote_id.from_json_obj(jobj['objectId']),
                         self.debug)

    def get_object_at(self, index):
        url = self.url + '/fpga/v1/sysobject/get/at'

        req = {'object_id': self.object_id.to_json_obj(),
               'index': str(index)}

        if self.debug:
            print(url)
            print(f'fpgaObjectGetObjectAt Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.debug:
            print(f'fpgaObjectGetObjectAt Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaObjectGetObjectAt returned')

        return sysobject(self.url,
                         str(index),
                         fpga_remote_id.from_json_obj(jobj['objectId']),
                         self.debug)
