#!/usr/bin/env python3
# Copyright(c) 2019-2021, Intel Corporation
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

from __future__ import absolute_import
import argparse
import fcntl
import logging
import os
import re
import sys

from opae.admin.fpga import fpga

try:
    from pathlib import Path
except ImportError:
    from pathlib2 import Path  # noqa


RSU_LOCK_DIR = '/var/lib/opae'
RSU_LOCK_FILE = os.path.join(RSU_LOCK_DIR, 'rsu_lock')

FPGA_TO_AVAILABLE_IMAGES = {
    '0': 'fpga_user1',
    '1': 'fpga_user2',
    'factory': 'fpga_factory'
}

logger = logging.getLogger('rsu')

DESCRIPTION = '''
Perform RSU (remote system update) operation on PAC device
given its PCIe address.
An RSU operation sends an instruction to the device to trigger
a power cycle of the card only. This will force reconfiguration
from flash for the BMC image.
'''

EPILOG = '''
Example usage:

     %(prog)s bmcimg 25:00.0
     This will trigger a boot of the BMC image for a device with a pci address
     of 25:00.0.
     NOTE: Both BMC and FPGA images will be reconfigured from user bank.

     %(prog)s bmcimg 25:00.0 -f
     This will trigger a factory boot of the BMC image for a device with a
     pci address of 25:00.0.
     NOTE: Both BMC image will be reconfigured from factory bank and the
           FPGA image will be reconfigured from the user bank.
'''


def parse_args():
    fc_ = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(description=DESCRIPTION, epilog=EPILOG,
                                     formatter_class=fc_)
    parser.add_argument('type', help='type of operation',
                        choices=fpga.BOOT_TYPES)
    parser.add_argument('bdf', nargs='?',
                        help=('PCIe address of device to do rsu '
                              '(e.g. 04:00.0 or 0000:04:00.0)'))
    parser.add_argument('-f', '--factory', action='store_true',
                        help='reload from factory bank')
    parser.add_argument('-d', '--debug', action='store_true',
                        help='log debug statements')
    parser.add_argument('-p', '--page', choices=['0', '1', 'factory'],
                        default='0', help='select fpga page')
    return parser.parse_args()


def normalize_bdf(bdf):
    pat = r'[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$'
    if re.match(pat, bdf):
        return bdf

    if re.match(r'[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$', bdf):
        return "0000:{}".format(bdf)
    logger.warn('invalid bdf: {}'.format(bdf))
    raise SystemExit(os.EX_USAGE)


def do_rsu(rsu_type, device, factory):
    device.safe_rsu_boot(factory, type=rsu_type)


def main():
    args = parse_args()
    level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(level=level,
                        format='%(asctime)s - %(message)s')
    compatible = fpga.enum([{'supports_rsu': True}])
    if not compatible:
        sys.stderr.write('No compatible devices found\n')
        raise SystemExit(os.EX_USAGE)

    if args.bdf is None:
        if len(compatible) == 1:
            args.bdf = compatible[0].pci_node.pci_address
        elif len(compatible) > 1:
            prog = os.path.basename(sys.argv[0])
            sys.stderr.write(('Please specify PCIe address as '
                              '[<segment>:]<bus>:<device>.<function>\n'))
            sys.stderr.write('Acceptable commands:\n')
            for dev in compatible:
                sys.stderr.write('>{} {} {}\n'.format(prog,
                                                      args.type,
                                                      dev.pci_node.bdf))
            raise SystemExit(os.EX_USAGE)

    bdf = normalize_bdf(args.bdf)

    if args.type == 'fpga':
        args.type = FPGA_TO_AVAILABLE_IMAGES[args.page]

    Path(RSU_LOCK_DIR).mkdir(parents=True, exist_ok=True)

    for device in compatible:
        if device.pci_node.pci_address == bdf:
            exit_code = os.EX_IOERR
            with open(RSU_LOCK_FILE, 'w') as flock:
                fcntl.flock(flock.fileno(), fcntl.LOCK_EX)
                try:
                    do_rsu(args.type, device, args.factory)
                except IOError:
                    logging.error('RSU operation failed')
                else:
                    exit_code = os.EX_OK
                    logging.info('RSU operation complete')
                finally:
                    fcntl.flock(flock.fileno(), fcntl.LOCK_UN)
            raise SystemExit(exit_code)

    logging.error(('PCIe address (%s) is invalid or does not identify a'
                   'compatible device'), args.bdf)


if __name__ == "__main__":
    main()
