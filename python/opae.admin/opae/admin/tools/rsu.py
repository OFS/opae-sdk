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

logger = logging.getLogger('rsu')

DESCRIPTION = '''
Perform RSU (Remote System Update) operation on PAC device given its
PCIe address. An RSU operation sends an instruction to the device to
trigger a power cycle of the card only. This will force reconfiguration
from flash for the given image.
'''

EPILOG = '''
Example usage:

     %(prog)s bmc 25:00.0
     This will trigger a boot of the BMC image for the device with PCIe
     address 25:00.0.
     NOTE: The BMC image will be reconfigured from user bank and the
           FPGA image will be reconfigured using the default setting.

     %(prog)s bmc 25:00.0 --page=factory
     This will trigger a factory boot of the BMC image for the device
     with PCIe address 25:00.0.
     NOTE: The BMC image will be reconfigured from factory bank and the
           FPGA image will be reconfigured using the default setting.

     %(prog)s fpgadefault 25:00.0 --page=factory --fallback=user1,user2
     This sets the default FPGA image of the device with PCIe address
     25:00.0 to the FPGA Factory image, with a fallback to FPGA user1
     then FPGA user2.
'''


def fpga_defaults_valid(pci_id, value):
    sequences = { (0x8086, 0xaf00): [ 'fpga_user1',
                                      'fpga_user2',
                                      'fpga_factory',
                                      'fpga_factory fpga_user1',
                                      'fpga_factory fpga_user2',
                                      'fpga_factory fpga_user1 fpga_user2',
                                      'fpga_factory fpga_user2 fpga_user1'
                                    ],
                  (0x8086, 0xaf01): [ 'fpga_user1',
                                      'fpga_user2',
                                      'fpga_factory',
                                      'fpga_factory fpga_user1',
                                      'fpga_factory fpga_user2',
                                      'fpga_factory fpga_user1 fpga_user2',
                                      'fpga_factory fpga_user2 fpga_user1'
                                    ],
                  (0x8086, 0xbcce): [ 'fpga_user1',
                                      'fpga_user2',
                                      'fpga_factory',
                                      'fpga_factory fpga_user1',
                                      'fpga_factory fpga_user2',
                                      'fpga_factory fpga_user1 fpga_user2',
                                      'fpga_factory fpga_user2 fpga_user1'
                                    ]
                }
    return value in sequences[pci_id]


def set_fpga_default(device, args):
    secure_update = device.secure_update
    if not secure_update:
        logging.error('failed to find secure '
                      'attributes for {}'.format(device.pci_node.pci_address))
        raise IOError

    power_on_image = secure_update.find_one('control/power_on_image')

    if not args.page and not args.fallback:
        # Print the power_on_image value.
        logging.info('fallback sequence: {}'.format(power_on_image.value))
        return
    elif not args.page and args.fallback:
        logging.error('--fallback must be accompanied by a --page selection.')
        raise IOError

    values = ['fpga_' + args.page]

    if args.fallback:
        fb = args.fallback
        fb = fb.replace(',', ' ')   # allow a comma-separated list
        fb = re.sub('\s+', ' ', fb) # compress whitespace
        fb = fb.strip()
        fb = fb.rstrip()

        values += ['fpga_' + x for x in fb.split(' ')]

    value = ' '.join(values)

    try:
        if fpga_defaults_valid(device.pci_node.pci_id, value):
            logging.info('Setting default FPGA image: {}'.format(value))
            power_on_image.value = value
        else:
            logging.error('boot sequence {} is not valid for {}'.format(
                          value, device.pci_node.pci_address))
            raise IOError
    except KeyError:
        logging.error('Setting a default FPGA image '
                      'is not available for {}'.format(
                      device.pci_node.pci_address))
        raise IOError


def device_rsu(device, available_image):
    device.safe_rsu_boot(available_image)


def device_rsu_bmc(device, args):
    device_rsu(device, 'bmc_' + args.page)


def device_rsu_retimer(device, args):
    device_rsu(device, 'retimer_fw')


def device_rsu_fpga(device, args):
    device_rsu(device, 'fpga_' + args.page)


def device_rsu_sdm(device, args):
    device_rsu(device, 'sdm')


def parse_args():
    fc_ = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(description=DESCRIPTION, epilog=EPILOG,
                                     formatter_class=fc_)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='log debug statements')

    subparser = parser.add_subparsers(dest='which')

    bmcimg = subparser.add_parser('bmc', aliases=['bmcimg'],
                                  help='RSU BMC Image')
    bmcimg.add_argument('bdf', nargs='?',
                        help=('PCIe address '
                              '(eg 04:00.0 or 0000:04:00.0)'))
    bmcimg.add_argument('-p', '--page', choices=['user', 'factory'],
                        default='user', help='select BMC page')
    bmcimg.set_defaults(func=device_rsu_bmc)

    retimer = subparser.add_parser('retimer', help='RSU Retimer Image')
    retimer.add_argument('bdf', nargs='?',
                         help=('PCIe address '
                               '(eg 04:00.0 or 0000:04:00.0)'))
    retimer.set_defaults(func=device_rsu_retimer)

    fpga_img = subparser.add_parser('fpga', help='RSU FPGA Image')
    fpga_img.add_argument('bdf', nargs='?',
                          help=('PCIe address '
                                '(eg 04:00.0 or 0000:04:00.0)'))
    fpga_img.add_argument('-p', '--page',
                          choices=['user1', 'user2', 'factory'],
                          default='user1', help='select FPGA page')
    fpga_img.set_defaults(func=device_rsu_fpga)

    sdm = subparser.add_parser('sdm', help='RSU SDM Image')
    sdm.add_argument('bdf', nargs='?',
                     help=('PCIe address '
                           '(eg 04:00.0 or 0000:04:00.0)'))
    sdm.set_defaults(func=device_rsu_sdm)

    fpgadefault = subparser.add_parser('fpgadefault',
                                       help='Set default FPGA image')
    fpgadefault.add_argument('bdf', nargs='?',
                             help=('PCIe address '
                                   '(eg 04:00.0 or 0000:04:00.0)'))
    fpgadefault.add_argument('-p', '--page',
                             choices=['user1', 'user2', 'factory'],
                             default=None, help='select primary FPGA page')
    fpgadefault.add_argument('-f', '--fallback',
                             nargs='?', help='select secondary FPGA page(s) '
                                             'as comma-separated list')
    fpgadefault.set_defaults(func=set_fpga_default)

    return parser.parse_args()


def normalize_bdf(bdf):
    pat = r'[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$'
    if re.match(pat, bdf):
        return bdf

    if re.match(r'[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$', bdf):
        return "0000:{}".format(bdf)
    logger.warn('invalid bdf: {}'.format(bdf))
    raise SystemExit(os.EX_USAGE)


def main():
    args = parse_args()
    level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(level=level,
                        format='%(asctime)s - %(message)s')
    compatible = fpga.enum([{'supports_rsu': True}])
    if not compatible:
        sys.stderr.write('No compatible devices found\n')
        raise SystemExit(os.EX_USAGE)

    if not hasattr(args, 'bdf'):
        if len(compatible) == 1:
            args.bdf = compatible[0].pci_node.pci_address
        elif len(compatible) > 1:
            prog = os.path.basename(sys.argv[0])
            sys.stderr.write(('Please specify PCIe address as '
                              '[<segment>:]<bus>:<device>.<function>\n'))
            sys.stderr.write('Acceptable commands:\n')
            for dev in compatible:
                sys.stderr.write('>{} {} {}\n'.format(prog,
                                                      args.which,
                                                      dev.pci_node.bdf))
            raise SystemExit(os.EX_USAGE)

    bdf = normalize_bdf(args.bdf)

    Path(RSU_LOCK_DIR).mkdir(parents=True, exist_ok=True)

    for device in compatible:
        if device.pci_node.pci_address.lower() == bdf.lower():
            exit_code = os.EX_IOERR
            with open(RSU_LOCK_FILE, 'w') as flock:
                fcntl.flock(flock.fileno(), fcntl.LOCK_EX)
                try:
                    args.func(device, args)
                except IOError:
                    logging.error('RSU operation failed')
                else:
                    exit_code = os.EX_OK
                    logging.info('RSU operation complete')
                finally:
                    fcntl.flock(flock.fileno(), fcntl.LOCK_UN)
            raise SystemExit(exit_code)

    logging.error('PCIe address (%s) does not identify a compatible device',
                  args.bdf)
    raise SystemExit(os.EX_UNAVAILABLE)

if __name__ == "__main__":
    main()
