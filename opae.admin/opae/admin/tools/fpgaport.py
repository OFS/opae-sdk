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
import os
import sys
import struct

from opae.admin.fpga import fpga
from opae.admin.sysfs import pci_node


FPGA_FME_PORT_ASSIGN = 0xB582
FPGA_FME_PORT_RELEASE = 0xB581

LOG = logging.getLogger('fpgaport')


def fpga_filter(inp):
    if inp == 'all':
        return {}
    if inp.startswith('/dev'):
        return {'sysfspath':
                inp.replace('/dev', '/sys/class/fpga')}
    pci_info = pci_node.parse_address(inp)
    if pci_info:
        pci_address = pci_info['pci_address']
        return {'pci_node.pci_address': pci_address}
    raise TypeError('{} is not valid device pattern')


def call_ioctl(fme, req, data):
    try:
        with fme as fmedev:
            try:
                fcntl.ioctl(fmedev, req, data)
            except IOError as err:
                LOG.warn('ioctl() failed: "%s"', err)
                sys.exit(os.EX_IOERR)
    except OSError as err:
        LOG.error('open() failed: "%s"', err)
        sys.exit(os.EX_NOPERM)


def set_numvfs(device, value):
    try:
        device.pci_node.sriov_numvfs = value
    except ValueError as err:
        LOG.warn('error setting num_sriov: "%s"', err)


def main():
    # parse command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('action', action='store',
                        choices=['assign', 'release'],
                        help='action to perform - ("assign", "release")')
    parser.add_argument('device', type=fpga_filter,
                        help='the FPGA (FME) device')
    parser.add_argument('port', type=int, default=0, nargs='?',
                        help='The port number')
    parser.add_argument('--numvfs', type=int,
                        help='Create VFs from the argument given.')
    parser.add_argument('--debug', action='store_true', default=False,
                        help='Set logging to DEBUG if verbose selected')

    args = parser.parse_args()

    level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(level=level,
                        format=('[%(asctime)-15s] [%(levelname)-8s]'
                                '%(message)s'))

    requests = {'assign': FPGA_FME_PORT_ASSIGN,
                'release': FPGA_FME_PORT_RELEASE}

    devices = fpga.enum([args.device])

    if not devices:
        LOG.error('Could not find device using pattern')
        sys.exit(os.EX_USAGE)

    try:
        req = requests[args.action]
    except IndexError:
        LOG.error('Invalid action: %s', args.action)
        sys.exit(os.EX_USAGE)

    ioctl_data = struct.pack('III', 12, 0, args.port)

    for device in devices:
        items = list(device.find('*-fpga-fme.*'))
        if not items:
            LOG.debug('Device has no FME: "%s"', device.pci_node.sysfspath)
            continue

        fme = device.fme

        if args.action == 'assign':
            if args.numvfs != 0 and device.pci_node.sriov_numvfs:
                LOG.warn('Device has VFs, skipping...')
                continue
            set_numvfs(device, args.numvfs)

        call_ioctl(fme, req, ioctl_data)

        if args.action == 'release' and args.numvfs:
            set_numvfs(device, args.numvfs)


if __name__ == "__main__":
    main()
