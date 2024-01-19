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

"""Provides a generic means to examine FPGA device MMIO and present it as
   a hexadecimal dump."""

import struct


#           1111111111222222222233333333334444444444555555555566666666667777777777
# 01234567890123456789012345678901234567890123456789012345678901234567890123456789
#
# 32
# ADDR       COL0     COL1     COL2     COL3      DECODE
# 00000000:  00000001 00000002 00000003 00000004  |0000000000000000|
#
# 64
# ADDR       COL0               COL1              DECODE
# 00000000:  0000000200000001   0000000400000003  |0000000000000000|

ADDR = 0
COL0 = 1
COL1 = 2
COL2 = 3
COL3 = 4
DECODE = 5
END = 6


class hex_view():
    """Given a memory_access or a mailbox_access object and an access
       width (32 or 64), produce a decoded hexadecimal dump for a given
       number of bytes."""
    def __init__(self, access, width):
        self.access = access
        self.width = width

    @staticmethod
    def decode(integer, length, order):
        """Decode a 32- or 64-bit integer into its printable character
           representation. When individual bytes of the given integer
           represent printable characters, the resulting string will
           contain those characters, else if a character is non-printable,
           the string will contain a '.' character."""
        to_bytes = integer.to_bytes(length=length, byteorder=order)
        decoded = ''
        for b in struct.unpack(str(length) + 'c', to_bytes):
            try:
                b = b.decode()
                decoded += b if b.isprintable() else '.'
            except UnicodeDecodeError:
                decoded += '.'
        return decoded

    def render32(self, start_addr, byte_count, region, fp):
        """Render a 32-bit memory dump to file fp. The dump starts
           at memory address start_addr and continues for byte_count
           bytes."""
        length = 4
        order = 'little'
        bytes_per_row = 16
        addr_to_display = start_addr & ~(bytes_per_row - 1)
        addr = addr_to_display
        state = ADDR
        remaining = byte_count
        end = start_addr + byte_count
        decode = ''

        while state != END:
            if state == ADDR:
                print(f'{addr:08x}:', end='', file=fp)
                state = COL0
            elif state == COL0:
                printing = (addr >= start_addr) and (addr < end)
                if printing:
                    col0 = self.access.read(addr, self.width, region)
                    decode += self.decode(col0, length, order)
                    print(f'  {col0:08x}', end='', file=fp)
                    remaining -= length
                else:
                    decode += ' ' * length
                    print(' ' * 10, end='', file=fp)
                addr += length
                state = COL1
            elif state == COL1:
                printing = (addr >= start_addr) and (addr < end)
                if printing:
                    col1 = self.access.read(addr, self.width, region)
                    decode += self.decode(col1, length, order)
                    print(f' {col1:08x}', end='', file=fp)
                    remaining -= length
                else:
                    decode += ' ' * length
                    print(' ' * 9, end='', file=fp)
                addr += length
                state = COL2
            elif state == COL2:
                printing = (addr >= start_addr) and (addr < end)
                if printing:
                    col2 = self.access.read(addr, self.width, region)
                    decode += self.decode(col2, length, order)
                    print(f' {col2:08x}', end='', file=fp)
                    remaining -= length
                else:
                    decode += ' ' * length
                    print(' ' * 9, end='', file=fp)
                addr += length
                state = COL3
            elif state == COL3:
                printing = (addr >= start_addr) and (addr < end)
                if printing:
                    col3 = self.access.read(addr, self.width, region)
                    decode += self.decode(col3, length, order)
                    print(f' {col3:08x}', end='', file=fp)
                    remaining -= length
                else:
                    decode += ' ' * length
                    print(' ' * 9, end='', file=fp)
                addr += length
                state = DECODE
            elif state == DECODE:
                print(f'  |{decode}|', file=fp)
                decode = ''
                state = END if addr - start_addr >= byte_count else ADDR

    def render64(self, start_addr, byte_count, region, fp):
        """Render a 64-bit memory dump to file fp. The dump starts
           at memory address start_addr and continues for byte_count
           bytes."""
        length = 8
        order = 'little'
        bytes_per_row = 16
        addr_to_display = start_addr & ~(bytes_per_row - 1)
        addr = addr_to_display
        state = ADDR
        remaining = byte_count
        end = start_addr + byte_count
        decode = ''

        while state != END:
            if state == ADDR:
                print(f'{addr:08x}:', end='', file=fp)
                state = COL0
            elif state == COL0:
                printing = (addr >= start_addr) and (addr < end)
                if printing:
                    col0 = self.access.read(addr, self.width, region)
                    decode += self.decode(col0, length, order)
                    print(f'  {col0:016x}', end='', file=fp)
                    remaining -= length
                else:
                    decode += ' ' * length
                    print(' ' * 18, end='', file=fp)
                addr += length
                state = COL1
            elif state == COL1:
                printing = (addr >= start_addr) and (addr < end)
                if printing:
                    col1 = self.access.read(addr, self.width, region)
                    decode += self.decode(col1, length, order)
                    print(f'   {col1:016x}', end='', file=fp)
                    remaining -= length
                else:
                    decode += ' ' * length
                    print(' ' * 19, end='', file=fp)
                addr += length
                state = DECODE
            elif state == DECODE:
                print(f'  |{decode}|', file=fp)
                decode = ''
                state = END if addr - start_addr >= byte_count else ADDR

    def render(self, start_addr, byte_count, region, fp):
        """Render the hex_view to file fp, based on the width
           parameter given during construction."""
        if self.width == 32:
            self.render32(start_addr, byte_count, region, fp)
        else:
            self.render64(start_addr, byte_count, region, fp)
