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

import json
import os
import requests

import opae.http.constants as constants
from opae.http.conversion import to_json_obj
from opae.http.fpga_remote_id import fpga_remote_id
from opae.http.handle import handle


class token():
    MAGIC = 0x46504741544f4b4e

    attrs = {
             'magic':               [],
             'vendor_id':           ['vendorId'],
             'device_id':           ['deviceId'],
             'segment':             [],
             'bus':                 [],
             'device':              [],
             'function':            [],
             'interface':           [],
             'objtype':             [],
             'object_id':           ['objectId'],
             'guid':                [],
             'subsystem_vendor_id': ['subsystemVendorId'],
             'subsystem_device_id': ['subsystemDeviceId'],
             'token_id':            ['tokenId'],
            }

    def __getattr__(self, attr):
        if attr in token.attrs:
            return self.__dict__[attr]
        raise AttributeError(attr + ' is not a valid token attr')

    def __setattr__(self, attr, value):
        if attr in token.attrs:
            self.__dict__[attr] = value
        else:
            raise AttributeError(attr + ' is not a token attr')

    def to_json_obj(self):
        d = {}
        d['magic'] = str(token.MAGIC)
        transforms = {'objtype': constants.fpga_objtype_to_str,
                      'interface': constants.fpga_interface_to_str,
                      'object_id': str,
                      'token_id': to_json_obj,
                     }
        for attr in token.attrs:
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
        for attr, aliases in token.attrs.items():
            if attr in c:
                continue
            for a in aliases:
                if a in c:
                    c[attr] = c[a]
                    del c[a]
        c['token_id'] = fpga_remote_id.resolve_aliases(c['token_id'])
        return c

    @staticmethod
    def from_json_obj(jobj):
        tok = token()
        transforms = {'objtype': constants.fpga_objtype_from_str,
                      'interface': constants.fpga_interface_from_str,
                      'object_id': int,
                      'token_id': fpga_remote_id.from_json_obj,
                     }
        for k, v in token.resolve_aliases(jobj).items():
            if k in transforms:
                setattr(tok, k, transforms[k](v))
            else:
                setattr(tok, k, v)
        return tok

    def destroy(self):
        url = os.path.join(self.__dict__['_url'], 'fpga/v1/token/destroy')
        print(url)

        req = {'token_id': self.token_id.to_json_obj()}
        print(req)

        resp = requests.post(url, json=req)

        resp.raise_for_status()

    def open(self, flags):
        url = os.path.join(self.__dict__['_url'], 'fpga/v1/token/open')
        print(url)

        req = {'token_id': self.token_id.to_json_obj(),
               'flags': flags}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        h = handle.from_json_obj(jobj['handle'])
        h.__dict__['_token'] = self

        return h
