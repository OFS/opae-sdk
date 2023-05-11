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

#import base64
import json
import requests
#import struct
#import sys

from opae.http.conversion import to_json_obj
from opae.http.fpga_remote_id import fpga_remote_id
import opae.http.constants as constants


class shared_buffer():
    def __init__(self, h, buf_id):
        self.handle = h
        self.buf_id = buf_id

    def ioaddr(self):
        tok = self.handle.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/ioaddress'
        print(url)

        req = {'handle_id': self.handle.handle_id.to_json_obj(),
               'buf_id': self.buf_id.to_json_obj()}

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetIOAddress returned')
        return int(jobj['ioaddr'])

