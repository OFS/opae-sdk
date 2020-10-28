#!/usr/bin/env python3
# Copyright(c) 2020, Intel Corporation
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

""" Bind/Unbind a PCIe device to/from uio. """

import argparse
import grp
import os
import pwd
import re
import subprocess
import sys
import time

OPAEUIO_VERSION = '1.0.0'

ABBREV_PCI_ADDR_PATTERN = r'([\da-fA-F]{2}):' \
                          r'([\da-fA-F]{2})\.' \
                          r'(\d)'
ABBREV_PCI_ADDR_REGEX = re.compile(ABBREV_PCI_ADDR_PATTERN)
FULL_PCI_ADDR_PATTERN = r'([\da-fA-F]{4}):' + ABBREV_PCI_ADDR_PATTERN
FULL_PCI_ADDR_REGEX = re.compile(FULL_PCI_ADDR_PATTERN)


def normalized_pci_addr(addr):
    """Format a PCIe device address to its canonical form.

    addr - a PCIe address string of the form bb:dd.f or ssss:bb:dd.f

    Returns the full PCIe address of the form ssss:bb:dd.f, or
    None if addr is not formatted correctly.
    """
    match = ABBREV_PCI_ADDR_REGEX.match(addr)
    if match:
        return '0000:' + addr
    else:
        match = FULL_PCI_ADDR_REGEX.match(addr)
        if match:
            return addr
    return None


def parse_args():
    """Parse script arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('device', nargs='?',
                        help='DFL device name')
    parser.add_argument('-i', '--init', default=False, action='store_true',
                        help='initialize the given device for vfio')
    parser.add_argument('-r', '--release', default=False, action='store_true',
                        help='release the given device from vfio')
    #parser.add_argument('-d', '--driver', default='dfl-pci',
    #                    help='driver to re-bind on release')
    parser.add_argument('-u', '--user', default='root',
                        help='userid to assign during init')
    parser.add_argument('-g', '--group', default='root',
                        help='groupid to assign during init')
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s {}'.format(OPAEUIO_VERSION),
                        help='display version information and exit')
    return parser, parser.parse_args()



def main():
    """ Main script function."""
    parser, args = parse_args()

    if not args.device:
        print('Error: DFL device name is a required argument\n')
        parser.print_help(sys.stderr)
        sys.exit(1)

    #addr = normalized_pci_addr(args.addr)
    #if not addr:
    #    print('Invalid PCIe address: {}'.format(args.addr))
    #    parser.print_help(sys.stderr)
    #    sys.exit(1)

    #if args.init:
    #    initialize_vfio(addr, '{}:{}'.format(args.user, args.group))
    #elif args.release:
    #    release_vfio(addr, args.driver)
    #else:
    #    parser.print_help(sys.stderr)
    #    sys.exit(1)


if __name__ == '__main__':
    main()
