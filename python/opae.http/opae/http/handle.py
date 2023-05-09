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

from opae.http.conversion import to_json_obj
from opae.http.fpga_remote_id import fpga_remote_id


class handle():
    MAGIC = 0x46504741484e444c

    attrs = {
             'magic':     [],
             'token_id':  ['tokenId'],
             'handle_id': ['handleId'],
            }

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
        url = os.path.join(tok.__dict__['_url'], 'fpga/v1/handle/close')
        print(url)

        req = {'handle_id': self.handle_id.to_json_obj()}

        resp = requests.post(url, json=req)

        resp.raise_for_status()
