#!/usr/bin/env python3
# Copyright(c) 2021-2023, Intel Corporation
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
import logging
import re
import sys
import subprocess
import time
from argparse import ArgumentParser, FileType
from opae.admin.sysfs import pcie_device, pci_node
from opae.admin.utils.process import call_process
from opae.admin.version import pretty_version


PCI_ADDRESS_PATTERN = (r'^(?P<pci_address>'
                       r'(?:(?P<segment>[\da-f]{4}):)?'
                       r'(?P<bdf>(?P<bus>[\da-f]{2}):'
                       r'(?P<device>[\da-f]{2})\.(?P<function>[0-7]{1})))$')
PCI_ADDRESS_RE = re.compile(PCI_ADDRESS_PATTERN, re.IGNORECASE)

PCI_VENDOR_DEVICE_PATTERN = r'^(?P<vendor>[\da-f]{4})?:(?P<device>[\da-f]{4})?$'
PCI_VENDOR_DEVICE_RE = re.compile(PCI_VENDOR_DEVICE_PATTERN, re.IGNORECASE)


def pci_devices(inp):
    m = PCI_ADDRESS_RE.match(inp)
    if m:
        d = m.groupdict()
        pci_address = '{}:{}'.format(d.get('segment') or '0000', d['bdf'])
        filt = {}
        filt['pci_node.pci_address'] = pci_address
        return [filt]
    m = PCI_VENDOR_DEVICE_RE.match(inp)
    if m:
        d = m.groupdict()
        vendor_str = d.get('vendor')
        device_str = d.get('device')
        if not vendor_str and not device_str:
            raise SystemExit('must specify vendor or device')
        filt = {}
        if vendor_str:
            filt['pci_node.vendor_id'] = int(vendor_str, 16)
        if device_str:
            filt['pci_node.device_id'] = int(device_str, 16)
        return [filt]

    raise ValueError('wrong pci address format: {}'.format(inp))


class pci_op(object):
    def __init__(self, op):
        if op not in dir(pci_node):
            raise KeyError(f'{op} is not a valid operation')
        self.op = op

    def __call__(self, pci_device, args):
        if not args.other_endpoints:
            self.do_op(pci_device, args)
        else:
            for p in pci_device.pci_node.root.endpoints:
                if p.pci_address != args.device:
                    self.do_op(p, args)

    def do_op(self, pci_device, args):
        if args.which == 'unbind':
            pci_device.pci_node.unbind()
        elif args.which == 'bind':
            pci_device.pci_node.bind(args.driver)
        elif args.which == 'remove':
            pci_device.pci_node.remove()




class pci_prop(object):
    def __init__(self, op, arg_type=str):
        if op not in dir(pci_node):
            raise KeyError(f'{op} is not a valid operation')
        self.op = op
        self.arg_type = arg_type

    def __call__(self, pci_device, args, value):
        if not args.other_endpoints:
            try:
                setattr(pci_device.pci_node, self.op, self.arg_type(value))
            except ValueError:
                print(f'Could not set {self.op} to {value} for {pci_device}')
        else:
            for p in pci_device.pci_node.root.endpoints:
                if p.pci_address != args.device:
                    try:
                        setattr(p, self.op, self.arg_type(value))
                    except ValueError:
                        print(f'Could not set {self.op} to {value} for {p}')


def rescan(pci_device, args):
    pci_device.pci_node.pci_bus.node('rescan').value = 1


class aer(object):
    @staticmethod
    def add_subparser(subparser):
        aer = subparser.add_parser('aer')

        action = aer.add_subparsers(dest='aer_which')
        action.add_parser('dump')
        action.add_parser('clear')

        mask = action.add_parser('mask')
        mask.add_argument('values',
                          nargs='*', default=[],
                          help='show, print, all, off, <int> <int>')
        mask.add_argument('-o', '--output', type=FileType('w'),
                          default=sys.stdout,
                          help='file to store values for show/print')
        mask.add_argument('-i', '--input', type=FileType('r'),
                          help='file to read values from')

    def __call__(self, device, args):
        if args.aer_which == 'dump':
            self.dump(device)
        elif args.aer_which == 'clear':
            self.clear(device)
        elif args.aer_which == 'mask':
            self.mask(device, args)

    def clear(self, device):
        """
        clear aer errors and print error status.
        """
        try:
            cmd = f'setpci -s {device} ECAP_AER+0x10.L'
            call_process(f'{cmd}=FFFFFFFF')
            output = call_process(cmd)
            print("aer clear errors:", output)
        except (subprocess.CalledProcessError, OSError):
            print("Failed to clear aer errors")

    def dump(self, device):
        for key in ['aer_dev_correctable',
                    'aer_dev_fatal', 'aer_dev_nonfatal']:
            if device.pci_node.have_node(key):
                print('-' * 64)
                label = f'{key.lstrip("aer_dev").upper()} ({device})'
                print(f'|{label:^62}|')
                print('-' * 64)
                for line in device.node(key).value.split('\n'):
                    k, v = line.split()
                    print(f'|{k:>30}: {v:30}|')
                print('-' * 64)
                print()

    def mask(self, device, args):
        inp = args.input
        if inp:
            values = inp.readline().split()
        else:
            values = args.values

        if not values or values[0] in ['show', 'print']:
            v0, v1 = device.pci_node.aer
            args.output.write(f'0x{v0:08x} 0x{v1:08x}\n')
        elif values[0] == 'all':
            device.pci_node.aer = (0xFFFFFFFF, 0xFFFFFFFF)
        elif values[0] == 'off':
            device.pci_node.aer = (0, 0)
        elif len(values) == 2:
            device.pci_node.aer = [int(v, 0) for v in values]
        else:
            raise SystemExit('incorrect values for setting AER')


def topology(pci_device, args):
    for line in pci_device.pci_node.root.tree().split('\n'):
        if pci_device.pci_node.pci_address in line:
            line = '\033[92m{}\033[00m' .format(line)
        elif '(pcieport)' not in line:
            line = '\033[96m{}\033[00m' .format(line)
        print(line)


class unplug(object):
    @staticmethod
    def add_subparser(subparser):
        unplug = subparser.add_parser('unplug')
        unplug.add_argument('-d', '--debug', action='store_true',
                            default=False, help='enable debug output')
        unplug.add_argument('-e', '--exclude', default=None, type=pci_devices,
                            help='do not unplug the device specified here')

    def __call__(self, device, args):
        debug = args.debug

        exclude_node = None
        if args.exclude:
            exclude_devs = pcie_device.enum(args.exclude)
            if exclude_devs:
                exclude_node = exclude_devs[0].pci_node

        root = device.pci_node.root

        v0, v1 = None, None
        if root.supports_ecap('aer'):
            if debug:
                print('ECAP_AER is supported')
            v0, v1 = root.aer
            root.aer = (0xFFFFFFFF, 0xFFFFFFFF)

        unplug.unplug_node(root, debug, exclude_node)

        if v0:
            root.aer = (v0, v1)

    @staticmethod
    def unplug_node(root, debug, exclude_node):
        if debug:
            print('Unbinding drivers for leaf devices', end='')
            if exclude_node:
                print(f' except {exclude_node.pci_address}')
            else:
                print('')

        for e in root.endpoints:
            unbind = True

            if exclude_node and e == exclude_node:
                unbind = False

            if unbind:
                if debug:
                    print(f' unbind {e.pci_address}')
                e.unbind()

        remove = True
        if exclude_node and exclude_node.root == root:
            remove = False

        if remove:
            if debug:
                print(f'Removing the root device {root.pci_address}')
            root.remove()


class plug(object):
    @staticmethod
    def add_subparser(subparser):
        plug = subparser.add_parser('plug')
        plug.add_argument('-d', '--debug', action='store_true',
                          default=False, help='enable debug output')

    def __call__(self, device, args):
        debug = args.debug
        root = device.pci_node.root

        self.clear_device_status(root, debug)
        self.clear_uncorrectable_errors(root, debug)
        self.clear_correctable_errors(root, debug)

        for e in root.endpoints:
            self.clear_device_status(e, debug)
            self.clear_uncorrectable_errors(e, debug)
            self.clear_correctable_errors(e, debug)

    def clear_device_status(self, device, debug):
        if debug:
            print(f'Clearing device status for {device.pci_address}')
        cmd = f'setpci -s {device.pci_address} CAP_EXP+0x08.L'
        try:
            output = int(call_process(cmd), 16)
        except ValueError: # Error during conversion. setpci() call failed.
            return
        output &= ~0xFF000
        output |= 0xF5000
        call_process(f'{cmd}={output:08x}')

    def clear_uncorrectable_errors(self, device, debug):
        if debug:
            print(f'Clearing uncorrectable errors for {device.pci_address}')
        cmd = f'setpci -s {device.pci_address} ECAP_AER+0x04.L'
        call_process(f'{cmd}=FFFFFFFF')

    def clear_correctable_errors(self, device, debug):
        if debug:
            print(f'Clearing correctable errors for {device.pci_address}')
        cmd = f'setpci -s {device.pci_address} ECAP_AER+0x10.L'
        call_process(f'{cmd}=FFFFFFFF')


class reset(object):
    @staticmethod
    def add_subparser(subparser):
        reset = subparser.add_parser('reset')
        reset.add_argument('-d', '--debug', action='store_true',
                           default=False, help='enable debug output')

    def __call__(self, device, args):
        debug = args.debug

        root = device.pci_node.root

        v0, v1 = None, None
        if root.supports_ecap('aer'):
            if debug:
                print('ECAP_AER is supported')
            v0, v1 = root.aer
            root.aer = (0xFFFFFFFF, 0xFFFFFFFF)

        reset.reset_node(root, debug)

        if v0:
            root.aer = (v0, v1)

    @staticmethod
    def reset_node(root, debug):
        if debug:
            print('Unbinding drivers for leaf devices')

        for e in root.endpoints:
            if debug:
                print(f' unbind {e.pci_address}')
            e.unbind()

        for e in root.endpoints:
            if debug:
                print(f' remove {e.pci_address}')
            try:
                e.remove()
            except NameError:
                None # Ignore endpoints with no sysfs remove entry

        if debug:
            print(f'Resetting secondary devices under {root.pci_address}')
        root.reset_bridge()
        root.rescan()


def add_unbind_subparser(subparser):
    subparser.add_parser('unbind')


def add_bind_subparser(subparser):
    bind = subparser.add_parser('bind')
    bind.add_argument('driver', help='driver to bind to the device')


def add_rescan_subparser(subparser):
    subparser.add_parser('rescan')


def add_remove_subparser(subparser):
    subparser.add_parser('remove')


def add_vf_subparser(subparser):
    vf = subparser.add_parser('vf')
    vf.add_argument('numvfs', type=int,
                    help='number of vfs to set')


def add_topology_subparser(subparser):
    subparser.add_parser('topology')


def main():
    actions = {'unbind': (add_unbind_subparser, pci_op('unbind')),
               'bind': (add_bind_subparser, pci_op('bind')),
               'rescan': (add_rescan_subparser, rescan),
               'remove': (add_remove_subparser, pci_op('remove')),
               'vf': (add_vf_subparser, pci_prop('sriov_numvfs', int)),
               'aer': (aer.add_subparser, aer()),
               'topology': (add_topology_subparser, topology),
               'unplug': (unplug.add_subparser, unplug()),
               'plug': (plug.add_subparser, plug()),
               'reset': (reset.add_subparser, reset())}

    parser = ArgumentParser()

    log_levels = ['verbose', 'info']
    parser.add_argument('--log-level', choices=log_levels,
                        default='info', help='log level to use')

    parser.add_argument('devices', type=pci_devices,
                        metavar='device-filter',
                        help=('PCIe filter of device '
                              '([segment:]bus:device.function)'
                              ' or [vendor]:[device]'))
    parser.add_argument('-E', '--other-endpoints', action='store_true',
                        default=False,
                        help='perform action on peer PCIe devices')
    parser.add_argument('-v', '--version', action='version',
                        version=f"%(prog)s {pretty_version()}",
                        help='display version information and exit')
    subparser = parser.add_subparsers(dest='which')

    for k in actions.keys():
        actions[k][0](subparser)

    args = parser.parse_args()

    if args.log_level == 'info':
        logging.basicConfig(level=logging.INFO, format='%(message)s')
    else:
        logging.basicConfig(level=logging.DEBUG, format='%(message)s')

    if hasattr(args, 'which') and args.which and args.which == 'plug':
        # Force a PCI bus rescan.
        with open('/sys/bus/pci/rescan', 'w') as fd:
            fd.write('1')
        time.sleep(1)

    devices = pcie_device.enum(args.devices)
    if not devices:
        raise SystemExit(f'{sys.argv[1]} not found')

    for dev in devices:
        if hasattr(args, 'which') and args.which:
            if args.which == 'vf':
                actions['vf'][1](dev, args, args.numvfs)
            else:
                actions[args.which][1](dev, args)
                # Clear AER after reset
                if args.which == 'reset':
                    actions['plug'][1](dev, args)
        else:
            actions['topology'][1](dev, args)


if __name__ == '__main__':
    main()
