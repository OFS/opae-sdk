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
import ecdsa
import hashlib
import io
import random
import tempfile

from unittest import mock

from pacsign import database


class bitstream(object):
    _c2p = {
        database.CONTENT_BMC: database.SIGN_BMC,
        database.CONTENT_PR: database.SIGN_PR,
        database.CONTENT_SR: database.SIGN_SR
    }

    def __init__(self, content_type, payload):
        self.content_type = content_type
        self.payload = payload
        self.sign()
        self.block0 = self.create_block0()
        self.block1 = self.create_block1()
        self.write_bytes()

    def c2p(self):
        return self._c2p.get(self.content_type)

    def sign(self):
        self.sk = ecdsa.SigningKey.generate(ecdsa.SECP256k1)
        self.vk = self.sk.verifying_key
        self.pk = self.vk.pubkey
        self.signature = self.sk.sign(self.payload)


    def write_bytes(self):
        self.bytes_io = io.BytesIO()
        self.bytes_io.write(self.block0)
        self.bytes_io.write(self.block1)
        self.bytes_io.write(self.payload)


    def create_block0(self):
        m256 = hashlib.sha256(self.payload)
        m384 = hashlib.sha384(self.payload)
        m256le = m256.digest()
        m384le = m384.digest()
        self.m256be = int.from_bytes(
            m256le, 'little').to_bytes(len(m256le), 'big')
        self.m384be = int.from_bytes(
            m384le, 'little').to_bytes(len(m384le), 'big')
        bio = io.BytesIO()
        # write a valid bitstream header
        header = [database.DESCRIPTOR_BLOCK_MAGIC_NUM,
                  128,
                  (database.BITSTREAM_TYPE_RK_256 << 8) | self.content_type,
                  0]
        for h in header:
            bio.write(h.to_bytes(4, 'little'))
        bio.write(self.m256be)
        bio.write(self.m384be)
        bio.write(bytearray(16))
        bio.write(int(3).to_bytes(4, 'little'))
        bio.write(bytearray(12))
        return bio.getbuffer()

    def create_block1(self):
        bio = io.BytesIO()
        bio.write(database.SIGNATURE_BLOCK_MAGIC_NUM.to_bytes(4, 'little'))
        bio.write(bytearray(12))
        # begin signature chain
        # root entry
        for v in [0xA757A046, 0xC7B88C74, 0xFFFFFFFF, 1 << 31]:
            bio.write(int(v).to_bytes(4, 'little'))
        bio.write(self.pk.point.x().to_bytes(48, 'big'))
        bio.write(self.pk.point.y().to_bytes(48, 'big'))
        bio.write(bytearray(20))

        csk_begin = bio.tell()
        # csk entry
        for v in [0x14711C2F, 0xC7B88C74, self.c2p(), 0]:
            bio.write(int(v).to_bytes(4, 'little'))
        bio.write(self.pk.point.x().to_bytes(48, 'big'))
        bio.write(self.pk.point.y().to_bytes(48, 'big'))
        bio.write(bytearray(20))
        bio.write(int(0xDE64437D).to_bytes(4, 'little'))
        bio.write(self.signature[1:49]) # should be R
        bio.write(self.signature[2:50]) # should be S
        csk_end = bio.tell()

        # block 0 entry
        for v in [0x15364367, 0xDE64437D]:
            bio.write(int(v).to_bytes(4, 'little'))

        bio.write(self.signature[1:49]) # should be R
        bio.write(self.signature[2:50]) # should be S
        b0entry_end = bio.tell()

        bio.write(bytearray(896-bio.tell()))
        data =  bio.getbuffer()
        self.csk = data[csk_begin:csk_end]
        self.b0entry = data[csk_end:b0entry_end]

        return data

    @classmethod
    def create(cls, content_type, size=1024):
        # put random bytes in a BytesIO object
        random_buffer = io.BytesIO()
        with open('/dev/urandom', 'rb') as tmp:
            random_buffer.write(tmp.read(size))

        if content_type == database.CONTENT_PR:
            random_buffer.seek(0)
            random_buffer.write(database.DC_PLATFORM_NUM.to_bytes(4, 'little'))
            random_buffer.seek(12)
            random_buffer.write(database.PR_IDENTIFIER.to_bytes(4, 'little'))

        payload = random_buffer.getbuffer()
        bs = cls(content_type, payload)
        return bs

    def getbuffer(self):
        return self.bytes_io.getbuffer()


class byte_array(object):
    def __init__(self, data):
        self.data = data

    def get_dword(self, offset):
        return int.from_bytes(self.data[offset:offset+4], 'little')

    def get_word(self, offset):
        return int.from_bytes(self.data[offset:offset+2], 'little')

def args(**kwargs):
    m = mock.MagicMock()
    for k, v in kwargs.items():
        setattr(m, k, v)
    return m

