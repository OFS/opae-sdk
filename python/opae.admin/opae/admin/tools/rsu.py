#!/usr/bin/env python
# Copyright(c) 2019, Intel Corporation
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
import fcntl
import logging
import sys
import glob
import os
import re

from opae.admin.fpga import fpga
from opae.admin.sysfs import pci_node, PCI_ADDRESS_RE

if sys.version_info[0] == 2:
    import cPickle as pickle  # pylint: disable=E0401
else:
    import pickle  # pylint: disable=E0401


RSU_LOCK_FILE = "/tmp/rsu_lock"

USER_BOOT_PAGE = 'user'
FACTORY_BOOT_PAGE = 'factory'

BOOT_PAGE = {0x0b30: {'fpga': {'user': 1,
                               'factory': 0},
                      'bmcimg': {'user': 0,
                                 'factory': 1}},
             0x0b2b: {'fpga': {'user': 1,
                               'factory': 0},
                      'bmcimg': {'user': 1,
                                 'factory': 0}}}

logger = logging.getLogger('rsu')


def parse_args():
    descr = 'A tool to test RSU.'

    epi = 'example usage:\n\n'
    epi += '    rsu.py fpga 25:00.0\n\n'

    fc_ = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(description=descr, epilog=epi,
                                     formatter_class=fc_)
    parser.add_argument('type', help='type of operation',
                        choices=fpga.BOOT_TYPES + ['disable-card',
                                                   'enable-card'])
    bdf_help = "bdf of device to do rsu (e.g. 04:00.0 or 0000:04:00.0)"
    parser.add_argument('bdf', nargs='?', help=bdf_help)
    opt_help = "reload from factory image"
    parser.add_argument('-f', '--factory', action='store_true', help=opt_help)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='debug mode')
    return parser.parse_args()


def normalize_bdf(bdf):
    pat = r'[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$'
    if re.match(pat, bdf):
        return bdf

    if re.match(r'[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$', bdf):
        return "0000:{}".format(bdf)
    logger.warn('invalid bdf: {}'.format(bdf))
    raise SystemExit(os.EX_USAGE)


def do_rsu(rsu_type, bdf, boot_page):
    devices = fpga.enum([{'pci_node.pci_address': bdf}])
    if not devices:
        logger.error('Could not locate device at {}'.format(bdf))
        raise SystemExit(os.EX_UNAVAILABLE)
    if len(devices) > 1:
        logger.error('Found more than one device at: {}'.format(bdf))
        raise SystemExit(os.EX_SOFTWARE)

    device = devices[0]
    dev_id = device.pci_node.device_id
    try:
        boot_number = BOOT_PAGE[dev_id][rsu_type][boot_page]
    except KeyError:
        logger.error('could not determine boot page for given device: %s',
                     device.pci_node)
        raise SystemExit(os.EX_SOFTWARE)
    else:
        device.safe_rsu_boot(boot_number, type=rsu_type)


def main():
    args = parse_args()
    level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(level=level,
                        format='%(asctime)s - %(message)s')
    if args.bdf is None:
        if args.type == 'enable-card':
            paths = glob.glob(os.path.join('/tmp/*:*:*.*'))
            if len(paths) == 1:
                args.bdf = os.path.basename(paths[0])
        if args.bdf is None:
            logger.warn("Please specify bdf as [bus]:[device].[function]")
            raise SystemError(os.EX_USAGE)
    bdf = normalize_bdf(args.bdf)
    boot_page = FACTORY_BOOT_PAGE if args.factory else USER_BOOT_PAGE

    if args.type in fpga.BOOT_TYPES:
        with open(RSU_LOCK_FILE, 'w') as flock:
            fcntl.flock(flock.fileno(), fcntl.LOCK_EX)
            do_rsu(args.type, bdf, boot_page)
    elif args.type == 'disable-card':
        devices = fpga.enum([{'pci_node.pci_address': bdf}])
        device = devices[0]
        root = device.pci_node.root
        to_remove = device.pci_node.branch[1]
        with open(os.path.join("/tmp/", bdf), 'w') as f:
            pickle.dump({'root': root.pci_address,
                         'domain': to_remove.domain,
                         'bus': to_remove.bus,
                         'aer': root.aer}, f)
        root.aer = (0xFFFFFFFF, 0xFFFFFFFF)
        to_remove.remove()
    elif args.type == 'enable-card':
        with open(os.path.join("/tmp", bdf), 'r') as f:
            data = pickle.load(f)

        root_info = PCI_ADDRESS_RE.match(data['root']).groupdict()
        root = pci_node(root_info)
        root.rescan_bus('{domain}:{bus}'.format(**data))
        root.aer = data['aer']


if __name__ == "__main__":
    main()
