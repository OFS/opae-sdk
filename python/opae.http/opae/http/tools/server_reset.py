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
import requests

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


def main():
    parser, args = parse_args()

    url = f'{args.protocol}{args.server}:{args.port}/fpga/v1/server/reset'
    r = requests.post(url)
    print(r.text)


if __name__ == '__main__':
    main()
