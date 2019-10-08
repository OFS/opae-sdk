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
import logging
import os
import sys

from opae.admin.fpga import fpga
from opae.admin.sysfs import pci_node


LOG = logging.getLogger('fpgaport')


def fpga_filter(inp):
    filt = {'pci_node.supports_sriov': True}
    if inp == 'all':
        return filt
    if inp.startswith('/dev') and os.path.exists(inp):
        filt.update({'fme.devpath', inp})
    else:
        pci_info = pci_node.parse_address(inp)
        if pci_info:
            pci_address = pci_info['pci_address']
            filt.update({'pci_node.pci_address': pci_address})
        else:
            raise TypeError('{} is not valid device pattern')
    return filt


def set_numvfs(device, value):
    try:
        device.pci_node.sriov_numvfs = value
    except ValueError as err:
        LOG.warn('error setting num_sriov: "%s"', err)


def assign(args, device):
    destroy_vfs = args.numvfs == 0 or args.destroy_vfs
    if not destroy_vfs and device.pci_node.sriov_numvfs:
        LOG.warn('Device (%s) has VFs, skipping...', device.pci_node)
        return
    numvfs = device.pci_node.sriov_numvfs
    set_numvfs(device, 0)
    try:
        device.fme.assign_port(args.port)
    except (OSError, IOError) as err:
        LOG.warn('assiging port %d for device %s: %s', args.port,
                 device.pci_node, err)
        set_numvfs(device, numvfs)


def release(args, device):
    if args.destroy_vfs:
        LOG.warn('--destroy-vfs only applicable with "assign"')
    try:
        device.fme.release_port(args.port)
    except (OSError, IOError) as err:
        LOG.warn('releasing port %d for device %s: %s', args.port,
                 device.pci_node, err)
    else:
        if args.numvfs:
            set_numvfs(device, args.numvfs)


def main():
    actions = {'assign': assign,
               'release': release}
    # parse command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('action', action='store',
                        choices=sorted(actions.keys()),
                        help='Action to perform - {}'.format(actions.keys()))
    parser.add_argument('device',
                        help=('The FPGA (FME) device.'
                              'Can be identified by dev path (/dev/*fme.0) '
                              'or by PCIe address: '
                              '([<segment>:]<bus>:<device>.<function>'))
    parser.add_argument('port', type=int, default=0, nargs='?',
                        help='The port number.')
    parser.add_argument('-N', '--numvfs', type=int,
                        help='Create VFs from the argument given.')
    parser.add_argument('-X', '--destroy-vfs', action='store_true',
                        default=False,
                        help='Destroy all VFs prior to assigning.')
    parser.add_argument('--debug', action='store_true', default=False,
                        help='Set logging to DEBUG if verbose selected')

    args = parser.parse_args()

    level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(level=level,
                        format=('[%(asctime)-15s] [%(levelname)-8s] '
                                '%(message)s'))
    devices = fpga.enum([fpga_filter(args.device)])

    if not devices:
        LOG.error('Could not find device using pattern: "%s"', args.device)
        sys.exit(os.EX_USAGE)

    for device in devices:
        actions[args.action](args, device)


if __name__ == "__main__":
    main()
