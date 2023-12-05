# Copyright(c) 2023, Intel Corporation
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

"""Provides utility classes for parsing PCIe addresses, PCIe device id's, and
   hexadecimal integers. Also includes helper functions for manipulating
   opae.fpga.properties objects."""

import re
import struct
import uuid
from opae import fpga


ADDRESS_PATTERN = (r'^(?P<pcie_address>'
                   r'(?:(?P<segment>[\da-f]{4}):)?'
                   r'(?P<bdf>(?P<bus>[\da-f]{2}):'
                   r'(?P<device>[\da-f]{2})\.(?P<function>[0-7]{1})))$')

ID_PATTERN = (r'^(?P<pcie_id>'
              r'(?P<vendor_id>[\da-f]{4}):(?P<device_id>[\da-f]{4})'
              r'(?:\s+(?P<subsystem_vendor_id>[\da-f]{4}):(?P<subsystem_device_id>[\da-f]{4}))?'
              r')$')

HEX_INT_PATTERN = (r'^(?P<hex_int>'
                   r'(?:(?P<sign>[-+]))?'
                   r'(?:(?P<prefix>0[x]))?'
                   r'(?P<number>[\da-f]+)'
                   r')$') 


class pcie_address():
    """Parse a PCIe address from a string such that its
       component parts can be used to initialize an
       opae.fpga.properties object."""
    regex = re.compile(ADDRESS_PATTERN, re.IGNORECASE)

    def __init__(self, addr_str: str):
        self.candidate = addr_str
        self.address = None
        self.properties = {}
        mg = self.regex.match(self.candidate)
        if mg:
            d = mg.groupdict()
            if d['segment'] is None:
                d['segment'] = '0000'
                d['pcie_address'] = d['segment'] + ':' + d['bdf']

            self.address = d['pcie_address']
            self.properties = {
                               'segment': int(d['segment'], 16),
                               'bus': int(d['bus'], 16),
                               'device': int(d['device'], 16),
                               'function': int(d['function']),
                              }

    def is_ok(self) -> bool:
        """Did the given string used to initialize the object
           successfully parse into a PCIe address?"""
        return self.address is not None

    def __str__(self):
        return self.address if self.address else self.candidate

    def __repr__(self):
        return str(self)

    def __eq__(self, other):
        return str(self) == str(other)


class pcie_id():
    """Parse a PCIe VID:DID [SVID:SDID] from a string such
       that its component parts can be used to initialize an
       opae.fpga.properties object."""
    regex = re.compile(ID_PATTERN, re.IGNORECASE)

    def __init__(self, id_str: str):
        self.candidate = id_str
        self.id = None
        self.properties = {}
        mg = self.regex.match(self.candidate)
        if mg:
            d = mg.groupdict()
            self.id = d['pcie_id']
            self.properties = {
                               'vendor_id': int(d['vendor_id'], 16),
                               'device_id': int(d['device_id'], 16),
                              }
            if d['subsystem_vendor_id'] and d['subsystem_device_id']:
                self.properties['subsystem_vendor_id'] = int(d['subsystem_vendor_id'], 16)
                self.properties['subsystem_device_id'] = int(d['subsystem_device_id'], 16)

    def is_ok(self) -> bool:
        """Did the given string used to initialize the object
           successfully parse into a PCIe ID?"""
        return self.id is not None

    def __str__(self):
        return self.id if self.id else self.candidate

    def __repr__(self):
        return str(self)

    def __eq__(self, other):
        return str(self) == str(other)


def device_filter(addr_str: str=None, id_str: str=None):
    """Create a device filter dictionary from the given
       PCIe address and PCIe ID strings. The resulting
       dictionary is suitable to pass as kwargs to
       opae.fpga.enumerate()."""
    filt = {}
    if addr_str:
        addr = pcie_address(addr_str)
        if addr.is_ok():
            filt.update(addr.properties)
    if id_str:
        ID = pcie_id(id_str)
        if ID.is_ok():
            filt.update(ID.properties)
    return filt


def undo_device_filter(filt: dict):
    """Given a dictionary created by device_filter(), convert
       that dictionary back into its string form, ie the
       original PCIe address or PCIe ID in string form."""
    if filt is None:
        return ''
    if 'segment' in filt:
        return f'{filt["segment"]:04x}:{filt["bus"]:02x}:{filt["device"]:02x}.{filt["function"]}'
    elif 'vendor_id' in filt:
        ID = f'{filt["vendor_id"]:04x}:{filt["device_id"]:04x}'
        if 'subsystem_vendor_id' in filt:
            ID += ' {filt["subsystem_vendor_id"]:04x}:{filt["subsystem_device_id"]:04x}'
        return ID
    return ''


class hex_int():
    """Parse a string as a decimal or hexadecimal integer."""
    regex = re.compile(HEX_INT_PATTERN, re.IGNORECASE)

    def __init__(self, int_str, hex_str: bool=True, print_zeros: bool=True):
        if isinstance(int_str, int):
            int_str = str(int_str)
        self.candidate = int_str
        self.hex_str = hex_str
        self.print_zeros = print_zeros
        self.prefix = None
        self.integer = None
        self.width = 8
        mg = self.regex.match(self.candidate)
        if mg:
            d = mg.groupdict()
            self.integer = int(d['hex_int'], 0)
            self.prefix = d['prefix']
            if self.integer > 0xffffffff:
                self.width = 16
            elif self.integer > 0xffff:
                self.width = 8
            elif self.integer > 0xff:
                self.width = 4
            else:
                self.width = 2

    def is_ok(self) -> bool:
        """Did the given string used to initialize the object
           successfully parse into a hex int?"""
        return self.integer is not None

    def __str__(self):
        if self.hex_str and self.prefix:
            fmt = self.prefix + '{:'
            if self.print_zeros:
                fmt += '0'
                fmt += str(self.width)
            fmt += 'x}'
            return fmt.format(self.integer) if self.integer else self.candidate
        return str(self.integer) if self.integer else self.candidate

    def __repr__(self):
        return str(self)

    def __int__(self):
        return self.integer

    def __eq__(self, other):
        if isinstance(other, hex_int):
            return self.integer == other.integer
        return self.integer == other

    def __add__(self, other):
        if isinstance(other, hex_int):
            return self.integer + other.integer
        return self.integer + other

    def __format__(self, format_spec):
        if not format_spec:
            return format(str(self), format_spec)
        return format(self.integer, format_spec)


class MemoryAccessWidthError(RuntimeError):
    """Raised when a memory_access object is constructed with
       an invalid access width."""
    def __init__(self, msg):
        super().__init__(msg)


class memory_access():
    """Provide an abstraction around memory accesses. The given
       access_width (32 or 64) determines how the read and write
       methods acquire the memory, whether 32 or 64 bits at a time."""
    def __init__(self, access_width, hndl=None):
        self.hndl = hndl
        self.access_width = access_width
        if self.access_width not in [64, 32]:
            raise MemoryAccessWidthError(f'Only 32 and 64 '
                                         f'are supported, not {access_width}')

    def read(self, offset, size, region=0):
        if size == 64:
            if self.access_width == 64:
                return self.hndl.read_csr64(offset, region)
            elif self.access_width == 32:
                low = self.hndl.read_csr32(offset, region)
                high = self.hndl.read_csr32(offset + 4, region)
                return (high << 32) | low
        elif size == 32:
            return self.hndl.read_csr32(offset, region)

    def write(self, offset, value, size, region=0):
        """Write the given value to the offset in memory. size is
           either 32 or 64."""
        if size == 64:
            if self.access_width == 64:
                self.hndl.write_csr64(offset, value, region)
            elif self.access_width == 32:
                self.hndl.write_csr32(offset, value & 0xffffffff, region)
                self.hndl.write_csr32(offset + 4, value >> 32, region)
        elif size == 32:
            self.hndl.write_csr32(offset, value, region)

    def read_guid(self, offset, region=0):
        """Read and decipher the first two quadwords at offset
           as a 16-byte guid encoding."""
        low = self.read(offset, 64, region)
        high = self.read(offset + 8, 64, region)
        return uuid.UUID(bytes=struct.pack('>QQ', high, low))


def properties_to_address(p: fpga.properties) -> pcie_address:
    """Convert an opae.fpga.properties object containing the fields
       of a PCIe address back to its string form."""
    return pcie_address(f'{p.segment:04x}:{p.bus:02x}:{p.device:02x}.{p.function}')


def tok_or_handle_to_address(tok_or_handle) -> pcie_address:
    """Retrieve the properties of the given token or handle and
       use them to construct the PCIe address."""
    return properties_to_address(fpga.properties(tok_or_handle))


def properties_to_id(p: fpga.properties) -> pcie_id:
    """Convert an opae.fpga.properties object containing the
       fields of a PCIe ID back to its string form."""
    return pcie_id(f'{p.vendor_id:04x}:{p.device_id:04x} '
                   f'{p.subsystem_vendor_id:04x}:{p.subsystem_device_id:04x}')
