#!/usr/bin/env python
# Copyright(c) 2018, Intel Corporation
##
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
##
# * Redistributions of  source code  must retain the  above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
# may be used to  endorse or promote  products derived  from this  software
# without specific prior written permission.
##
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
import logging
import argparse
import uuid
import sys
from opae import properties, token, handle, OPEN_SHARED, ACCELERATOR


def _parse_num(value):
    try:
        return int(value, 16)
    except TypeError:
        try:
            return int(value, 10)
        except TypeError:
            return value


def _get_accel(args):
    props = properties()
    props.type = ACCELERATOR
    if args.bus:
        props.bus = _parse_num(args.bus)
    if args.guid:
        props.guid = str(uuid.UUID(args.guid))
    tokens = token.enumerate([props])
    if not tokens:
        logging.warning("No resources found using filter")
        sys.exit(-1)
    if len(tokens) > 1:
        logging.warning("Found more than one resourc, using first one")
    mode = 0 if args.exclusive_mode else OPEN_SHARED
    accel = handle.open(tokens[0], mode)
    if not accel:
        logging.error("Could not open accelerator")
        sys.exit(-1)
    return accel


def peek(args):
    accel = _get_accel(args)
    offset = _parse_num(args.offset)
    print hex(accel.read_csr64(offset, 0))


def poke(args):
    accel = _get_accel(args)
    offset = _parse_num(args.offset)
    value = _parse_num(args.value)
    accel.write_csr64(offset, value, 0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bus',
                        help='pcie bus to look for accelerator')
    parser.add_argument('-g', '--guid',
                        help='GUID to use to look for accelerator')
    parser.add_argument('--exclusive-mode', default=False, action='store_true',
                        help='Open accelerator in exclusive mode'
                             'Default is shared mode')
    subparsers = parser.add_subparsers(title='commands')

    peek_parser = subparsers.add_parser('peek')
    peek_parser.add_argument('offset')
    peek_parser.set_defaults(func=peek)

    poke_parser = subparsers.add_parser('poke')
    poke_parser.add_argument('offset',
                             help='register offset to write to')
    poke_parser.add_argument('value',
                             help='value to write to register')
    poke_parser.set_defaults(func=poke)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
