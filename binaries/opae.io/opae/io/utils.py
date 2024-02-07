# Copyright(c) 2020-2023, Intel Corporation
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

import contextlib
import errno
import grp
import json
import os
import pwd
import re
import struct
import subprocess
import sys
import uuid
import time

from enum import Enum
from ctypes import Union, LittleEndianStructure, c_uint64, c_uint32
from logging import StreamHandler
from opae.io.config import Config
from . import pci

if sys.version_info[0] == 3:
    from pathlib import Path
else:
    from pathlib2 import Path

import libvfio

JSON_FILE = '/var/lib/opae/opae.io.json'
ACCESS_MODE = 64


class pcicfg(Enum):
    command = 0x4


def hex_int(inp):
    return int(inp, 0)


def load_driver(driver, quiet=0):
    cmd = ['modprobe']
    if quiet:
        cmd += ['-q']
    cmd += [driver]
    return subprocess.call(cmd)


def get_bound_driver(pci_addr):
    link = '/sys/bus/pci/devices/{}/driver'.format(pci_addr)
    if os.path.islink(link):
        driver = os.readlink(link).split(os.sep)[-1]
        return driver


def unbind_driver(driver, pci_addr):
    unbind = '/sys/bus/pci/drivers/{}/unbind'.format(driver)
    if os.path.exists(unbind):
        with open(unbind, 'w') as outf:
            outf.write(pci_addr)


def bind_driver(driver, pci_addr):
    bind = '/sys/bus/pci/drivers/{}/bind'.format(driver)
    if os.path.exists(bind):
        try:
            with open(bind, 'w') as outf:
                outf.write(pci_addr)
        except OSError:
            return False
        return True
    return False


def get_dev_dict(file_name):
    dev_dict = {}
    if os.path.isfile(file_name):
        with open(file_name, 'r') as inf:
            try:
                dev_dict = json.load(inf)
            except json.JSONDecodeError as jde:
                LOG.warn('{} at {} line {} col {}'.format(
                         jde.msg, file_name, jde.lineno, jde.colno))
    return dev_dict


def put_dev_dict(file_name, dev_dict):
    d = os.path.dirname(file_name)
    if not os.path.isdir(d):
        os.makedirs(d)
    with open(file_name, 'w') as outf:
        json.dump(dev_dict, outf)


def chown_pci_sva(pci_addr, uid, gid):
    sva_bind_dev = os.path.join('/dev/dfl-pci-sva', pci_addr)
    if os.path.exists(sva_bind_dev):
        LOG.info('Setting owner of {}'.format(sva_bind_dev))
        os.chown(sva_bind_dev, uid, gid)


def enable_sriov(enable):
    sriov = '/sys/module/vfio_pci/parameters/enable_sriov'
    if not os.path.exists(sriov):
        return False

    LOG.info('Enabling SR-IOV for vfio-pci')
    try:
        with open(sriov, 'w') as outf:
            outf.write('Y' if enable else 'N')
    except OSError:
        return False
    return True


def vfio_init(pci_addr, new_owner='', force=False, **kwargs):
    vid_did = pci.vid_did_for_address(pci_addr)
    driver = get_bound_driver(pci_addr)
    init_sriov = kwargs.get('enable_sriov')

    msg = '(0x{:04x},0x{:04x}) at {}'.format(
        int(vid_did[0], 16), int(vid_did[1], 16), pci_addr)

    if not iommu_enabled():
        LOG.error("Binding to vfio-pci will fail because IOMMU is disabled.")
        raise SystemExit(os.EX_NOTFOUND)

    if driver and driver != 'vfio-pci':
        dev_dict = get_dev_dict(JSON_FILE)
        dev_dict[pci_addr] = driver
        try:
            put_dev_dict(JSON_FILE, dev_dict)
        except PermissionError:
            LOG.warn('Do not have sufficient permissions to save current state')
            if not force:
                return
        LOG.info('Unbinding {} from {}'.format(msg, driver))
        unbind_driver(driver, pci_addr)

    load_driver('dfl-pci-sva', quiet=1)
    load_driver('vfio-pci')

    print('Binding {} to vfio-pci'.format(msg))

    # On Linux kernel >= 3.16, use driver_override to specify the
    # driver that may bind to the function with the given address.
    # On older kernels, fall back to new_id which matches the vendor
    # and device ID to determine whether a driver may bind to a
    # device, which has the disadvantage that the driver may bind
    # to multiple functions with the same vendor and device ID.
    #
    # https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=782a985d7af26db39e86070d28f987cad21313c0
    driver_override = '/sys/bus/pci/devices/{}/driver_override'.format(pci_addr)
    if os.path.exists(driver_override):
        try:
            with open(driver_override, 'w') as outf:
                outf.write('vfio-pci')
        except OSError as exc:
            LOG.error('Cannot write driver_override to vfio-pci for: {}'.format(msg, exc))
            return
    else:
        new_id = '/sys/bus/pci/drivers/vfio-pci/new_id'
        try:
            with open(new_id, 'w') as outf:
                outf.write('{} {}'.format(vid_did[0], vid_did[1]))
        except OSError as exc:
            if exc.errno != errno.EEXIST:
                LOG.error(f'Cannot write new_id to vfio-pci for: {msg}')
                return

    time.sleep(0.50)

    try:
        bind_driver('vfio-pci', pci_addr)
    except OSError as exc:
        if exc.errno != errno.EBUSY:
            LOG.error(exc)
            return

    time.sleep(0.50)

    iommu_group = os.path.join('/sys/bus/pci/devices',
                               pci_addr,
                               'iommu_group')

    try:
        group_num = os.readlink(iommu_group).split(os.sep)[-1]
    except FileNotFoundError:
        LOG.info("No such directory: {}".format(iommu_group))
        return

    LOG.info('iommu group for {} is {}'.format(msg, group_num))

    if new_owner != 'root:root':
        device = os.path.join('/dev/vfio', group_num)
        items = new_owner.split(':')
        user = pwd.getpwnam(items[0]).pw_uid
        group = -1
        if len(items) > 1:
            group = grp.getgrnam(items[1]).gr_gid
            LOG.info('Assigning {} to {}:{}'.format(device, items[0], items[1]))
        else:
            LOG.info('Assigning {} to {}'.format(device, items[0]))
        os.chown(device, user, group)
        LOG.info('Changing permissions for {} to rw-rw----'.format(device))
        os.chmod(device, 0o660)
        chown_pci_sva(pci_addr, user, group)

    if init_sriov:
        enable_sriov(True)


def vfio_release(pci_addr):
    vid_did = pci.vid_did_for_address(pci_addr)
    driver = get_bound_driver(pci_addr)

    msg = '(0x{:04x},0x{:04x}) at {}'.format(
        int(vid_did[0], 16), int(vid_did[1], 16), pci_addr)

    if not driver or driver != 'vfio-pci':
        print('{} is not bound to vfio-pci'.format(msg))
        return

    print('Releasing {} from vfio-pci'.format(msg))

    # On Linux kernel >= 3.16, clear driver_override before
    # unbinding vfio-pci driver, which ensures subsequent
    # binding to another, previously bound driver succeeds.
    driver_override = '/sys/bus/pci/devices/{}/driver_override'.format(pci_addr)
    if os.path.exists(driver_override):
        try:
            with open(driver_override, 'w') as outf:
                outf.write('\n')
        except OSError as exc:
            LOG.error('Cannot clear driver_override for {}: {}'.format(msg, exc))
            return

    unbind_driver(driver, pci_addr)

    dev_dict = get_dev_dict(JSON_FILE)
    if dev_dict:
        driver = dev_dict.get(pci_addr)
        if driver:
            print('Rebinding {} to {}'.format(msg, driver))
            bind_driver(driver, pci_addr)
            del dev_dict[pci_addr]
            if dev_dict:
                put_dev_dict(JSON_FILE, dev_dict)
            else:
                os.remove(JSON_FILE)

    chown_pci_sva(pci_addr, 0, 0)


class opae_register(Union):
    _upper_mask = 0x00000000FFFFFFFF
    def __init__(self, region, offset, value=None):
        self.region = region
        self.offset = offset
        self.rd = getattr(self.region, f'read{self.width}')
        self.wr = getattr(self.region, f'write{self.width}')
        if value is None:
            self.update()
        else:
            self.value = value

    def __str__(self):
        return ', '.join([f'{name} = 0x{getattr(self.bits, name):x}'
                          for name, _, _ in self.bits._fields_])

    @classmethod
    def define(cls,
               name, bits=[('value', c_uint64, 64)], print_hex=True, width=64):
        bits_class = type('{name}_bits'.format(**locals()),
                          (LittleEndianStructure, ),
                          dict(_fields_=bits))
        if width not in [64, 32]:
            raise ValueError('width must be 64 or 32')
        int_t = c_uint64 if width == 64 else c_uint32
        return type(name,
                    (cls,),
                    dict(_fields_=[('bits', bits_class),
                                   ('value', int_t)],
                         width=width))

    def commit(self, value=None):
        if value is not None:
            self.value = value
        if ACCESS_MODE == 32 and self.width == 64:
            self.region.write32(self.offset, self._upper_mask & self.value)
            self.region.write32(self.offset + 4, self.value >> 32)
        else:
            self.wr(self.offset, self.value)

    def update(self):
        if self.region is None:
            raise OSError(os.EX_OSERR, 'no region open')
        if ACCESS_MODE == 32 and self.width == 64:
            lo = self.region.read32(self.offset)
            self.value = self.region.read32(self.offset+4) << 32
            self.value |= lo
        else:
            self.value = self.rd(self.offset)

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, tb):
        if exc_type and exc_value and tb:
            return False
        self.commit()


def register(name="value_register", bits=[('value', c_uint64, 64)], width=64):
    '''Create a register class dynamicaly using a list of bitfield tuples.

    Args:
        name: The name of the register class. Defaults to "value_register".
        bits: A list of three-element tuples describing the bitfields.
              The first element in the tuple is the name of the field.
              The second element in the tuple is the width of the register.
              Must be c_uint32 or c_uint64.
              The third element in the tuple is the size of the field in bits.
        width: The register width. Must be either 32 or 64. Default is 64


    Note:
        The register class is a little endian structure with bitfields starting
        at bit 0 going from top to bottom.
    '''
    r_class = opae_register.define(name, bits, width)
    return r_class


def read_guid(region, offset):
    lo = region.read64(offset)
    hi = region.read64(offset+0x08)
    return uuid.UUID(bytes=struct.pack('>QQ', hi, lo))


dfh0_bits = [
        ('id', c_uint64, 12),
        ('rev', c_uint64, 4),
        ('next', c_uint64, 24),
        ('eol', c_uint64, 1),
        ('reserved', c_uint64, 19),
        ('feature_type', c_uint64, 4)
]

dfh0 = register('dfh0', bits=dfh0_bits)

dfh1_bits = [
        ('reserved0', c_uint64, 12),
        ('rev', c_uint64, 4),
        ('next', c_uint64, 24),
        ('eol', c_uint64, 1),
        ('reserved', c_uint64, 7),
        ('rev_minor', c_uint64, 4),
        ('dfh_version', c_uint64, 8),
        ('feature_type', c_uint64, 4)
]

dfh1 = register('dfh1', bits=dfh1_bits)


def dfh_walk(region, offset=0, header=None, guid=None):
    while True:
        h = dfh1(region, offset)
        if header:
            h = header(region, offset, h.value)
        elif h.bits.dfh_version == 0:
            h = dfh0(region, offset, h.value)
        if guid is None or guid == read_guid(region, offset+0x8):
            yield offset, h
        if h.bits.eol or not h.bits.next:
            break
        offset += h.bits.next


def w_aligned(offset: int):
    mask = 0b11 if ACCESS_MODE == 32 else 0b111
    return (offset & mask) == 0


def walk(region,
         offset=0, show_uuid=False, count=None, delay=None, dump=False,
         safe_walk=True):
    for i, (offset_, hdr) in enumerate(dfh_walk(region, offset=offset)):
        if count and i >= count:
            break
        if safe_walk and not w_aligned(offset_):
            raise ValueError(
                f'offset 0x{offset_:04x} not {ACCESS_MODE}-bit aligned')
        print(f'offset: 0x{offset_:04x}, value: 0x{hdr.value:04x}')
        print(f'    dfh: {hdr}')
        if delay:
            time.sleep(delay)
        if show_uuid:
            print(f'    uuid: {read_guid(region, offset_+0x8)}')
            if delay:
                time.sleep(delay)
        if dump and not hdr.bits.eol:
            sz = hdr.bits.next if not hdr.bits.eol else len(region)-offset_
            for i in range(offset_+8, offset_+sz, 8):
                print(f'0x{i:04x}: 0x{region.read64(i):08x}')
                if delay:
                    time.sleep(delay)


def dump(region, start=0, output=sys.stdout, fmt='hex', count=None):
    stop_at = start + count*8 if count else len(region)
    offset = start
    if ACCESS_MODE == 64:
        rd = region.read64
        byte_length = 8
    else:
        rd = region.read32
        byte_length = 4
    while offset < stop_at:
        value = rd(offset)
        if fmt == 'hex':
            output.write(f'0x{offset:04x}: 0x{value:016x}\n')
        else:
            output.write(value.to_bytes(byte_length, 'little'))
        offset += byte_length


class feature(object):
    def __init__(self, region, offset=0, guid=None):
        self._region = region
        self._offset = offset
        self._guid = guid

    @property
    def offset(self):
        return self._offset

    @property
    def guid(self):
        return self._guid

    def __getitem__(self, offset):
        return read_csr(self._offset + offset)

    def __setitem__(self, offset, value):
        write_csr(self._offset + offset, value)

    @classmethod
    def find(cls, guid, offset=0):
        for o, h in dfh_walk(offset, guid=guid):
            yield cls(o, guid)

    @contextlib.contextmanager
    def register(self, offset, name='csr', bits=[('value', c_uint64, 64)]):
        try:
            err = True
            csr_class = opae_register.define(name, bits)
            csr = csr_class(self._region, self._offset + offset)
            yield csr
            err = False
        finally:
            if not err:
                csr.commit()


def read_attr(dirname, attr):
    fname = Path(dirname, attr)
    if fname.exists():
        return fname.read_text().strip()


def read_link(dirname, *attr):
    fname = Path(dirname, *attr)
    if fname.exists():
        return fname.resolve()


def lsfpga(**kwargs):
    _all = kwargs.pop('all', False)
    use_class = kwargs.pop('system_class', False)

    # Create a filter function that uses attributes in kwargs to match devices
    # as well "known" devices as those listed in the opae.io configuration data.
    def filter_fn(d: pci.device):
        for k,v in kwargs.items():
            try:
                attr_value = getattr(d, k)
            except FileNotFoundError:
                return False
            if attr_value != v:
                return False
        known = Config.opae_io_is_supported(*d.pci_id)
        if not _all and not known:
            return False
        return True

    def describe(d: pci.device):
        desc = Config.opae_io_platform_for(*d.pci_id)
        if desc:
            d._desc = desc
        return d

    devices = pci.ls(filter=filter_fn)
    if use_class:
        return devices
    return map(describe, devices)


def ls(**kwargs):
    """Enumerate FPGA devices"""
    for device in lsfpga(**kwargs):
        print(f'[{device}] ({device.vendor}:{device.device} '
              f'{device.subsystem_vendor}:{device.subsystem_device}) '
              f'{device.desc} (Driver: {device.driver})')


def open_pciaddr(pci_addr):
    device = libvfio.device.open(pci_addr)
    if device:
        cmd = 0b10 # enable memory space access
        command_offset = pcicfg.command.value
        value = device.config_read16(command_offset)
        if value | cmd != value:
            LOG.warn('setting 0x%x to command register', cmd)
            device.config_write16(command_offset, value | cmd)
        else:
            LOG.debug('command register: 0x%x', value)
    return device


def find_device(pci_addr=None):
    if pci_addr is not None:
        d = open_pciaddr(pci_addr)
        if not d:
            errstr = 'cannot open device: {}'.format(pci_addr)
            LOG.error(errstr)
            raise OSError(os.EX_OSERR, errstr)
        return d

    for fpga in lsfpga():
        if fpga.driver == 'vfio-pci':
            iommu_group = read_link('/sys/bus/pci/devices', fpga.addr, 'iommu_group')
            vfiodev = Path('/dev/vfio', iommu_group.stem)
            if vfiodev.exists():
                dev = open_pciaddr(fpga.addr)
                if dev:
                    return dev


def find_region(device, region):
    for r in device.regions:
        if r.index() == region:
            return r


def iommu_enabled():
    iommu_on = any(Path('/sys/kernel/iommu_groups').iterdir())
    return iommu_on
