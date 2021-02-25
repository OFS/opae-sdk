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

import contextlib
import glob
import grp
import json
import os
import pickle
import pwd
import re
import struct
import subprocess
import sys
import uuid
import time

from enum import Enum
from ctypes import Union, LittleEndianStructure, c_uint64

if sys.version_info[0] == 3:
    from pathlib import Path
else:
    from pathlib2 import Path

import libvfio

PCI_ADDRESS_PATTERN = (r'(?P<pci_address>'
                       r'(?:(?P<segment>[\da-f]{4}):)?'
                       r'(?P<bdf>(?P<bus>[\da-f]{2}):'
                       r'(?P<device>[\da-f]{2})\.(?P<function>\d)))')
PCI_ADDRESS_RE = re.compile(PCI_ADDRESS_PATTERN, re.IGNORECASE)


VENDOR_DEVICE_PATTERN = r'(?P<vendor>[\da-f]{0,4}):(?P<device>[\da-f]{0,4})'
VENDOR_DEVICE_RE = re.compile(VENDOR_DEVICE_PATTERN, re.IGNORECASE)

PICKLE_FILE = '/var/lib/opae/opae.io.pickle'


class pcicfg(Enum):
    command = 0x4

def hex_int(inp):
    return int(inp, 0)

def vendev(inp):
    m = VENDOR_DEVICE_RE.match(inp)
    if not m:
        raise ValueError('wrong vendor/device format: {}'.format(inp))
    return m.groupdict()

def pci_address(inp):
    m = PCI_ADDRESS_RE.match(inp)
    if not m:
        raise ValueError('wrong pci address format: {}'.format(inp))

    d = m.groupdict()
    return '{}:{}'.format(d.get('segment') or '0000', d['bdf'])

def vid_did_for_address(pci_addr):
    path = Path('/sys/bus/pci/devices', pci_addr)
    vid = path.joinpath('vendor').read_text().strip()
    did = path.joinpath('device').read_text().strip()
    return (vid, did)

def load_driver(driver):
    return subprocess.call(['modprobe', driver])

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
        with open(bind, 'w') as outf:
            outf.write(pci_addr)

def get_dev_dict(file_name):
    if os.path.isfile(file_name):
        with open(file_name, 'rb') as inf:
            dev_dict = pickle.load(inf)
            return dev_dict

def put_dev_dict(file_name, dev_dict):
    d = os.path.dirname(file_name)
    if not os.path.isdir(d):
        os.makedirs(d)
    with open(file_name, 'wb') as outf:
        pickle.dump(dev_dict, outf)

def vfio_init(pci_addr, new_owner=''):
    vid_did = vid_did_for_address(pci_addr)
    driver = get_bound_driver(pci_addr)

    msg = '(0x{:04x},0x{:04x}) at {}'.format(
        int(vid_did[0], 16), int(vid_did[1], 16), pci_addr)

    if driver and driver != 'vfio-pci':
        dev_dict = get_dev_dict(PICKLE_FILE)
        if not dev_dict:
            dev_dict = {}
        dev_dict[pci_addr] = driver
        put_dev_dict(PICKLE_FILE, dev_dict)
        print('Unbinding {} from {}'.format(msg, driver))
        unbind_driver(driver, pci_addr)

    load_driver('vfio-pci')

    print('Binding {} to vfio-pci'.format(msg))
    new_id = '/sys/bus/pci/drivers/vfio-pci/new_id'
    with open(new_id, 'w') as outf:
        outf.write('{} {}'.format(vid_did[0], vid_did[1]))

    time.sleep(0.25)

    iommu_group = os.path.join('/sys/bus/pci/devices',
                               pci_addr,
                               'iommu_group')
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

def vfio_release(pci_addr):
    vid_did = vid_did_for_address(pci_addr)
    driver = get_bound_driver(pci_addr)

    msg = '(0x{:04x},0x{:04x}) at {}'.format(
        int(vid_did[0], 16), int(vid_did[1], 16), pci_addr)

    if not driver or driver != 'vfio-pci':
        print('{} is not bound to vfio-pci'.format(msg))
        return

    print('Releasing {} from vfio-pci'.format(msg))
    unbind_driver(driver, pci_addr)

    dev_dict = get_dev_dict(PICKLE_FILE)
    if not dev_dict:
        return

    driver = dev_dict.get(pci_addr)
    if driver:
        print('Rebinding {} to {}'.format(msg, driver))
        bind_driver(driver, pci_addr)
        del dev_dict[pci_addr]
        if dev_dict:
            put_dev_dict(PICKLE_FILE, dev_dict)
        else:
            os.remove(PICKLE_FILE)

class opae_register(Union):
    def __init__(self, offset, value=None):
        self.offset = offset
        if value is None:
            self.update()
        else:
            self.value = value

    def __str__(self):
        value = ''
        return ', '.join(['{name} = 0x{value:x}'.format(name=name,
                                                       value=getattr(self.bits, name))
                         for name, _, _ in self.bits._fields_])

    @classmethod
    def define(cls, name, bits=[('value', c_uint64, 64)], print_hex=True):
        bits_class = type('{name}_bits'.format(**locals()),
                          (LittleEndianStructure, ),
                          dict(_fields_=bits))
        return type(name,
                    (cls,),
                    dict(_fields_=[('bits', bits_class),
                                   ('value', c_uint64)]))

    def commit(self, value=None):
        if value is not None:
            self.value = value
        write_csr(self.offset, self.value)

    def update(self):
        if the_region is None:
            raise OSError(os.EX_OSERR, 'no region open')
        self.value = read_csr(self.offset)

    def __enter__(self):
      pass

    def __exit__(self, exc_type, exc_value, tb):
      if exc_type and exc_value and tb:
          return False
      self.commit()

def register(name="value_register", bits=[('value', c_uint64, 64)]):
    '''Create a register class dynamicaly using a list of bitfield tuples.

    Args:
        name: The name of the register class. Defaults to "value_register".
        bits: A list of three-element tuples describing the bitfields.
              The first element in the tuple is the name of the field.
              The second element in the tuple is the width of the register. Must be c_uint64.
              The third element in the tuple is the size of the field in bits.

    Note:
        The register class is a little endian structure with bitfields starting at bit 0 going
        from top to bottom.
    '''
    r_class = opae_register.define(name, bits)
    return r_class


def read_guid(offset):
    lo = read_csr(offset)
    hi = read_csr(offset+0x08)
    return uuid.UUID(bytes=struct.pack('>QQ', hi, lo))


dfh0_bits = [
        ('id', c_uint64, 12),
        ('rev', c_uint64, 4),
        ('next', c_uint64,24),
        ('eol', c_uint64, 1),
        ('reserved', c_uint64, 19),
        ('feature_type', c_uint64, 4)
]

dfh0 = register('dfh0', bits=dfh0_bits)

def dfh_walk(offset=0, header=dfh0, guid=None):
    while True:
        h = header(offset)
        if guid is None or guid == read_guid(offset+0x8):
            yield offset, h
        if h.bits.eol or not h.bits.next:
            break
        offset += h.bits.next

class feature(object):
    def __init__(self, offset=0, guid=None):
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
            csr = csr_class(self._offset + offset)
            yield csr
            err = False
        finally:
            if not err:
                csr.commit()
            
fpga_devices = {(0x8086, 0x09c4) : "Intel PAC A10 GX",
                (0x8086, 0x09c5) : "Intel PAC A10 GX VF",
                (0x8086, 0x0b2b) : "Intel PAC D5005",
                (0x8086, 0x0b2c) : "Intel PAC D5005 VF",
                (0x8086, 0x0b30) : "Intel PAC N3000",
                (0x8086, 0x0b31) : "Intel PAC N3000 VF"}



def read_attr(dirname, attr):
    fname = Path(dirname, attr)
    if fname.exists():
        return fname.read_text().strip()

def read_link(dirname, *attr):
    fname = Path(dirname, *attr)
    if fname.exists():
        return fname.resolve()


def get_conf():
    conf_file = os.path.expanduser('~/.opae-io.conf')
    if os.path.exists(conf_file):
        with open(conf_file, 'r') as fp:
            conf = json.load(fp)
        if isinstance(conf, dict):
            return conf
        print('not a valid format')
    return {}


def lsfpga(**kwargs):
    vendor = kwargs.get('vendor')
    device = kwargs.get('device')
    driver = kwargs.get('driver')
    device_ids = dict(fpga_devices)
    conf = get_conf()
    conf_ids = {}
    if conf:
        for k,v in conf.get('fpga_devices', {}).items():
            try:
                vstr, dstr = k.split(':')
            except:
                print('error with vendor/device: {}'.format(k))
            else:
                conf_ids[(int(vstr, 16), int(dstr, 16))] = v
            
        if conf.get('override', False):
            device_ids.update(conf_ids)
        else:
            for k, v in conf_ids.items():
                if k not in device_ids:
                    device_ids[k] = v

    if kwargs.get('all'):
        device_ids = {}

    for pcidir in Path('/sys/bus/pci/devices').glob('*'):
        vid = read_attr(pcidir, 'vendor')
        did = read_attr(pcidir, 'device')
        drv = read_link(pcidir, 'driver')
        if drv:
            drv = drv.stem
        if vid is None or did is None:
            continue
        if vendor and int(vid, 16) != int(vendor, 16):
            continue
        if device and int(did, 16) != int(device, 16):
            continue
        id_tuple = (int(vid, 0), int(did, 0))
        if not device_ids or id_tuple in device_ids:
            if not driver or (driver and drv and driver == drv):
                yield (pcidir.parts[-1], vid, did, device_ids.get(id_tuple, ''), drv)


def ls(**kwargs):
    """Enumerate FPGA devices"""
    for pci_addr, vid, did, name, drv in lsfpga(**kwargs):
        print('[{}] ({}, {}) {} (Driver: {})'.format(pci_addr, vid, did, name, drv))


def open_pciaddr(pci_addr):
    device = libvfio.device.open(pci_addr)
    conf = get_conf()
    if device:
        cmd = conf.get('set-pci-command', 0b10)
        if cmd:
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
        
    for pci_addr, vid, did, name, driver in lsfpga():
        if driver == 'vfio-pci':
            iommu_group = read_link('/sys/bus/pci/devices', pci_addr, 'iommu_group')
            vfiodev = Path('/dev/vfio', iommu_group.stem)
            if vfiodev.exists():
                device = open_pciaddr(pci_addr)
                if device:
                    return device


def find_region(device, region):
    for r in device.regions:
        if r.index() == region:
            return r
