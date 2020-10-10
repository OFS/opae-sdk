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

""" Bind/Unbind a PCIe device to/from vfio-pci. """

import argparse
import grp
import os
import pwd
import re
import subprocess
import sys
import time

OPAEVFIO_VERSION = '1.0.0'

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
    parser.add_argument('addr', nargs='?',
                        help='PCI Address of the device')
    parser.add_argument('-i', '--init', default=False, action='store_true',
                        help='initialize the given device for vfio')
    parser.add_argument('-r', '--release', default=False, action='store_true',
                        help='release the given device from vfio')
    parser.add_argument('-d', '--driver', default='dfl-pci',
                        help='driver to re-bind on release')
    parser.add_argument('-u', '--user', default='root',
                        help='userid to assign during init')
    parser.add_argument('-g', '--group', default='root',
                        help='groupid to assign during init')
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s {}'.format(OPAEVFIO_VERSION),
                        help='display version information and exit')
    return parser, parser.parse_args()


def vid_did_for_address(addr):
    """Retrieve the PCIe Vendor/Device ID for a given address.

    addr - canonical PCIe address.

    Returns a tuple of the (vid, did).
    """
    path = os.path.join('/sys/bus/pci/devices', addr)
    vid, did = '', ''
    with open(os.path.join(path, 'vendor'), 'r') as inf:
        vid = inf.read().rstrip('\n')
    with open(os.path.join(path, 'device'), 'r') as inf:
        did = inf.read().rstrip('\n')
    return (vid, did)


def get_bound_driver(addr):
    """Retrieve the name of the driver (if any) bound to a PCIe address.

    addr - canonical PCIe address.

    Returns the driver name or None if addr is not bound.
    """
    link = '/sys/bus/pci/devices/{}/driver'.format(addr)
    if os.path.islink(link):
        driver = os.readlink(link).split(os.sep)[-1]
        return driver
    return None


def unbind_driver(driver, addr):
    """Unbind a PCIe device from its bound driver.

    driver - name of bound driver.
    addr - canonical PCIe address.
    """
    unbind = '/sys/bus/pci/drivers/{}/unbind'.format(driver)
    if os.path.exists(unbind):
        with open(unbind, 'w') as outf:
            outf.write(addr)


def bind_driver(driver, addr):
    """Bind a PCIe device to a driver.

    driver - name of driver to bind.
    addr - canonical PCIe address.
    """
    bind = '/sys/bus/pci/drivers/{}/bind'.format(driver)
    if os.path.exists(bind):
        with open(bind, 'w') as outf:
            outf.write(addr)


def load_driver(driver):
    """Load a Linux kernel module via modprobe.

    driver - name of the driver to load.
    """
    return subprocess.call(['modprobe', driver])


def initialize_vfio(addr, new_owner):
    """Bind a PCIe device and prepare it for use with vfio-pci.

    addr - canonical PCIe address.
    new_owner - user:group for the owner of the resulting vfio device.

    Unbind the given PCIe device from its current driver, re-assigning it
    to vfio-pci. Discover the newly-created iommu group, and set he device
    permissions to rw-rw---- for new_owner, given that new_owner !=
    'root:root'.
    """
    vid_did = vid_did_for_address(addr)
    driver = get_bound_driver(addr)

    msg = '(0x{:04x},0x{:04x}) at {}'.format(
        int(vid_did[0], 16), int(vid_did[1], 16), addr)

    if driver and driver == 'vfio-pci':
        print('{} is already bound to vfio-pci'.format(msg))
        return

    if driver:
        print('Unbinding {} from {}'.format(msg, driver))
        unbind_driver(driver, addr)

    load_driver('vfio-pci')

    print('Binding {} to vfio-pci'.format(msg))
    new_id = '/sys/bus/pci/drivers/vfio-pci/new_id'
    with open(new_id, 'w') as outf:
        outf.write('{} {}'.format(vid_did[0], vid_did[1]))

    time.sleep(0.25)

    iommu_group = os.path.join('/sys/bus/pci/devices', addr, 'iommu_group')
    group_num = os.readlink(iommu_group).split(os.sep)[-1]

    print('iommu group for {} is {}'.format(msg, group_num))

    if new_owner != 'root:root':
        device = os.path.join('/dev/vfio', group_num)
        items = new_owner.split(':')
        user = pwd.getpwnam(items[0]).pw_uid
        group = -1
        if len(items) > 1:
            group = grp.getgrnam(items[1]).gr_gid
            print('Assigning {} to {}:{}'.format(device, items[0], items[1]))
        else:
            print('Assigning {} to {}'.format(device, items[0]))
        os.chown(device, user, group)
        print('Changing permissions for {} to rw-rw----'.format(device))
        os.chmod(device, 0o660)

    print('To reverse what this command did, use: ' +
          'opaevfio -r {} -d {}'.format(addr, driver))


def release_vfio(addr, new_driver):
    """Release and rebind a device bound to vfio-pci.

    addr - canonical PCIe address.
    new_driver - name of the new driver to bind the device.

    If addr is currently bound to vfio-pci, then unbind it
    and rebind it to new_driver.
    """
    vid_did = vid_did_for_address(addr)
    driver = get_bound_driver(addr)

    msg = '(0x{:04x},0x{:04x}) at {}'.format(
        int(vid_did[0], 16), int(vid_did[1], 16), addr)

    if not driver or driver != 'vfio-pci':
        print('{} is not bound to vfio-pci'.format(msg))
        return

    print('Releasing {} from vfio-pci'.format(msg))
    unbind_driver(driver, addr)

    print('Rebinding {} to {}'.format(msg, new_driver))
    bind_driver(new_driver, addr)


def main():
    """ Main script function."""
    parser, args = parse_args()

    if not args.addr:
        print('Error: addr is a required argument\n')
        parser.print_help(sys.stderr)
        sys.exit(1)

    addr = normalized_pci_addr(args.addr)
    if not addr:
        print('Invalid PCIe address: {}'.format(args.addr))
        parser.print_help(sys.stderr)
        sys.exit(1)

    if args.init:
        initialize_vfio(addr, '{}:{}'.format(args.user, args.group))
    elif args.release:
        release_vfio(addr, args.driver)
    else:
        parser.print_help(sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
