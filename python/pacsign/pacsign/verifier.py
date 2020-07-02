# Copyright(c) 2019, Intel Corporation
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

##########################
#
# All about bitstream/data handling
#
# Base class is BITSTREAM_READER, which provide common API likely to be shared
#     by different family (PAC_CARD is considered as family in this context)
#
#   PAC_CARD_READER is inherits from BITSTREAM_READER, which provides
#     customized functions on how to generate Block0 and Block1 according
#     to PAC spec
#
#   In the future if we need to support more families, just create a class
#     that inherits from base and customize this family flow
#       You need to define the database of this family in database.py
#       You also need to update openssl.py for the curve that is supported
#          by this family
#
##########################
from hashlib import sha256, sha384
import json

from pacsign.logger import log
from pacsign import common_util
from pacsign import ecdsa
from pacsign import database

METADATA_GUID = "XeonFPGA" + chr(0xB7) + "GBSv001"

# Metadata length field is a unsigned 32 bit int
SIZEOF_LEN_FIELD = 4

# Length of GUID string
GUID_LEN = len(METADATA_GUID)


class _VERIFIER_BASE(object):
    def __init__(self, args):
        self.args = args
        self.database = database.FAMILY_LIST["PAC_CARD"]
        log.debug(args.main_command)
        self.bitstream_type = self.database.SUPPORTED_TYPES[args.main_command].ENUM
        self.verify = True if args.root_bitstream else False
        if self.verify:
            offset = 0
            self.reh = common_util.BYTE_ARRAY("FILE", args.root_bitstream)
            con_type = self.reh.get_dword(offset + 8)

            if (
                self.reh.get_dword(offset) != database.DESCRIPTOR_BLOCK_MAGIC_NUM
                or self.reh.get_dword(offset + 4) != 128
                or (con_type >> 8) & 0xFF != database.BITSTREAM_TYPE_RK_256
                or self.reh.get_dword(offset + 128)
                != database.SIGNATURE_BLOCK_MAGIC_NUM
            ):
                common_util.assert_in_error(
                    False,
                    "File '{}' is not a root entry hash programming bitstream".format(
                        args.root_bitstream
                    ),
                )

            if con_type & 0xFF != self.bitstream_type:
                log.warn(
                    "File '{}' different type than bitstream."
                    "  Validation impossible".format(args.root_bitstream)
                )
                self.verify = False

        if self.verify:
            self.val_root_hash = int.from_bytes(
                self.reh.data[1024:1056], byteorder="big"
            )
            self.val_root_hash_dc = int.from_bytes(
                self.reh.data[1072:1104], byteorder="big"
            )

    def is_Darby_PR(self, contents, offset):
        # TODO: Write function to read dword and determine RC or VC
        val = contents.get_dword(offset)
        log.debug("platform value is '{}' ".format(hex(val)))
        type = contents.get_word(offset + int(0xC))
        return val == database.DC_PLATFORM_NUM and type == database.PR_IDENTIFIER


class verify_reader(_VERIFIER_BASE):
    def __init__(self, args):
        super(verify_reader, self).__init__(args)
        self.cert_type = self.database.SUPPORTED_CERT_TYPES[args.cert_type]

    def run(self, fname, file_offset, block0, block1, payload):
        log.info("Starting verification")
        payload_content = common_util.BYTE_ARRAY("FILE", fname)

        # Skip JSON and old signature if they exist
        has_json = self.is_JSON(payload_content)
        log.debug("has_json = {}".format(has_json))
        sig_offset = 0 if not has_json else self.skip_JSON(payload_content)

        # Determine if platform is RC or DC
        self.dc_pr = self.is_Darby_PR(payload_content, sig_offset)

        fd = open(fname, "rb")
        fd.seek(file_offset, 0)

        b0 = fd.read(block0.size())
        b1 = fd.read(block1.size())
        if self.dc_pr:
            pay = fd.read(len(payload))
            pay_good = pay == bytes(payload)
        else:
            pay = fd.read(payload.size())
            pay_good = pay == bytes(payload.data)

        fd.close()
        common_util.assert_in_error(
            b0 == bytes(block0.data) and b1 == bytes(block1.data) and pay_good,
            "File not written properly",
        )

        log.info("Bitstream file written properly (bits match expected)")

        pay_sha256_v = int.from_bytes(sha256(pay).digest(), byteorder="big")
        pay_sha384_v = int.from_bytes(sha384(pay).digest(), byteorder="big")

        if not self.dc_pr:
            if pay_sha256_v != int.from_bytes(b0[16 : 16 + 32], byteorder="big"):
                log.error("SHA-256 in Block 0 does not match")

            if pay_sha384_v != int.from_bytes(b0[48 : 48 + 48], byteorder="big"):
                log.error("SHA-384 in Block 0 does not match")

        # Check root hash
        if self.dc_pr:
            hash = int.from_bytes(sha256(b1[152 : 152 + 64]).digest(), byteorder="big")
            good = hash == self.val_root_hash_dc
        else:
            hash = int.from_bytes(sha256(b1[20:148]).digest(), byteorder="big")
            good = hash == self.val_root_hash

        if good:
            log.info("Root hash matches")
        else:
            log.error("Root hash mismatch:")
            log.error("Signed bitstream:                   {}".format(hex(hash)))
            log.error(
                "Root entry hash bitstream provided: {}".format(
                    hex(self.val_root_hash_dc if self.dc_pr else self.val_root_hash)
                )
            )

        if self.dc_pr:
            root_pub_key_x = int.from_bytes(b1[152 : 152 + 32], byteorder="big")
            root_pub_key_y = int.from_bytes(b1[152 + 32 : 152 + 64], byteorder="big")
            root_pub_key_v = (root_pub_key_x, root_pub_key_y)
            csk_pub_key_x = int.from_bytes(b1[264 : 264 + 32], byteorder="big")
            csk_pub_key_y = int.from_bytes(b1[264 + 32 : 264 + 64], byteorder="big")
            csk_pub_key_v = (csk_pub_key_x, csk_pub_key_y)
            csk_rs_r = int.from_bytes(b1[344 : 344 + 32], byteorder="big")
            csk_rs_s = int.from_bytes(b1[344 + 32 : 344 + 64], byteorder="big")
            csk_rs_v = (csk_rs_r, csk_rs_s)
            b0e_rs_r = int.from_bytes(b1[448 : 448 + 32], byteorder="big")
            b0e_rs_s = int.from_bytes(b1[448 + 32 : 448 + 64], byteorder="big")
            b0e_rs_v = (b0e_rs_r, b0e_rs_s)

            root_sha_v = hash
            csk_sha_v = int.from_bytes(
                sha256(b1[240 : 240 + 88]).digest(), byteorder="big"
            )
            b0_sha_v = int.from_bytes(sha256(b0).digest(), byteorder="big")
        else:
            if self.cert_type == database.BITSTREAM_TYPE_CANCEL:
                b0e_off = 156
                csk_pub_key_v = (0, 0)
                csk_rs_v = (0, 0)
                csk_sha_v = 0
            else:
                b0e_off = 388
                csk_pub_key_x = int.from_bytes(b1[164 : 164 + 32], byteorder="big")
                csk_pub_key_y = int.from_bytes(b1[212 : 212 + 32], byteorder="big")
                csk_pub_key_v = (csk_pub_key_x, csk_pub_key_y)
                csk_rs_r = int.from_bytes(b1[284 : 284 + 32], byteorder="big")
                csk_rs_s = int.from_bytes(b1[332 : 332 + 32], byteorder="big")
                csk_rs_v = (csk_rs_r, csk_rs_s)
                csk_sha_v = int.from_bytes(
                    sha256(b1[152 : 152 + 128]).digest(), byteorder="big"
                )

            root_pub_key_x = int.from_bytes(b1[32 : 32 + 32], byteorder="big")
            root_pub_key_y = int.from_bytes(b1[80 : 80 + 32], byteorder="big")
            root_pub_key_v = (root_pub_key_x, root_pub_key_y)
            b0e_rs_r = int.from_bytes(b1[b0e_off : b0e_off + 32], byteorder="big")
            b0e_rs_s = int.from_bytes(
                b1[b0e_off + 48 : b0e_off + 48 + 32], byteorder="big"
            )
            b0e_rs_v = (b0e_rs_r, b0e_rs_s)
            root_sha_v = hash
            b0_sha_v = int.from_bytes(sha256(b0).digest(), byteorder="big")

        # validate signatures
        if self.cert_type != database.BITSTREAM_TYPE_CANCEL:
            if ecdsa.verify_signature(root_pub_key_v, csk_sha_v, csk_rs_v):
                log.info("Signature of CSK with root key OK")
            else:
                log.error("Signature of CSK with root key mismatch")

            if ecdsa.verify_signature(csk_pub_key_v, b0_sha_v, b0e_rs_v):
                log.info("Signature of Block 0 with CSK OK")
            else:
                log.error("Signature of Block 0 with CSK mismatch")
        else:
            if ecdsa.verify_signature(root_pub_key_v, b0_sha_v, b0e_rs_v):
                log.info("Signature of Block 0 with root key OK")
            else:
                log.error("Signature of Block 0 with root key mismatch")
        return

    def is_JSON(self, contents):
        log.debug(
            "GUID: {} vs. file {}".format(
                METADATA_GUID, "".join([chr(i) for i in contents.data[:GUID_LEN]])
            )
        )
        return METADATA_GUID == "".join([chr(i) for i in contents.data[:GUID_LEN]])

    def skip_JSON(self, contents):
        leng = contents.get_dword(GUID_LEN)
        log.debug("length of json={}".format(leng))
        return leng + GUID_LEN + SIZEOF_LEN_FIELD


class print_bitstream(_VERIFIER_BASE):
    def __init__(self, args, b0, b1, payload, json_str=None):
        super(print_bitstream, self).__init__(args)
        try:
            _ = payload[0]
        except TypeError:
            payload = payload.data
        self.cert_type = self.database.SUPPORTED_CERT_TYPES[args.cert_type]
        val = b0.get_dword(0)
        log.debug("platform value is '{}' ".format(hex(val)))
        type = b0.get_word(int(0xC))
        self.dc_pr = val == database.DC_PLATFORM_NUM and type == database.PR_IDENTIFIER
        if not self.dc_pr:
            self.b0 = Block_0(b0.data, payload)
            self.b1 = Block_1(b1.data, self.b0)
        else:
            self.b0 = Block_0_dc(b0.data, payload)
            self.b1 = Block_1_dc(b1.data, self.b0)

        self.print_json(json_str)

        self.b0.print_block()
        self.b1.print_block()
        self.print_payload(self.b0, payload)

    def print_json(self, json_str):
        if not json_str:
            return

        jstr = json.loads(json_str)
        print("JSON:\n")
        print(json.dumps(jstr, sort_keys=True, indent=4))
        print("")

    def print_payload(self, b0, bits):
        print("Payload:")
        off = 0

        for xx in range(8):
            print("\t", end="")
            try:
                print("".join("{:02x} ".format(x) for x in bits[off : off + 16]))
            except TypeError:
                print("".join("{:02x} ".format(x) for x in bits.data[off : off + 16]))
            off += 16

        if b0.content_len == 128:
            return

        print("\n\t...\n\t", end="")
        try:
            print("".join("{:02x} ".format(x) for x in bits[-16:]))
        except TypeError:
            print("".join("{:02x} ".format(x) for x in bits.data[-16:]))


class Block_0:
    def __init__(self, bits, payload):
        self.is_good = False
        self.content_len = None
        self.content_type = None
        self.cert_type = None
        self.sha256 = None
        self.sha384 = None
        self.hash = None
        self.magic = int.from_bytes(bits[0:4], byteorder="little")
        if self.magic != database.DESCRIPTOR_BLOCK_MAGIC_NUM:
            return
        self.is_good = True
        self.content_len = int.from_bytes(bits[4:8], byteorder="little")
        self.content_type = bits[8]
        self.cert_type = bits[9]
        self.sha256 = int.from_bytes(bits[16:48], byteorder="big")
        self.sha384 = int.from_bytes(bits[48:96], byteorder="big")
        self.calc_sha256 = int.from_bytes(sha256(payload).digest(), byteorder="big")
        self.calc_sha384 = int.from_bytes(sha384(payload).digest(), byteorder="big")

        self.hash = int.from_bytes(sha256(bits).digest(), byteorder="big")

    def print_block(self):
        if not self.is_good:
            print("No block 0")
            return
        print("Block 0:")
        print("\tBlock 0 magic =\t\t{0:#0{1}x}".format(self.magic, 10))
        print("\tContent length =\t{0:#0{1}x}".format(self.content_len, 10))
        con = self.content_type
        print("\tContent type =\t\t{}".format(["SR", "BMC", "PR"][con & 0xFF]))
        print(
            "\tCert type =\t\t{}".format(
                ["UPDATE", "CANCEL", "Root Entry Hash (256)", "Root Entry Hash (384)"][
                    self.cert_type
                ]
            )
        )
        print("\tProtected content SHA-256: \n\t\t\t{0:#0{1}x}".format(self.sha256, 66))
        print(
            "\tCalculated protected content SHA-256: \n\t\t\t{0:#0{1}x}".format(
                self.calc_sha256, 66
            )
        )
        print("\t\tMatch" if self.sha256 == self.calc_sha256 else "\t\tNo match")

        print("\tProtected content SHA-384: \n\t\t\t{0:#0{1}x}".format(self.sha384, 98))
        print(
            "\tCalculated protected content SHA-384: \n\t\t\t{0:#0{1}x}".format(
                self.calc_sha384, 98
            )
        )
        print("\t\tMatch" if self.sha384 == self.calc_sha384 else "\t\tNo match")


class Block_0_dc:
    def __init__(self, bits, payload):
        self.is_good = False
        self.content_len = None
        self.content_type = None
        self.cert_type = None
        self.sha256 = None
        self.sha384 = None
        self.hash = None
        self.magic = int.from_bytes(bits[0:4], byteorder="little")
        self.is_good = True
        self.content_len = int.from_bytes(bits[8:12], byteorder="little")
        self.content_type = chr(bits[12]) + chr(bits[13])
        self.cert_type = "UPDATE"
        self.hash = int.from_bytes(sha256(bits).digest(), byteorder="big")
        self.sha384 = int.from_bytes(sha384(bits).digest(), byteorder="big")

    def print_block(self):
        if not self.is_good:
            print("No block 0")
            return
        print("Block 0:")
        print("\tBlock 0 magic =\t\t{0:#0{1}x}".format(self.magic, 10))
        print("\tContent length =\t{0:#0{1}x}".format(self.content_len, 10))
        print("\tContent type =\t\t{}".format(self.content_type))
        print("\tCert type =\t\t{}".format(self.cert_type))


class Block_1:
    def __init__(self, bits, b0):
        self.is_good = False
        self.root_entry = None
        self.csk = None
        self.b0_entry = None
        self.magic = int.from_bytes(bits[0:4], byteorder="little")
        if self.magic != database.SIGNATURE_BLOCK_MAGIC_NUM:
            return
        self.is_good = True
        self.root_entry = Root_Entry(bits[16:148])
        self.csk = CSK(bits[148:380], self.root_entry)
        off = 148
        self.csk_offset = 148
        if b0.cert_type != 1:
            off = 380
        self.b0e_offset = off
        self.b0_entry = Block_0_Entry(bits[off : off + 232])

    def print_block(self):
        if not self.is_good:
            print("No block 1")
            return
        print("Block 1:")
        print("\tBlock 1 magic =\t{0:#0{1}x}".format(self.magic, 10))
        self.root_entry.print_block()
        self.csk.print_block()
        self.b0_entry.print_block()


class Root_Entry:
    def __init__(self, bits):
        self.is_good = False
        self.curve_magic = None
        self.x = None
        self.y = None
        self.hash = None
        self.dc_hash = None
        self.magic = int.from_bytes(bits[0:4], byteorder="little")
        if self.magic != database.ROOT_ENTRY_MAGIC_NUM:
            return
        self.is_good = True
        self.curve_magic = int.from_bytes(bits[4:8], byteorder="little")
        self.permissions = int.from_bytes(bits[8:12], byteorder="little")
        self.key_id = int.from_bytes(bits[12:16], byteorder="little")
        self.x = int.from_bytes(bits[16:48], byteorder="big")
        self.y = int.from_bytes(bits[64:96], byteorder="big")
        self.hash = int.from_bytes(sha256(bits[4:132]).digest(), byteorder="big")

    def print_block(self):
        if not self.is_good:
            print("\tNo root entry")
            return
        print("\t\tRoot Entry magic =\t\t{0:#0{1}x}".format(self.magic, 10))
        print("\t\tRoot Entry curve magic =\t{0:#0{1}x}".format(self.curve_magic, 10))
        print("\t\tRoot Entry permissions =\t{0:#0{1}x}".format(self.permissions, 10))
        print("\t\tRoot Entry key ID =\t\t{0:#0{1}x}".format(self.key_id, 10))
        print("\t\tRoot public key X =\t\t{0:#0{1}x}".format(self.x, 66))
        print("\t\tRoot public key Y =\t\t{0:#0{1}x}".format(self.y, 66))
        print("\n\t\tExpected root entry hash =\t{0:#0{1}x}".format(self.hash, 66))


class CSK:
    def __init__(self, bits, root):
        self.is_good = False
        self.curve_magic = None
        self.permissions = None
        self.key_id = None
        self.x = None
        self.y = None
        self.hash = None
        self.sig_magic = None
        self.r = None
        self.s = None
        self.magic = int.from_bytes(bits[0:4], byteorder="little")
        if self.magic != database.CODE_SIGNING_KEY_ENTRY_MAGIC_NUM:
            return
        self.curve_magic = int.from_bytes(bits[4:8], byteorder="little")
        if self.curve_magic != root.curve_magic:
            return
        self.is_good = True
        self.permissions = int.from_bytes(bits[8:12], byteorder="little")
        self.key_id = int.from_bytes(bits[12:16], byteorder="little")
        self.x = int.from_bytes(bits[16:48], byteorder="big")
        self.y = int.from_bytes(bits[64:96], byteorder="big")
        self.hash = int.from_bytes(sha256(bits[4:132]).digest(), byteorder="big")
        self.sig_magic = int.from_bytes(bits[132:136], byteorder="little")
        self.r = int.from_bytes(bits[136:168], byteorder="big")
        self.s = int.from_bytes(bits[184:216], byteorder="big")

    def print_block(self):
        if not self.is_good:
            print("\tNo CSK")
            return
        print("\n\t\tCSK magic =\t\t\t{0:#0{1}x}".format(self.magic, 10))
        print("\t\tCSK curve magic =\t\t{0:#0{1}x}".format(self.curve_magic, 10))
        print("\t\tCSK permissions =\t\t{0:#0{1}x}".format(self.permissions, 10))
        print("\t\tCSK key ID =\t\t\t{0:#0{1}x}".format(self.key_id, 10))
        print("\t\tCode signing key X =\t\t{0:#0{1}x}".format(self.x, 66))
        print("\t\tCode signing key Y =\t\t{0:#0{1}x}".format(self.y, 66))
        print("\t\tCSK signature magic =\t\t{0:#0{1}x}".format(self.sig_magic, 10))
        print("\t\tSignature R =\t\t\t{0:#0{1}x}".format(self.r, 66))
        print("\t\tSignature S =\t\t\t{0:#0{1}x}".format(self.s, 66))
        print("\n\t\tExpected CSK hash =\t\t{0:#0{1}x}".format(self.hash, 66))


class Block_0_Entry:
    def __init__(self, bits):
        self.is_good = False
        self.sig_magic = None
        self.r = None
        self.s = None
        self.magic = int.from_bytes(bits[0:4], byteorder="little")
        if self.magic != database.BLOCK0_MAGIC_NUM:
            return
        self.sig_magic = int.from_bytes(bits[4:8], byteorder="little")
        if self.sig_magic != 0xDE64437D:
            return
        self.is_good = True
        self.r = int.from_bytes(bits[8:40], byteorder="big")
        self.s = int.from_bytes(bits[56:88], byteorder="big")

    def print_block(self):
        if not self.is_good:
            print("\tNo block 0 entry")
            return
        print("\n\t\tBlock 0 Entry magic =\t\t{0:#0{1}x}".format(self.magic, 10))
        print(
            "\t\tBlock 0 Entry signature magic = {0:#0{1}x}".format(self.sig_magic, 10)
        )
        print("\t\tSignature R =\t\t\t{0:#0{1}x}".format(self.r, 66))
        print("\t\tSignature S =\t\t\t{0:#0{1}x}".format(self.s, 66))


class Block_1_dc:
    def __init__(self, bits, b0):
        self.is_good = False
        self.root_entry = None
        self.csk = None
        self.b0_entry = None
        sha384 = int.from_bytes(bits[0:48], byteorder="big")
        if sha384 != b0.sha384:
            print("SHA-384 mismatch")
            return
        self.is_good = True
        self.sig_entries = []
        self.num_sigs = int.from_bytes(bits[64:68], byteorder="little")
        sig_blk_offset = int.from_bytes(bits[68:72], byteorder="little")
        if self.num_sigs <= 0:
            return

        magic = int.from_bytes(
            bits[sig_blk_offset : sig_blk_offset + 4], byteorder="little"
        )
        if magic != database.DC_ROOT_ENTRY_MAGIC:
            self.is_good = False
            print("Can't find root entry")
            return

        siz = int.from_bytes(
            bits[sig_blk_offset + 4 : sig_blk_offset + 8], byteorder="little"
        )

        self.sig_entries.append(
            DC_Root_Entry(bits[sig_blk_offset : sig_blk_offset + siz])
        )
        sig_blk_offset += siz

        magic = int.from_bytes(
            bits[sig_blk_offset : sig_blk_offset + 4], byteorder="little"
        )
        if magic != database.DC_CSK_MAGIC_NUM:
            self.is_good = False
            print("Can't find public key entry")
            return

        siz = int.from_bytes(
            bits[sig_blk_offset + 4 : sig_blk_offset + 8], byteorder="little"
        )

        self.sig_entries.append(
            DC_CSK_Entry(bits[sig_blk_offset : sig_blk_offset + siz])
        )
        sig_blk_offset += siz

        magic = int.from_bytes(
            bits[sig_blk_offset : sig_blk_offset + 4], byteorder="little"
        )
        if magic != database.BLOCK0_MAGIC_NUM:
            self.is_good = False
            print("Can't find public key entry")
            return

        siz = int.from_bytes(
            bits[sig_blk_offset + 4 : sig_blk_offset + 8], byteorder="little"
        )

        self.sig_entries.append(
            DC_B0_Entry(bits[sig_blk_offset : sig_blk_offset + siz])
        )

    def print_block(self):
        if not self.is_good:
            print("No block 1")
            return
        if self.num_sigs <= 0:
            print("No signature chains present")
            return
        print("Block 1:")
        print("\tNumber of signature chains =\t{}".format(self.num_sigs))

        for blk in self.sig_entries:
            blk.print_block()


class DC_Root_Entry:
    def __init__(self, bits):
        self.is_good = False
        self.size = int.from_bytes(bits[4:8], byteorder="little")
        self.dlen = int.from_bytes(bits[8:12], byteorder="little")
        self.rhs = int.from_bytes(bits[20:24], byteorder="little")
        self.msw_hash = int.from_bytes(bits[24:28], byteorder="big")
        self.pk_magic = int.from_bytes(bits[32:36], byteorder="little")
        self.x_size = int.from_bytes(bits[36:40], byteorder="little")
        self.y_size = int.from_bytes(bits[40:44], byteorder="little")
        self.pk_curve_magic = int.from_bytes(bits[44:48], byteorder="little")
        self.permissions = int.from_bytes(bits[48:52], byteorder="little")
        self.key_id = int.from_bytes(bits[52:56], byteorder="little")
        self.key_x = int.from_bytes(bits[56:88], byteorder="big")
        self.key_y = int.from_bytes(bits[88:120], byteorder="big")
        hash = sha256(bits[56:120]).digest()
        self.hash = int.from_bytes(hash, byteorder="big")
        self.calc_hash_msw = int.from_bytes(hash[0:4], byteorder="big")

        if (
            self.msw_hash == self.calc_hash_msw
            and self.permissions == 0xFFFFFFFF
            and self.key_id == 0xFFFFFFFF
            and self.pk_magic == database.DC_XY_KEY_MAGIC
            and self.pk_curve_magic == 0x21339360
        ):
            self.is_good = True

    def print_block(self):
        if not self.is_good:
            print("No root entry")
            return
        print("\tRoot entry:")
        print(
            "\t\tRoot hash select =\t\t{}".format(
                ["Intel", "User", "Engineering", "Manufacturing", "Upgrade", "Slot"][
                    self.rhs
                ]
            )
        )
        print("\t\tPublic key magic =\t\t{0:#0{1}x}".format(self.pk_magic, 10))
        print(
            "\t\tPublic key curve magic =\t{0:#0{1}x}".format(self.pk_curve_magic, 10)
        )
        print("\t\tPublic key X size =\t\t{}".format(self.x_size))
        print("\t\tPublic key Y size =\t\t{}".format(self.y_size))
        print("\t\tPublic key permissions =\t{0:#0{1}x}".format(self.permissions, 10))
        print("\t\tPublic key id =\t\t\t{0:#0{1}x}".format(self.key_id, 10))
        print("\t\tPublic key X =\t\t\t{0:#0{1}x}".format(self.key_x, 66))
        print("\t\tPublic key Y =\t\t\t{0:#0{1}x}".format(self.key_y, 66))
        print("\n\t\tExpected root key hash =\t{0:#0{1}x}".format(self.hash, 66))
        print("\t\tRoot key hash MSW =\t\t{0:#0{1}x}".format(self.calc_hash_msw, 10))


class DC_CSK_Entry:
    def __init__(self, bits):
        self.is_good = False
        self.magic = int.from_bytes(bits[0:4], byteorder="little")
        self.size = int.from_bytes(bits[4:8], byteorder="little")
        self.dlen = int.from_bytes(bits[8:12], byteorder="little")
        self.pk_magic = int.from_bytes(bits[24:28], byteorder="little")
        self.x_size = int.from_bytes(bits[28:32], byteorder="little")
        self.y_size = int.from_bytes(bits[32:36], byteorder="little")
        self.pk_curve_magic = int.from_bytes(bits[36:40], byteorder="little")
        self.permissions = int.from_bytes(bits[40:44], byteorder="little")
        self.key_id = int.from_bytes(bits[44:48], byteorder="little")
        self.key_x = int.from_bytes(bits[48:80], byteorder="big")
        self.key_y = int.from_bytes(bits[80:112], byteorder="big")
        hash = sha256(bits[24:112]).digest()
        self.hash = int.from_bytes(hash, byteorder="big")
        self.sig_magic = int.from_bytes(bits[112:116], byteorder="little")
        self.r_size = int.from_bytes(bits[116:120], byteorder="little")
        self.s_size = int.from_bytes(bits[120:124], byteorder="little")
        self.sig_hash_magic = int.from_bytes(bits[124:128], byteorder="little")
        self.r = int.from_bytes(bits[128:160], byteorder="big")
        self.s = int.from_bytes(bits[160:192], byteorder="big")

        if (
            self.pk_magic == database.DC_XY_KEY_MAGIC
            and self.pk_curve_magic == 0x21339360
            and self.sig_magic == database.DC_SIGNATURE_MAGIC_NUM
            and self.sig_hash_magic == 0x00113305
        ):
            self.is_good = True

    def print_block(self):
        if not self.is_good:
            print("No public key")
            return
        print("\n\tPublic key entry:")
        print("\t\tPublic key entry magic =\t{0:#0{1}x}".format(self.magic, 10))
        print("\t\tPublic key magic =\t\t{0:#0{1}x}".format(self.pk_magic, 10))
        print(
            "\t\tPublic key curve magic =\t{0:#0{1}x}".format(self.pk_curve_magic, 10)
        )
        print("\t\tPublic key X size =\t\t{}".format(self.x_size))
        print("\t\tPublic key Y size =\t\t{}".format(self.y_size))
        print("\t\tPublic key permissions =\t{0:#0{1}x}".format(self.permissions, 10))
        print("\t\tPublic key id =\t\t\t{0:#0{1}x}".format(self.key_id, 10))
        print("\t\tPublic key X =\t\t\t{0:#0{1}x}".format(self.key_x, 66))
        print("\t\tPublic key Y =\t\t\t{0:#0{1}x}".format(self.key_y, 66))
        print("\t\tSignature magic =\t\t{0:#0{1}x}".format(self.sig_magic, 10))
        print(
            "\t\tSignature curve magic =\t\t{0:#0{1}x}".format(self.sig_hash_magic, 10)
        )
        print("\t\tSignature R =\t\t\t{0:#0{1}x}".format(self.r, 66))
        print("\t\tSignature S =\t\t\t{0:#0{1}x}".format(self.s, 66))
        print("\n\t\tPublic key entry hash =\t\t{0:#0{1}x}".format(self.hash, 66))


class DC_B0_Entry:
    def __init__(self, bits):
        self.is_good = False
        self.magic = int.from_bytes(bits[0:4], byteorder="little")
        self.size = int.from_bytes(bits[4:8], byteorder="little")
        self.dlen = int.from_bytes(bits[8:12], byteorder="little")
        self.sig_len = int.from_bytes(bits[12:16], byteorder="little")
        self.sig_magic = int.from_bytes(bits[24:28], byteorder="little")
        self.r_size = int.from_bytes(bits[28:32], byteorder="little")
        self.s_size = int.from_bytes(bits[32:36], byteorder="little")
        self.sig_hash_magic = int.from_bytes(bits[36:40], byteorder="little")
        self.r = int.from_bytes(bits[40:72], byteorder="big")
        self.s = int.from_bytes(bits[72:104], byteorder="big")

        if (
            self.sig_magic == database.DC_SIGNATURE_MAGIC_NUM
            and self.sig_hash_magic == 0x00113305
        ):
            self.is_good = True

    def print_block(self):
        if not self.is_good:
            print("No block 0 entry")
            return
        print("\n\tBlock 0 entry:")
        print("\t\tBlock 0 entry magic =\t\t{0:#0{1}x}".format(self.magic, 10))
        print("\t\tSignature magic =\t\t{0:#0{1}x}".format(self.sig_magic, 10))
        print("\t\tSignature R size =\t\t{}".format(self.r_size))
        print("\t\tSignature S size =\t\t{}".format(self.s_size))
        print(
            "\t\tSignature curve magic =\t\t{0:#0{1}x}".format(self.sig_hash_magic, 10)
        )
        print("\t\tSignature R =\t\t\t{0:#0{1}x}".format(self.r, 66))
        print("\t\tSignature S =\t\t\t{0:#0{1}x}".format(self.s, 66))

