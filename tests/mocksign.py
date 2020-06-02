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
import hashlib
import io
import struct
import tempfile

from pacsign import database


class bitstream(object):
    def __init__(self, payload):
        self.payload = payload
        self.block0 = self.create_block0()
        self.block1 = self.create_block1()
        self.bytes_io = io.BytesIO()
        self.bytes_io.write(self.block0)
        self.bytes_io.write(self.block1)
        self.bytes_io.write(self.payload)

    def create_block0(self):
        m256 = hashlib.sha256()
        m384 = hashlib.sha384()
        m256.update(self.payload)
        m384.update(self.payload)
        bio = io.BytesIO()
        # write a valid bitstream header
        header = [database.DESCRIPTOR_BLOCK_MAGIC_NUM,
                  128,
                  (database.BITSTREAM_TYPE_RK_256 << 8) | database.CONTENT_BMC,
                  0]
        bio.write(struct.pack('<IIII', *header))
        bio.write(m256.digest())
        bio.write(m384.digest())
        bio.write(bytearray(32))
        return bio.getbuffer()

    def create_block1(self):
        bio = io.BytesIO()
        bio.write(
            struct.pack('<I', database.SIGNATURE_BLOCK_MAGIC_NUM))
        with open('/dev/null', 'rb') as null:
            bio.write(null.read(896))
        return bio.getbuffer()

    @classmethod
    def create(cls, content_type, size=1024):
        # put random bytes in a BytesIO object
        random_buffer = io.BytesIO()
        with open('/dev/urandom', 'rb') as tmp:
            random_buffer.write(tmp.read(size))
        payload = random_buffer.getbuffer()
        bs = bitstream(payload)
        return bs

    def getbuffer(self):
        return self.bytes_io.getbuffer()
