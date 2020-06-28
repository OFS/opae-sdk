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

from unittest import mock

from pacsign import database


class bitstream(io.BytesIO):
    _c2p = {
        database.CONTENT_BMC: database.SIGN_BMC,
        database.CONTENT_PR: database.SIGN_PR,
        database.CONTENT_SR: database.SIGN_SR
    }

    def __init__(self, content_type, payload):
        super(bitstream, self).__init__()
        self.sig_block_offset = 96
        self.content_type = content_type
        self.payload = self.insert_header(payload)
        self.sign()
        self.block0 = self.get_bytes(self.write_block0)
        self.block1 = self.get_bytes(self.write_block1)
        self.payload_offset = self.tell()
        self.write(self.payload)

    def c2p(self):
        return self._c2p.get(self.content_type)

    @property
    def data(self):
        return self.getbuffer()

    def sign(self):
        self.sk = ecdsa.SigningKey.generate(ecdsa.SECP256k1)
        self.vk = self.sk.verifying_key
        self.pk = self.vk.pubkey
        self.signature = self.sk.sign(self.payload)

    def get_bytes(self, fn, *args, **kwargs):
        b = self.tell()
        fn(*args, **kwargs)
        data = self.getvalue()[b:self.tell()]
        return data

    def write_block0(self):
        m256 = hashlib.sha256(self.payload)
        m384 = hashlib.sha384(self.payload)
        m256le = m256.digest()
        m384le = m384.digest()
        self.m256be = int.from_bytes(
            m256le, 'little').to_bytes(len(m256le), 'big')
        self.m384be = int.from_bytes(
            m384le, 'little').to_bytes(len(m384le), 'big')
        # write a valid bitstream header

        ctype = (database.BITSTREAM_TYPE_RK_256 << 8) | self.content_type
        self.write_int32(database.DESCRIPTOR_BLOCK_MAGIC_NUM,
                         128,
                         ctype,
                         0)
        self.write(self.m256be)
        self.write(self.m384be)
        self.write(bytearray(16))
        # Not in the doc? Or need latest doc
        self.write_int32(3)
        self.write(self.sig_block_offset.to_bytes(4, 'little'))
        self.write(bytearray(8))

    def write_root_entry(self):
        for v in [0xA757A046, 0xC7B88C74, 0xFFFFFFFF, 1 << 31]:
            self.write(int(v).to_bytes(4, 'little'))
        self.write(self.pk.point.x().to_bytes(48, 'big'))
        self.write(self.pk.point.y().to_bytes(48, 'big'))
        self.write(bytearray(20))

    def write_csk_entry(self):
        # csk entry
        for v in [0x14711C2F, 0xC7B88C74, self.c2p(), 0]:
            self.write(int(v).to_bytes(4, 'little'))
        self.write(self.pk.point.x().to_bytes(48, 'big'))
        self.write(self.pk.point.y().to_bytes(48, 'big'))
        self.write(bytearray(20))
        self.write(int(0xDE64437D).to_bytes(4, 'little'))
        self.write(self.signature[1:49])  # should be R
        self.write(self.signature[2:50])  # should be S

    def write_b0_entry(self):
        for v in [0x15364367, 0xDE64437D]:
            self.write(int(v).to_bytes(4, 'little'))

        self.write(self.signature[1:49])  # should be R
        self.write(self.signature[2:50])  # should be S

    def write_b1_header(self):
        self.write(database.SIGNATURE_BLOCK_MAGIC_NUM.to_bytes(4, 'little'))
        self.write(bytearray(12))

    def write_block1(self):
        begin = self.tell()
        self.write_b1_header()
        # begin signature chain
        # root entry
        self.root_entry = self.get_bytes(self.write_root_entry)
        # csk entry
        self.csk_entry = self.get_bytes(self.write_csk_entry)
        # block 0 entry
        self.b0_entry = self.get_bytes(self.write_b0_entry)
        # padding
        self.write(bytearray(896-(self.tell()-begin)))

    @classmethod
    def create(cls, content_type, size=1024):
        # put random bytes in a BytesIO object
        random_buffer = io.BytesIO()
        random_buffer.write(bytearray(size))
        payload = random_buffer.getbuffer()
        bs = cls(content_type, payload)
        return bs

    def insert_header(self, data):
        return data

    def write_int32(self, *args, **kwargs):
        endian = kwargs.get('endian', 'little')
        for v in args:
            self.write(int(v).to_bytes(4, endian))

    def write_int64(self, *args, **kwargs):
        endian = kwargs.get('endian', 'little')
        for v in args:
            self.write(int(v).to_bytes(8, endian))


class d5005_pr(bitstream):

    @classmethod
    def create(cls, content_type=database.CONTENT_PR, size=1024):
        return super(d5005_pr, cls).create(database.CONTENT_PR, size)

    def insert_header(self, data):
        bio = io.BytesIO()
        bio.write(database.DC_PLATFORM_NUM.to_bytes(4, 'little'))
        bio.write(bytearray(8))
        bio.write(database.PR_IDENTIFIER.to_bytes(4, 'little'))
        bio.write(data[bio.tell():])
        return bio.getbuffer()

    def write_block0(self):
        for i in range(0, 0x1000, 4):
            self.write(int(i).to_bytes(4, 'little'))

    def write_b1_header(self):
        sha384 = hashlib.sha384(self.block0)
        digest = sha384.digest()
        self.write(digest)
        self.write(bytearray(64-len(digest)))
        # number of entries, root, csk, b0
        self.write(int(3).to_bytes(4, 'little'))
        # offset to first entry in signature block
        self.write(int(0x60).to_bytes(4, 'little'))
        self.write_int64(0, 0, 0)

    def write_root_entry(self):
        data = self.pk.point.x().to_bytes(32, 'big')
        data += self.pk.point.y().to_bytes(32, 'big')
        self.write_int32(database.DC_ROOT_ENTRY_MAGIC,
                         0x78, 0x60, 0, 0, 1)

        chksum = hashlib.sha256(data).digest()
        self.write(chksum[0:4])

        self.write_int32(0, database.DC_XY_KEY_MAGIC,
                         32, 32,
                         )
        cinfo = database.get_curve_info_from_name('secp256r1')
        self.write_int32(cinfo.dc_curve_magic_num,
                         0xFFFFFFFF, 0xFFFFFFFF)
        self.write(data)

    def write_csk_entry(self):
        cinfo = database.get_curve_info_from_name('secp256r1')
        self.write_int32(database.DC_CSK_MAGIC_NUM,
                         0xc0,
                         0x58,
                         0x50,
                         0,
                         0)
        # csk body
        self.write_int32(database.DC_XY_KEY_MAGIC,
                         0x20,
                         0x20,
                         cinfo.dc_curve_magic_num,
                         0x2,
                         0)  # pub csk id

        self.write(self.pk.point.x().to_bytes(32, 'big'))
        self.write(self.pk.point.y().to_bytes(32, 'big'))
        self.write_int32(database.DC_SIGNATURE_MAGIC_NUM,
                         0x20,
                         0x20,
                         cinfo.dc_sig_hash_magic_num)
        # sign csk body sha256 but use 0 for now
        self.write(bytearray(64))

    def write_b0_entry(self):
        begin = self.tell()
        cinfo = database.get_curve_info_from_name('secp256r1')
        self.write_int32(database.BLOCK0_MAGIC_NUM,
                         0x68, 0, 0x50, 0, 0,
                         database.DC_SIGNATURE_MAGIC_NUM,
                         0x20, 0x20, cinfo.dc_sig_hash_magic_num)
        b0_entry_size = self.tell() - begin

        # Signature R & S
        # or zeroes
        self.write(bytearray(0x68-b0_entry_size))


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
