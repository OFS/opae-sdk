#!/usr/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
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

"""

"""
import argparse
import grp
import logging
import os
import pwd
import re
import subprocess
import time
import sys

if sys.version_info[0] == 2:
    from pathlib2 import Path
else:
    from pathlib import Path

PCI_ADDRESS_PATTERN = (r'(?P<pci_address>'
                       r'(?:(?P<segment>[\da-f]{4}):)?'
                       r'(?P<bdf>(?P<bus>[\da-f]{2}):'
                       r'(?P<device>[\da-f]{2})\.(?P<function>\d)))')
PCI_ADDRESS_RE = re.compile(PCI_ADDRESS_PATTERN, re.IGNORECASE)
PCI_BUS = Path('/sys/bus/pci')


class linux_path(object):
    def __init__(self, path):
        self._path = Path(path)

    def path(self, *components):
        return self._path.joinpath(*components)

    def read_text(self, *args, **kwargs):
        return self._path.read_text(*args, **kwargs)

    def write_text(self, *args, **kwargs):
        return self._path.write_text(*args, **kwargs)

    @property
    def name(self):
        return self._path.name


class linux_device(linux_path):
    def __init__(self, path):
        super(linux_device, self).__init__(path)

    @property
    def driver(self):
        driver = self.path('driver')
        if driver.exists():
            return linux_driver.from_name(driver.resolve(True).name)


class pci_device(linux_device):
    def __init__(self, path, **kwargs):
        super(pci_device, self).__init__(path)
        self._pci_address = kwargs.get('pci_address')
        self._bus = kwargs.get('bus')
        self._device = kwargs.get('device')
        self._function = kwargs.get('function')
        self._vendor_id = None
        self._device_id = None

    def __str__(self):
        driver_name = self.driver.name if self.driver else 'No Driver'
        return '({:04x}, {:04x}) : [{}] ({})'.format(
            self.vendor_id, self.device_id, self.pci_address, driver_name)

    def __repr__(self):
        return str(self)


    @property
    def vendor_id(self):
        if self._vendor_id is None:
            self._vendor_id = hexint(self.path('vendor').read_text())
        return self._vendor_id

    @property
    def device_id(self):
        if self._device_id is None:
            self._device_id = hexint(self.path('device').read_text())
        return self._device_id

    @property
    def iommu_group(self):
        group = self.path('iommu_group').resolve(True)
        if group.exists():
            return group

    @property
    def vfio_device(self):
        group = self.iommu_group.name
        if group and Path('/dev/vfio', group).exists():
            return str(Path('/dev/vfio', group))

    @property
    def pci_address(self):
        return self._pci_address

    @classmethod
    def from_address(cls, addr):
        m = PCI_ADDRESS_RE.match(addr)
        if not m:
            raise NameError('{} not a valid pci address format'.format(addr))
        info = m.groupdict()
        if info.get('segment') is None:
            info['segment'] = '0000'
            info['pci_address'] = '{segment}:{pci_address}'.format(**info)

        p = PCI_BUS.joinpath('devices', info['pci_address'])
        if not p.exists():
            raise OSError('{} not a pci device'.format(addr))
        return pci_device(p.resolve(True), **info)


class linux_driver(linux_path):
    def __init__(self, path):
        super(linux_driver, self).__init__(path)

    @classmethod
    def from_name(cls, name):
        p = PCI_BUS.joinpath('drivers', name)
        if not p.exists():
            raise OSError('{name} is not a driver or not loaded'.format(name=name))
        return linux_driver(p.resolve(True))

    def new_id(self, vendor_id, device_id):
        self._path.joinpath('new_id').write_text('{vendor_id} {device_id}'.format(**locals()))

    def bind(self, pci_address):
        m = PCI_ADDRESS_RE.match(pci_address)
        if m:
            self._path.joinpath('bind').write_text(pci_address)

    def unbind(self, pci_address):
        m = PCI_ADDRESS_RE.match(pci_address)
        if m:
            self._path.joinpath('unbind').write_text(pci_address)


def hexint(inp):
    return int(inp, 16) or int(inp, 10)


def vfio_own(device, user, group):
    if not device.iommu_group:
        SystemExit('device does not have iommu group')
    vfio_device = Path(device.vfio_device)
    time.sleep(0.1)
    if vfio_device.exists():
        os.chown(vfio_device, user, group)
        os.chmod(vfio_device, 0o660)
    else:
        logging.error('vfio device {vfio_device} not found'.format(**locals()))


def bind(args):
    device = args.device
    current_driver = args.device.driver
    new_driver = args.driver
    if current_driver and current_driver.name != new_driver.name:
        current_driver.unbind(device.pci_address)

    if not current_driver or current_driver.name != new_driver.name:
        new_driver.bind(device.pci_address)


def new_id(args):
    device = args.device
    current_driver = args.device.driver
    new_driver = args.driver
    if current_driver and current_driver.name != new_driver.name:
        current_driver.unbind(device.pci_address)

    if not current_driver or current_driver.name != new_driver.name:
        new_driver.new_id(device.vendor_id, device.device_id)


def unbind(args):
    device = args.device
    current_driver = args.device.driver
    if current_driver:
        current_driver.unbind(device.pci_address)


def find_devices(vendor=None, device=None):
    devices = []
    for d in PCI_BUS.joinpath('devices').glob('*'):
        did = hexint(d.joinpath('device').read_text())
        vid = hexint(d.joinpath('vendor').read_text())
        if device is not None and device != did:
            continue
        if vendor is not None and vendor != vid:
            continue
        dev = pci_device.from_address(d.name)
        if dev:
            devices.append(dev)

    return devices


def list_devices(args):
    devices = find_devices(vendor=args.vendor,
                           device=args.device)
    for d in devices:
        print(d)


def get_owner(inp):
    if inp:
        items = inp.split(':')
        uinfo = pwd.getpwnam(items.pop(0))
        if items:
            gid = grp.getgrnam(items[0]).gr_gid
        else:
            gid = uinfo.pw_gid
        return uinfo.pw_uid, gid


def vfio_op(args):
    if args.action == 'load-driver':
        logging.info('vfio-pci driver not loading, attempting to load...')
        subprocess.run(['modprobe', 'vfio-pci'])
        return

    vfio_driver = linux_driver.from_name('vfio-pci')

    if not vfio_driver:
        SystemExit('vfio-pci is not loaded, use load-driver  to load')

    if args.device is None:
        print('must use device pcie address')
        return

    if args.action == 'bind':
        vfio_driver.bind(args.device.pci_address)
    elif args.action == 'new_id':
        vfio_driver.new_id(args.device.vendor_id, args.device.device_id)
    elif args.action == 'unbind':
        vfio_driver.unbind(args.device.pci_address)

    if args.change_owner:
        if args.action not in ['bind', 'new_id']:
            print('change owner only allowed with "bind" or "new_id"')
        else:
            vfio_own(args.device, *args.change_owner)


def main():
    parser = argparse.ArgumentParser()

    sub = parser.add_subparsers()
    bind_parser = sub.add_parser('bind')
    bind_parser.add_argument('driver', type=linux_driver.from_name)
    bind_parser.add_argument('device', type=pci_device.from_address)
    bind_parser.set_defaults(func=bind)

    new_id_parser = sub.add_parser('new_id')
    new_id_parser.add_argument('driver', type=linux_driver.from_name)
    new_id_parser.add_argument('device', type=pci_device.from_address)
    new_id_parser.set_defaults(func=new_id)

    vfio_parser = sub.add_parser('vfio')
    vfio_parser.add_argument('device', type=pci_device.from_address, nargs='?')
    vfio_parser.add_argument('action', choices=['bind', 'new_id', 'unbind',
                                                'load-driver'])
    vfio_parser.add_argument('--change-owner', type=get_owner)
    vfio_parser.set_defaults(func=vfio_op)

    unbind_parser = sub.add_parser('unbind')
    unbind_parser.add_argument('device', type=pci_device.from_address)
    unbind_parser.set_defaults(func=unbind)

    list_parser = sub.add_parser('list')
    list_parser.add_argument('-v', '--vendor', type=hexint)
    list_parser.add_argument('-d', '--device', type=hexint)
    list_parser.set_defaults(func=list_devices)

    args = parser.parse_args()
    if not hasattr(args, 'func'):
        parser.print_usage()
    else:
        args.func(args)


if __name__ == '__main__':
    main()
