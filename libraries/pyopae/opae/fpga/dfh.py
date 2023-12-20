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

"""Define Configuration and Status Register bit layouts for Device Feature
Header version 0 and version 1."""

from ctypes import Union, LittleEndianStructure, c_uint64


class CSR(Union):
    """Configuration and Status Register"""
    def __init__(self, value):
        self.value = value


class DFH0_BITS(LittleEndianStructure):
    """Bit fields for DFH v0."""
    _fields_ = [
        ('id', c_uint64, 12),          # Feature ID
        ('rev', c_uint64, 4),          # Revision of feature
        ('next', c_uint64, 24),
        ('eol', c_uint64, 1),
        ('reserved', c_uint64, 19),
        ('feature_type', c_uint64, 4), # 1=AFU,3=private,4=FIU,5=interface
    ]


class dfh0(CSR):
    """Device Feature Header version 0"""
    _fields_ = [('bits', DFH0_BITS),
                ('value', c_uint64)]
    width = 64


class DFH1_BITS(LittleEndianStructure):
    """Bit fields for DFH v1."""
    _fields_ = [
        ('id', c_uint64, 12),          # Feature ID
        ('rev', c_uint64, 4),          # Revision of feature
        ('next', c_uint64, 24),
        ('eol', c_uint64, 1),
        ('reserved', c_uint64, 11),
        ('dfh_version', c_uint64, 8),  # DFH version (1)
        ('feature_type', c_uint64, 4), # 1=AFU,3=private,4=FIU,5=interface
    ]


class dfh1(CSR):
    """Device Feature Header version 1"""
    _fields_ = [('bits', DFH1_BITS),
                ('value', c_uint64)]
    width = 64
