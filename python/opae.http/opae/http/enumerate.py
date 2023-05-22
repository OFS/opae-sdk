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
from opae.http.conversion import to_json_obj
from opae.http.token import token


def enumerate(url, filters, max_tokens, debug=False):
    """Given a server URL and a list of filters (properties objects),
       retrieve a list of token objects that match the filter criteria.

       url - eg, http://my.server.net:8080

       filters - a list of populated properites objects.
       eg,
       p = properties()
       p.objtype = constants.FPGA_ACCELERATOR
       enumerate(url, [p], 1)

       max_tokens - the maximum number of tokens to return
    """
    req = {'filters': list(map(to_json_obj, filters)),
           'num_filters': len(filters),
           'max_tokens': max_tokens,
          }

    enum_url = url + '/fpga/v1/enumerate'
    if debug:
        print(enum_url)
        print(f'fpgaEnumerate Request: {req}')

    tokens = []
    resp = requests.post(enum_url, json=req)

    resp.raise_for_status()

    if debug:
        print(f'fpgaEnumerate Response: {resp.text}')

    jobj = resp.json()

    constants.raise_for_error(jobj['result'], 'fpgaEnumerate returned')

    for tok in jobj['tokens']:
        tokens.append(token.from_json_obj(tok))

    for tok in tokens:
        tok.__dict__['_url'] = url
        tok.__dict__['_debug'] = debug

    return tokens
