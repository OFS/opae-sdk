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
import subprocess
import sys
import time
from opae.admin.dfl import dfl

OPAEUIO_VERSION = '1.0.0'

def parse_args():
    """Parse script arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('device', nargs='?',
                        help='DFL device name')
    parser.add_argument('-i', '--init', default=False, action='store_true',
                        help='initialize the given device for uio')
    parser.add_argument('-r', '--release', default=False, action='store_true',
                        help='release the given device from uio')
    parser.add_argument('-d', '--driver', default='dfl-uio-pdev',
                        help='driver to bind on init')
    parser.add_argument('-u', '--user', default='root',
                        help='userid to assign during init')
    parser.add_argument('-g', '--group', default='root',
                        help='groupid to assign during init')
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s {}'.format(OPAEUIO_VERSION),
                        help='display version information and exit')
    return parser, parser.parse_args()


def load_driver(driver):
    """Load a Linux kernel module via modprobe.

    driver - name of the driver to load.
    """
    return subprocess.call(['modprobe', driver])


def initialize_uio(device, pac, new_owner, driver):
    """Bind a DFL device and prepare it for use with uio.

    device - the DFL device name.
    pac - the dfl object (from dfl.enum()) corresponding to device.
    new_owner - user:group for the owner of the resulting uio device.
    driver - the new uio driver to bind.

    Unbind the given DFL device from its current driver, re-assigning it
    to the driver named by the last parameter. Discover the newly-created
    uio device, and set the device permissions to rw-rw---- for new_owner,
    given that new_owner != 'root:root'.
    """
    if pac.driver_override.value == driver:
        print('{} is already bound to {}.'.format(device, driver))
        return

    print('Unbinding {} from {}.'.format(device, pac.driver.name))
    pac.unbind()

    print('Setting driver override to {}.'.format(driver))
    pac.driver_override.value = driver

    print('Loading {}.'.format(driver))
    load_driver(driver)

    print('Probing {}..'.format(device))
    with open('/sys/bus/dfl/drivers_probe', 'w') as fp:
        fp.write(device)

    time.sleep(0.25)

    uio_device = pac.find_one(os.path.join('uio_pdrv_genirq.*.auto',
                                           'uio',
                                           'uio*'))
    if uio_device:
        uio_device = os.path.join('/dev', uio_device.name)
    else:
        print('Error discovering char device for {}'.format(device))
        sys.exit(1)
    print('UIO device is {}.'.format(uio_device))

    if new_owner != 'root:root':
        items = new_owner.split(':')
        user = pwd.getpwnam(items[0]).pw_uid
        group = -1
        if len(items) > 1:
            group = grp.getgrnam(items[1]).gr_gid
            print('Assigning {} to {}:{}'.format(uio_device, items[0], items[1]))
        else:
            print('Assigning {} to {}'.format(uio_device, items[0]))
        os.chown(uio_device, user, group)
        print('Changing permissions for {} to rw-rw----'.format(uio_device))
        os.chmod(uio_device, 0o660)

    print('To reverse what this command did, use: ' +
          'opaeuio -r {}'.format(device))


def release_uio(device, pac, driver):
    """Release and rebind a device bound to uio.

    device - the DFL device name.
    pac - the dfl object (from dfl.enum()) corresponding to device.
    driver - the name of the uio driver to which pac is bound before
             this call.

    If device is currently bound to driver, then unbind it by eliminating
    its driver override setting.
    """
    if pac.driver_override.value != driver:
        print('{} is not bound to {}.'.format(device, driver))
        return

    print('Unbinding {} from {}.'.format(device, pac.driver.name))
    pac.unbind()

    print('Clearing driver override.')
    pac.driver_override.value = '\n'

    print('Probing {}..'.format(device))
    with open('/sys/bus/dfl/drivers_probe', 'w') as fp:
        fp.write(device)

    time.sleep(0.25)


def main():
    """ Main script function."""
    parser, args = parse_args()

    if not args.device:
        print('Error: DFL device name is a required argument\n')
        parser.print_help(sys.stderr)
        sys.exit(1)

    pacs = dfl.enum([{'name': args.device}])
    if not pacs:
        print('Invalid DFL device: {}'.format(args.device))
        parser.print_help(sys.stderr)
        sys.exit(1)

    if args.init:
        initialize_uio(args.device,
                       pacs[0],
                       '{}:{}'.format(args.user, args.group),
                       args.driver)
    elif args.release:
        release_uio(args.device, pacs[0], args.driver)
    else:
        parser.print_help(sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
