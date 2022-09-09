# Copyright(c) 2022, Intel Corporation
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
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISEDL OF THE
# POSSIBILITY OF SUCH DAMAGE.
import re
from pathlib import Path
from collections import namedtuple


PCI_ADDRESS_PATTERN = (r'(?P<pci_address>'
                       r'(?:(?P<segment>[\da-f]{4}):)?'
                       r'(?P<bdf>(?P<bus>[\da-f]{2}):'
                       r'(?P<device>[\da-f]{2})\.(?P<function>\d)))')
PCI_ADDRESS_RE = re.compile(PCI_ADDRESS_PATTERN, re.IGNORECASE)


VENDOR_DEVICE_PATTERN = r'(?P<vendor>[\da-f]{0,4}):(?P<device>[\da-f]{0,4})'
VENDOR_DEVICE_RE = re.compile(VENDOR_DEVICE_PATTERN, re.IGNORECASE)

pci_id = namedtuple(
    'pci_id', [
        'vendor', 'device', 'subsystem_vendor', 'subsystem_device'])

pci_id.short = lambda i: pci_id(i.vendor, i.device, 0, 0)


def make_id(vendor, device, s_vendor=0, s_device=0):
    return pci_id(vendor, device, s_vendor, s_device)


SYSFS_PCI_DEVICES = '/sys/bus/pci/devices'


def pci_address(inp):
    m = PCI_ADDRESS_RE.match(inp)
    if not m:
        raise ValueError('wrong pci address format: {}'.format(inp))

    d = m.groupdict()
    return '{}:{}'.format(d.get('segment') or '0000', d['bdf']).lower()


def vendev(inp):
    m = VENDOR_DEVICE_RE.match(inp)
    if not m:
        raise ValueError('wrong vendor/device format: {}'.format(inp))
    return dict((k, int(v, 16)) for k, v in m.groupdict().items() if v)


def vid_did_for_address(pci_addr):
    path = Path('/sys/bus/pci/devices', pci_addr)
    vid = path.joinpath('vendor').read_text().strip()
    did = path.joinpath('device').read_text().strip()
    svid = path.joinpath('subsystem_vendor').read_text().strip()
    sdid = path.joinpath('subsystem_device').read_text().strip()
    return (vid, did, svid, sdid)


class sysfs_attr:
    def __init__(self, parent, path: Path):
        self.parent = parent
        self.path = path

    @property
    def value(self):
        with self.parent.path.joinpath(self.path).open('r') as fp:
            return fp.read()

    def __str__(self):
        return self.value.strip()

    def __repr__(self):
        return f'{self.parent}.{self.path} = {self}'

    def __int__(self):
        return int(self.value.strip(), 0)

    def __eq__(self, other):
        args = [self.value.strip()]
        if isinstance(other, int):
            args.append(0)
        return type(other)(*args) == other


CLASSES = {
    0x01: "Mass storage controller",
    0x02: "Network controller",
    0x03: "Display controller",
    0x04: "Multimedia device",
    0x05: "Memory controller",
    0x06: "Bridge device",
    0x07: "Simple communication controllers ",
    0x08: "Base system peripherals ",
    0x09: "Input devices ",
    0x0A: "Docking stations ",
    0x0B: "Processors ",
    0x0C: "Serial bus controllers ",
    0x0D: "Wireless controller",
    0x0E: "Intelligent I/O controllers",
    0x0F: "Satellite communication controllers",
    0x10: "Encryption/Decryption controllers",
    0x11: "Data acquisition and signal processing controllers",
    0x12: "Processing accelerators",
    0x13: "Non-Essential Instrumentation"
}


class device:
    def __init__(self, path: Path):
        self.path = path
        self.addr = path.name
        self._desc = None

    def __repr__(self):
        return self.addr

    def __getattr__(self, path):
        return sysfs_attr(self, path)

    @property
    def driver(self):
        d = self.path.joinpath('driver')
        if d.exists():
            return d.resolve().name

    @property
    def desc(self):
        return self._desc or self.lookup_class()

    def lookup_class(self):
        class_code = int(sysfs_attr(self, 'class')) >> 16
        return CLASSES.get(class_code)

    @property
    def pci_id(self):
        values = {}
        for a in pci_id._fields:
            try:
                values[a] = int(getattr(self, a).value, 0)
            except FileNotFoundError:
                values[a] = 0
        if values:
            return pci_id(values['vendor'],
                          values['device'],
                          values['subsystem_vendor'],
                          values['subsystem_device'])
        return None


def ls(**kwargs):
    all_devices = map(device, Path(SYSFS_PCI_DEVICES).glob('*'))
    filter_fn = kwargs.pop('filter', None)
    if filter_fn:
        return filter(filter_fn, all_devices)
    return all_devices
