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

"""Define FPGA mailbox access class and register bits."""

import time

from ctypes import LittleEndianStructure, c_uint64
from opae.fpga.dfh import CSR


# Generic mailbox poll timeout == 100 usec
POLL_TIMEOUT = 1/10000

# Wait for read response timeout == 1 usec
POLL_SLEEP = 1/1000000


class MAILBOX_CMD_STATUS_BITS(LittleEndianStructure):
    """Mailbox command/status CSR bits."""
    _fields_ = [
        ("read_cmd", c_uint64, 1),
        ("write_cmd", c_uint64, 1),
        ("ack_trans", c_uint64, 1),
        ("reserved", c_uint64, 29),
        ("cmd_addr", c_uint64, 32),
    ]


class command_status(CSR):
    """Mailbox command and status register"""
    _fields_ = [("bits", MAILBOX_CMD_STATUS_BITS),
                ("value", c_uint64)]
    width = 64


class MAILBOX_DATA_BITS(LittleEndianStructure):
    """Mailbox read data/write data CSR bits."""
    _fields_ = [
        ("read_data", c_uint64, 32),
        ("write_data", c_uint64, 32),
    ]


class mb_data(CSR):
    """Mailbox data register"""
    _fields_ = [("bits", MAILBOX_DATA_BITS),
                ("value", c_uint64)]
    width = 64


class MailboxCSRAccessError(RuntimeError):
    """Raised when a mailbox_access is constructed with
       an invalid access width."""
    def __init__(self, msg):
        super().__init__(msg)


class MailboxAckTransTimeout(RuntimeError):
    """Raised when a mailbox read or write operation
       fails to see the ACK transaction bit transition
       to 1 within the timeout period."""
    def __init__(self, msg):
        super().__init__(msg)


class mailbox_access():
    """Mailbox read/write access class. The read and write methods
       are defined with the same signatures as the memory_access
       class so that the two can be used interchangeably."""
    def __init__(self, access_width, cmd_reg_offset, hndl=None,
                 poll_to=POLL_TIMEOUT, poll_sleep=POLL_SLEEP):
        self.hndl = hndl
        self.access_width = access_width
        if self.access_width not in [64, 32]:
            raise MailboxCSRAccessError(f'Only 32 and 64 '
                                        f'are supported, not {access_width}')
        self.cmd_stat_offset = cmd_reg_offset
        self.rd_data_offset = self.cmd_stat_offset + 0x8
        self.wr_data_offset = self.cmd_stat_offset + 0xc
        self.poll_to = poll_to
        self.poll_sleep = poll_sleep

    # For these read/write accessors, we know the register width implicitly
    # as defined in the classes above.
    def read_cmd_stat(self, region=0) -> command_status:
        """Retrieve the current contents of the mailbox command and
           status register."""
        if self.access_width == 64:
            value = self.hndl.read_csr64(self.cmd_stat_offset, region)
        elif self.access_width == 32:
            low = self.hndl.read_csr32(self.cmd_stat_offset, region)
            high = self.hndl.read_csr32(self.cmd_stat_offset + 4, region)
            value = (high << 32) | low
        return command_status(value)

    def write_cmd_stat(self, cmd_stat: command_status, region=0):
        """Write the contents of the given mailbox command and
           status register."""
        if self.access_width == 64:
            self.hndl.write_csr64(self.cmd_stat_offset, cmd_stat.value, region)
        elif self.access_width == 32:
            low = cmd_stat.value & 0xffffffff
            high = cmd_stat.value >> 32
            self.hndl.write_csr32(self.cmd_stat_offset, low, region)
            self.hndl.write_csr32(self.cmd_stat_offset + 4, high, region)

    def read_mb_data(self, region=0) -> mb_data:
        """Retrieve the current contents of the mailbox data register."""
        if self.access_width == 64:
            value = self.hndl.read_csr64(self.rd_data_offset, region)
        elif self.access_width == 32:
            low = self.hndl.read_csr32(self.rd_data_offset, region)
            high = self.hndl.read_csr32(self.wr_data_offset, region)
            value = (high << 32) | low
        return mb_data(value)

    def write_mb_data(self, data: mb_data, region=0):
        """Write the given value to the mailbox data register."""
        if self.access_width == 64:
            self.hndl.write_csr64(self.rd_data_offset, data.value, region)
        elif self.access_width == 32:
            rd = data.bits.read_data
            wr = data.bits.write_data
            self.hndl.write_csr32(self.rd_data_offset, rd, region)
            self.hndl.write_csr32(self.wr_data_offset, wr, region)

    def poll_for_ack_trans(self, timeout, region=0):
        """Examine the mailbox command and status register until
           bit 2 (Ack Transaction) transitions to 1."""
        total_time = 0
        sl = self.poll_sleep
        cmd_stat = self.read_cmd_stat(region)
        while total_time < timeout:
            if cmd_stat.bits.ack_trans:
                return (True, cmd_stat)
            time.sleep(sl)
            total_time += sl
            cmd_stat = self.read_cmd_stat(region)
        return (False, cmd_stat)

    def read(self, offset, size=64, region=0):
        """Perform mailbox read protocol and return the
           resulting field of the read/write data CSR."""
        # Clear the command/status CSR and wait.
        self.write_cmd_stat(command_status(0), region)
        time.sleep(self.poll_to)

        # Set read command bit(0) and read address.
        cmd_stat = command_status(1)
        cmd_stat.bits.cmd_addr = offset
        self.write_cmd_stat(cmd_stat, region)

        # Poll for ack transaction bit.
        ok, cmd_stat = self.poll_for_ack_trans(self.poll_to, region)
        if not ok:
            data = self.read_mb_data(region)
            msg = (f'mailbox read ACK timeout. '
                   f'command: {cmd_stat.value:016x} data: {data.value:016x}')
            raise MailboxAckTransTimeout(msg)

        return self.read_mb_data(region).bits.read_data

    def write(self, offset, value, size=64, region=0):
        """Perform mailbox write protocol."""
        # Clear the command/status CSR and wait.
        self.write_cmd_stat(command_status(0), region)
        time.sleep(self.poll_to)

        # Read-modify-write the data CSR.
        write_data = self.read_mb_data(region)
        write_data.bits.write_data = value
        self.write_mb_data(write_data, region)

        # Set write command bit(1) and write address.
        cmd_stat = command_status(2)
        cmd_stat.bits.cmd_addr = offset
        self.write_cmd_stat(cmd_stat, region)

        # Poll for ack transaction bit.
        ok, cmd_stat = self.poll_for_ack_trans(self.poll_to, region)
        if not ok:
            data = self.read_mb_data(region)
            msg = (f'mailbox write ACK timeout. '
                   f'command: {cmd_stat.value:016x} data: {data.value:016x}')
            raise MailboxAckTransTimeout(msg)


def feature_id_to_mailbox_base(feature_id: int) -> int:
    """Give a hint about the location of the mailbox
       command and status CSR, based on a feature id."""
    bases = {
             0x15: 0xa8, # HSSI Subsystem
             0x20: 0x28, # PCIe Subsystem
            }
    if feature_id in bases:
        return bases[feature_id]
    return None
