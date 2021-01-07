#! /usr/bin/env python
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
import re
from argparse import ArgumentParser
from opae.admin.sysfs import pcie_device, pci_node


PCI_ADDRESS_PATTERN = (r'^(?P<pci_address>'
                       r'(?:(?P<segment>[\da-f]{4}):)?'
                       r'(?P<bdf>(?P<bus>[\da-f]{2}):'
                       r'(?P<device>[\da-f]{2})\.(?P<function>[0-7]{1})))$')
PCI_ADDRESS_RE = re.compile(PCI_ADDRESS_PATTERN, re.IGNORECASE)


def pci_address(inp):
    m = PCI_ADDRESS_RE.match(inp)
    if not m:
        raise ValueError('wrong pci address format: {}'.format(inp))

    d = m.groupdict()
    return '{}:{}'.format(d.get('segment') or '0000', d['bdf'])


class pci_op(object):
    def __init__(self, op):
        if op not in dir(pci_node):
            raise KeyError(f'{op} is not a valid operation')
        self.op = op

    def __call__(self, pci_device, args):
        if not args.other_endpoints:
            getattr(pci_device.pci_node, self.op)()
        else:
            for p in pci_device.pci_node.root.endpoints:
                if p.pci_address != args.device:
                    getattr(p, self.op)()


def rescan(pci_device, args):
    pci_device.pci_node.pci_bus.node('rescan').value = 1


def topology(pci_device, args):
    for line in pci_device.pci_node.root.tree().split('\n'):
        if pci_device.pci_node.pci_address in line:
            line = '\033[92m{}\033[00m' .format(line)
        elif '(pcieport)' not in line:
            line = '\033[96m{}\033[00m' .format(line)
        print(line)


def main():
    actions = {'unbind': pci_op('unbind'),
               'rescan': rescan,
               'remove': pci_op('remove'),
               'topology': topology}

    parser = ArgumentParser()
    parser.add_argument('device', type=pci_address,
                        help=('pcie address of device '
                              '([segment:]bus:device.function)'))
    parser.add_argument('action', choices=sorted(actions.keys()),
                        help='action to perform on device')
    parser.add_argument('-E', '--other-endpoints', action='store_true',
                        default=False,
                        help='perform action on peer pcie devices')
    args = parser.parse_args()

    pci_devices = pcie_device.enum([{'pci_node.pci_address': args.device}])
    if not pci_devices:
        raise SystemExit(f'{args.device} not found')

    if len(pci_devices) > 1:
        raise SystemExit(f'more than one device found for: {args.device}')

    pci_device = pci_devices[0]

    actions[args.action](pci_device, args)


if __name__ == '__main__':
    main()
