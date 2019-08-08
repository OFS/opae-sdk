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
import common_util
import database
from hashlib import sha256, sha384
import json
import zlib

from logger import log

METADATA_GUID = "XeonFPGA" + chr(0xB7) + "GBSv001"

# Metadata length field is a unsigned 32 bit int
SIZEOF_LEN_FIELD = 4

# Length of GUID string
GUID_LEN = len(METADATA_GUID)


class _READER_BASE(object):
    def __init__(self, args, hsm_manager, config):
        self.args = args
        self.hsm_manager = hsm_manager.HSM_MANAGER(config)
        self.database = database.FAMILY_LIST["PAC_CARD"]
        self.cert_type = self.database.SUPPORTED_CERT_TYPES[args.cert_type]
        self.output_file_name = args.output_file
        self.input_file = args.input_file
        log.debug(args.main_command)
        self.bitstream_type = self.database.SUPPORTED_TYPES[
            args.main_command].ENUM
        log.debug(self.bitstream_type)
        self.root_hash = common_util.BYTE_ARRAY()
        self.s10_root_hash = common_util.BYTE_ARRAY()
        cinfo = database.get_curve_info_from_name("secp256r1")
        self.curve_magic_num = cinfo.curve_magic_num
        self.dc_curve_magic_num = cinfo.dc_curve_magic_num
        self.dc_sig_hash_magic_num = cinfo.dc_sig_hash_magic_num
        self.sig_magic_num = cinfo.sha_magic_num

    def is_Darby_PR(self, contents, offset):
		#TODO: Write function to read dword and determine RC or VC
        val = contents.get_dword(offset)
        log.info("platform value is '{}' ".format(val))
        type = contents.get_word(offset + int(0xC))
        return val == database.DC_PLATFORM_NUM and type == database.PR_IDENTIFIER

    def finalize(self, fd, block0, block1, payload):
        log.info("Writing blocks to file")
        # Write to the output fd
        block0.tofile(fd)
        block1.tofile(fd)
        payload.tofile(fd)

        fd.close()

        log.info("Processing of file '{}' complete".format(fd.name))

    def make_block0(self, payload, payload_size):
        b0 = common_util.BYTE_ARRAY()

        log.info("Starting Block 0 creation")
        # Offset 0x000 - 0x003 (magic num)
        b0.append_dword(database.DESCRIPTOR_BLOCK_MAGIC_NUM)

        # Offset 0x004 - 0x007 (Size)
        b0.append_dword(payload_size)

        # Offset 0x008 - 0x008 (Type)
        b0.append_byte(self.bitstream_type)

        # Offset 0x009 - 0x009
        b0.append_byte(self.cert_type)

        # Offset 0x00A - 0x00F (reserved)
        while b0.size() < 0x10:
            b0.append_byte(0)

        log.info("Calculating SHA256")
        # Offset 0x010 - 0x02F (Sha256)
        sha = sha256(payload.data).digest()
        b0.append_data(sha)
        del sha

        log.info("Calculating SHA384")
        # Offset 0x030 - 0x05F (Sha384)
        sha = sha384(payload.data).digest()
        b0.append_data(sha)
        del sha

        common_util.assert_in_error(
            b0.size() == 96,
            "Expect data size to be 96 Bytes, but found %d Bytes" % b0.size(),
        )

        # Offset 0x060 - 0x07F (reserved)
        while b0.size() < 0x80:
            b0.append_byte(0)

        log.info("Done with Block 0")

        log.debug("".join("{:02x} ".format(x) for x in b0.data))

        # with open("b0.bin", "wb") as f:
        #     b0.tofile(f)

        return b0
    # Creates the descriptor block for DC PR
    def make_block0_dc(self, payload, payload_size):
        b0 = common_util.BYTE_ARRAY()
        for i in range(0, int(0x1000),4):
            data = payload.get_dword(i)
            b0.append_dword(data)
        log.info("Done with Block 0 for DC PR")
        log.debug("".join("{:02x} ".format(x) for x in b0.data))
        return b0

    def make_root_entry(self, pub_key):
        log.info("Starting Root Entry creation")
        # Build without magic number to calculate hash over "green" part
        root_body = common_util.BYTE_ARRAY()
        root_body.append_dword(self.curve_magic_num)
        common_util.assert_in_error(
            self.pub_root_key_perm == 0xFFFFFFFF,
            "Root public key permissions should be 0xFFFFFFFF, got 0x%08X" %
            self.pub_root_key_perm,
        )
        root_body.append_dword(0xFFFFFFFF)
        common_util.assert_in_error(
            self.pub_root_key_id == 0xFFFFFFFF,
            "Root public key permissions should be 0xFFFFFFFF, got 0x%08X" %
            self.pub_root_key_id,
        )
        root_body.append_dword(0xFFFFFFFF)

        root_body.append_data(pub_key.data[:32])
        # Offset 0x00C - 0x03B key X
        while root_body.size() < 0x3C:
            root_body.append_byte(0)

        root_body.append_data(pub_key.data[-32:])
        # Offset 0x03C - 0x006B key Y
        # Offset 0x06C - 0x07F (reserved)
        while root_body.size() < 0x80:
            root_body.append_byte(0)

        root_entry = common_util.BYTE_ARRAY()
        root_entry.append_dword(database.ROOT_ENTRY_MAGIC_NUM)
        root_entry.append_data(root_body.data)

        log.info("Calculating Root Entry SHA256")
        sha = sha256(root_body.data).digest()
        self.root_hash.append_data(sha)
        sha = sha256(pub_key.data).digest()
        self.s10_root_hash.append_data(sha)
        del sha

        log.debug("".join("{:02x} ".format(x) for x in root_entry.data))
        log.debug("".join("{:02x} ".format(x) for x in self.root_hash.data))
        log.debug("".join("{:02x} ".format(x)
                          for x in self.s10_root_hash.data))

        # with open("root_entry.bin", "wb") as f:
        #     root_entry.tofile(f)

        with open("root_hash.bin", "wb") as f:
            self.root_hash.tofile(f)

        with open("root_hash_s10.bin", "wb") as f:
            self.s10_root_hash.tofile(f)

        return root_entry
    
    def make_root_entry_dc(self, pub_key):

        log.info("Starting Root Key Entry generation for DC")
        root_entry = common_util.BYTE_ARRAY()
        xy_body = common_util.BYTE_ARRAY()
        xy_body.append_data(pub_key.data[:32])
        xy_body.append_data(pub_key.data[-32:])

        log.info("Calculating Root Entry SHA256 for DC PR")
        sha = sha256(pub_key.data).digest()
        self.s10_root_hash.append_data(sha)
        sha_msb = sha[0:4] # get first 4 bytes
        del sha
        
        with open("root_hash_s10.bin", "wb") as f:
            self.s10_root_hash.tofile(f)


        # Root Entry Magic (ND)
        root_entry.append_dword(database.DC_ROOT_ENTRY_MAGIC)
        # Length (offset to next entry)
        # TODO: hardcoded length (should be 0x78)
        # root_entry.append_dword(int(0x38+length(xy)))
        root_entry.append_dword(int(0x78))
        # Data Length (should be 0x60)
        #root_entry.append_dword(int(0x20 + length(xy)))
        root_entry.append_dword(int(0x60))
        # Signature Length (0)
        root_entry.append_dword(0)
        # SHA length (0)
        root_entry.append_dword(0)
        # Reserved (0's)
        # TODO: This shows a value of 0x1 for dword
        root_entry.append_dword(0)

        # Most Significant word of SHA hash of public key
        #root_entry.append_dword(sha_msb)
        root_entry.append_data(sha_msb)

        # Reserved
        root_entry.append_dword(0)

        # Public key magic (if XY is hashed)
        root_entry.append_dword(database.DC_XY_KEY_MAGIC)

        # X size  
        # TODO: Should be 0x20 
        root_entry.append_dword(len(pub_key.data[:32]))
        # Y size
        root_entry.append_dword(len(pub_key.data[-32:]))

        # Public curve magic
        root_entry.append_dword(self.dc_curve_magic_num)
        # Public key permissions (reserved, 0xFFFFFFFF)
        common_util.assert_in_error(
            self.pub_root_key_perm == 0xFFFFFFFF,
            "Root public key permissions should be 0xFFFFFFFF, got 0x%08X" %
            self.pub_root_key_perm,
        )
        root_entry.append_dword(0xFFFFFFFF)
        # Public key cancellation (reserved, -1)
        root_entry.append_dword(0xFFFFFFFF)
        # Hashed XY
        root_entry.append_data(xy_body.data)
        return root_entry

    def make_csk_entry(self, root_key, CSK_pub_key):
        log.info("Starting Code Signing Key Entry creation")
        csk_body = common_util.BYTE_ARRAY()

        csk_body.append_dword(self.curve_magic_num)
        csk_body.append_dword(self.pub_CSK_perm)
        csk_body.append_dword(self.pub_CSK_id)

        csk_body.append_data(CSK_pub_key.data[:32])
        # Offset 0x00C - 0x03B key X
        while csk_body.size() < 0x3C:
            csk_body.append_byte(0)

        csk_body.append_data(CSK_pub_key.data[-32:])
        # Offset 0x03C - 0x006B key Y
        # Offset 0x06C - 0x07F (reserved)
        while csk_body.size() < 0x80:
            csk_body.append_byte(0)

        csk_entry = common_util.BYTE_ARRAY()
        csk_entry.append_dword(database.CODE_SIGNING_KEY_ENTRY_MAGIC_NUM)
        csk_entry.append_data(csk_body.data)
        csk_entry.append_dword(self.sig_magic_num)

        log.info("Calculating Code Signing Key Entry SHA256")
        if root_key is not None:
            sha = sha256(csk_body.data).digest()

            rs = self.hsm_manager.sign(sha, root_key)
            del sha

            csk_entry.append_data(rs.data[:32])
            # Offset 0x088 - 0x0B7 signature R
            while csk_entry.size() < 0xB8:
                csk_entry.append_byte(0)

            csk_entry.append_data(rs.data[-32:])

            del rs

        # Offset 0x0B8 - 0x00E7 signature S
        while csk_entry.size() < 0xE8:
            csk_entry.append_byte(0)

        log.info("Code Signing Key Entry done")

        return csk_entry

    def make_csk_entry_dc(self, root_key, CSK_pub_key):
        log.info("Starting Code Signing Key Entry creation")
        csk_body = common_util.BYTE_ARRAY()
        

        # Public key magic (no enablement)
        csk_body.append_dword(database.DC_XY_KEY_MAGIC)

        # Reserved if no enablement (write x size)
        # SHA256 is 32 bytes
        csk_body.append_dword(int(0x20))
        # Reserved if no enablement (write y size)
        csk_body.append_dword(int(0x20))

        # Public key curve magic
        csk_body.append_dword(self.dc_curve_magic_num)

        # Public key permissions
        # TODO: This should be 2 for DC PR
        #csk_body.append_dword(self.pub_CSK_perm)
        csk_body.append_dword(0x2)

        # Public key cancellation
        csk_body.append_dword(0)

        # Public Key XY
        csk_body.append_data(CSK_pub_key.data[:32])
        # TODO: REMOVE THIS if no padding needed
        # Offset 0x018 - 0x048 key x
        # while csk_body.size() < 0x48:
        #    csk_body.append_byte(0)

        csk_body.append_data(CSK_pub_key.data[-32:])
        # TODO: REMOVE THIS if no padding needed
        # Offset 0x048 - 0x078 key Y
        # while csk_body.size() < 0x78:
        #    csk_body.append_byte(0)
        # E Size is 0 (skipping)

        csk_entry = common_util.BYTE_ARRAY()
        # Public Entry Magic Number
        csk_entry.append_dword(database.DC_CSK_MAGIC_NUM)
        # Length (to next entry)
        # TODO: is this hardcoded
        csk_entry.append_dword(int(0xC0))

        # Data length (0x18 + XSIZE + YSIZE)
        csk_entry.append_dword(int(0x58))
        # Signature Length (0x10 + RSIZE + SSIZE)
        csk_entry.append_dword(int(0x50))

        # SHA Length (0)
        csk_entry.append_dword(0)
        # Reserved 0s
        csk_entry.append_dword(0)

        csk_entry.append_data(csk_body.data)
        csk_entry.append_dword(database.DC_SIGNATURE_MAGIC_NUM)
        # Reserved if no enablement (write r size)
        csk_entry.append_dword(int(0x20))
        # Reserved if no enablement (write s size)
        csk_entry.append_dword(int(0x20))
        # DC Signature Hash Magic
        csk_entry.append_dword(self.dc_sig_hash_magic_num)

        log.info("Calculating Code Signing Key Entry SHA256")
        if root_key is not None:
            sha = sha256(csk_body.data).digest()

            rs = self.hsm_manager.sign(sha, root_key)
            del sha

            csk_entry.append_data(rs.data[:32])
            # TODO: REMOVE THIS if no padding needed
            # Offset 0x088 - 0x0B7 signature R
            # while csk_entry.size() < 0xD0:
            #    csk_entry.append_byte(0)

            csk_entry.append_data(rs.data[-32:])

            del rs
        # TODO: REMOVE THIS if no padding needed
        while csk_entry.size() < 0xC0:
            csk_entry.append_byte(0)

        log.info("Code Signing Key Entry done")

        return csk_entry


    def make_block0_entry(self, block0, CSK_key):
        b0_entry = common_util.BYTE_ARRAY()
        log.info("Starting Block 0 Entry creation")
        b0_entry.append_dword(database.BLOCK0_MAGIC_NUM)

        b0_entry.append_dword(self.sig_magic_num)

        log.info("Calculating Block 0 Entry SHA256")
        if CSK_key is not None:
            sha = sha256(block0.data).digest()

            rs = self.hsm_manager.sign(sha, CSK_key)
            del sha

            b0_entry.append_data(rs.data[:32])
            # Offset 0x008 - 0x037 signature R
            while b0_entry.size() < 0x38:
                b0_entry.append_byte(0)

            b0_entry.append_data(rs.data[-32:])

            del rs
        # TODO: Check these offset values.  v5 says 0x38 - 0x58
        # Offset 0x038 - 0x0067 signature S
        while b0_entry.size() < 0x68:
            b0_entry.append_byte(0)

        log.info("Block 0 Entry done")

        return b0_entry

    # Make block 0 entry for DC
    def make_block0_entry_dc(self, block0, CSK_key):
        b0_entry = common_util.BYTE_ARRAY()
        log.info("Starting Block 0 Entry Creation DC PR")
        # Block 0 magic number
        b0_entry.append_dword(database.BLOCK0_MAGIC_NUM)
        # Length (to next entry)
        #b0_entry.append_dword(int(0x28 + rs.size()))
        b0_entry.append_dword(int(0x68))
        # Data Length(0)
        b0_entry.append_dword(0)
        # Signature Length (0x10 + RSIZE + SSIZE)
        #b0_entry.append_dword(int(0x10 + rs.size()))
        b0_entry.append_dword(int(0x50))
        # SHA length (0)
        b0_entry.append_dword(0)
        # Reserved (0)
        b0_entry.append_dword(0)

        # Signature Magic
        b0_entry.append_dword(database.DC_SIGNATURE_MAGIC_NUM)
        # R size (x)
        #b0_entry.append_dword(int(rs.size()/2))
        b0_entry.append_dword(0x20)
        # S size (x)
        #b0_entry.append_dword(int(rs.size()/2))
        b0_entry.append_dword(0x20)
        # Signature hash Magic
        # TODO: Right now only supports secp256r1
        b0_entry.append_dword(self.dc_sig_hash_magic_num)

        # Signature R & S
        if CSK_key is not None:
            sha = sha256(block0.data).digest()
            rs = self.hsm_manager.sign(sha, CSK_key)
            del sha
            b0_entry.append_data(rs.data[:32])
            b0_entry.append_data(rs.data[-32:])
            del rs

        while b0_entry.size() < 0x68:
            b0_entry.append_byte(0)

        log.info("Block 0 Entry done")
        return b0_entry

    def make_block1(self, root_entry=None, block0_entry=None, CSK=None):
        b1 = common_util.BYTE_ARRAY()
        log.info("Starting Block 1 creation")
        b1.append_dword(database.SIGNATURE_BLOCK_MAGIC_NUM)
        while b1.size() < 0x10:
            b1.append_byte(0)

        if root_entry is not None:
            b1.append_data(root_entry.data)

        if CSK is not None:
            b1.append_data(CSK.data)

        if block0_entry is not None:
            b1.append_data(block0_entry.data)

        while b1.size() < 1024 - 128:
            b1.append_byte(0)

        log.info("Block 1 done")

        return b1

    #Signature Block, Size (0x1000)
    def make_block1_dc(self, block0, root_entry=None, block0_entry=None, CSK=None):
        b1 = common_util.BYTE_ARRAY()
        log.info("Starting Block 1 creation for DC PR")
        # SHA384 over block 0
        # TODO: First 64 bytes are same from unsigned block (verify match)
        sha = sha384(block0.data).digest()
        b1.append_data(sha)
        # Padding to make sha384, 64 bits
        b1.append_qword(0x0)
        b1.append_qword(0x0)
        # 1st signature  # of entries
        # TODO: 3 entries. root, csk, b0
        b1.append_dword(int(0x3))
        # Offset to first entry (start of signature block)
        b1.append_dword(int(0x60))
        # 2nd-4th signature chains (Do not use)
        b1.append_qword(0)
        b1.append_qword(0)
        b1.append_qword(0)
        # Signature Chains
        if root_entry is not None:
            b1.append_data(root_entry.data)
        if CSK is not None:
            b1.append_data(CSK.data)
        if block0_entry is not None:
            b1.append_data(block0_entry.data)
        # TODO:  Main image pointer area?
        # Pointer to main image
        while (b1.size() < 0xFFC):
            b1.append_byte(0)
        # CRC32 over 0 to SIZE-4 TODO: add the right data
        crc = zlib.crc32(b1.data) & 0xffffffff
        b1.append_dword(crc)
        log.info("Block 1 done")

        return b1

class CANCEL_reader(_READER_BASE):
    def __init__(self, args, hsm_manager, config):
        super(CANCEL_reader, self).__init__(args, hsm_manager, config)
        self.root_key = args.root_key
        self.csk_id = args.csk_id
        common_util.assert_in_error(
            self.csk_id in self.database.SUPPORTED_CANCELS,
            "cancel_id ID of 0x%08X is not supported" % (self.csk_id),
        )
        log.debug("Root key is {}".format(args.root_key))
        self.pub_root_key_c = self.hsm_manager.get_public_key(args.root_key)
        common_util.assert_in_error(self.pub_root_key_c is not None,
                                    "Can not retrieve root public key")
        log.debug(self.pub_root_key_c.get_X_Y())
        log.debug("".join("{:02x} ".format(x)
                          for x in self.pub_root_key_c.get_X_Y().data))
        self.pub_root_key = self.pub_root_key_c.get_X_Y()
        self.pub_root_key_perm = self.pub_root_key_c.get_permission()
        self.pub_root_key_id = self.pub_root_key_c.get_ID()

        self.pub_root_key_type = self.pub_root_key_c.get_content_type()
        if isinstance(self.pub_root_key_type, str):
            self.pub_root_key_type = self.database.SUPPORTED_TYPES[
                self.pub_root_key_type].ENUM
        log.debug("FOO: {}".format(self.pub_root_key_type))
        common_util.assert_in_error(self.pub_root_key_type is not None,
                                    "Cannot retrieve root public key type")
        common_util.assert_in_error(
            self.pub_root_key_type == self.bitstream_type,
            "Root key content type mismatch: key={}, type={}".format(
                self.pub_root_key_type, self.bitstream_type),
        )

    def run(self):
        # Put the key ID into the payload and pad
        payload = common_util.BYTE_ARRAY()
        payload.append_dword(self.csk_id)
        # Append the data to be multiple of 128 Bytes aligned
        while payload.size() % 128:
            payload.append_byte(0)
        log.debug("".join("{:02x} ".format(x) for x in payload.data))

        # Create block 0, root entry, and block 0 entry
        block0 = self.make_block0(payload, payload.size())
        root_entry = self.make_root_entry(self.pub_root_key)
        block0_entry = self.make_block0_entry(block0, self.root_key)

        # Make block 1 using the "keychain" we just made components for
        block1 = self.make_block1(root_entry=root_entry,
                                  block0_entry=block0_entry)

        output_fd = open(self.output_file_name, "wb")
        # Write to the output file
        self.finalize(output_fd, block0, block1, payload)


class UPDATE_reader(_READER_BASE):
    def __init__(self, args, hsm_manager, config):
        super(UPDATE_reader, self).__init__(args, hsm_manager, config)
        self.root_key = args.root_key
        self.pub_root_key_perm = 0xFFFFFFFF
        self.pub_root_key_id = 0xFFFFFFFF
        self.pub_CSK_perm = 0xFFFFFFFF
        self.pub_CSK_id = 0
        self.pub_root_key_type = None
        self.pub_CSK_type = None
        if self.root_key is not None:
            self.pub_root_key_c = self.hsm_manager.get_public_key(
                args.root_key)
            log.debug(self.pub_root_key_c)
            common_util.assert_in_error(self.pub_root_key_c,
                                        "Cannot retrieve root public key")
            self.pub_root_key = self.pub_root_key_c.get_X_Y()
            self.pub_root_key_perm = self.pub_root_key_c.get_permission()
            self.pub_root_key_id = self.pub_root_key_c.get_ID()
            self.pub_root_key_type = self.pub_root_key_c.get_content_type()
            if isinstance(self.pub_root_key_type, str):
                self.pub_root_key_type = self.database.SUPPORTED_TYPES[
                    self.pub_root_key_type].ENUM
            common_util.assert_in_error(
                self.pub_root_key_type is not None,
                "Cannot retrieve root public key type")
            common_util.assert_in_error(
                self.pub_root_key_type == self.bitstream_type,
                "Root key content type mismatch: key={}, type={}".format(
                    self.pub_root_key_type, self.bitstream_type),
            )
        else:
            self.pub_root_key = common_util.BYTE_ARRAY()
            while self.pub_root_key.size() < 96:
                self.pub_root_key.append_byte(0)

        self.CSK = args.code_signing_key
        if self.CSK is not None:
            self.pub_CSK_c = self.hsm_manager.get_public_key(
                args.code_signing_key)
            common_util.assert_in_error(self.pub_CSK_c is not None,
                                        "Cannot retrieve public CSK")
            self.pub_CSK = self.pub_CSK_c.get_X_Y()
            self.pub_CSK_perm = self.pub_CSK_c.get_permission()
            self.pub_CSK_id = self.pub_CSK_c.get_ID()
            self.pub_CSK_type = self.pub_CSK_c.get_content_type()
            if isinstance(self.pub_CSK_type, str):
                self.pub_CSK_type = self.database.SUPPORTED_TYPES[
                    self.pub_CSK_type].ENUM
            common_util.assert_in_error(self.pub_CSK_type is not None,
                                        "Cannot retrieve CSK type")
            common_util.assert_in_error(
                self.pub_CSK_type == self.bitstream_type,
                "CSK content type mismatch: key={}, type={}".format(
                    self.pub_CSK_type, self.bitstream_type),
            )
        else:
            self.pub_CSK = common_util.BYTE_ARRAY()
            while self.pub_CSK.size() < 96:
                self.pub_CSK.append_byte(0)

    def is_Rush_BMC(self, payload, offset):
        if payload.get_word(offset) > 1024:
            return False

        # Check if this is a WRITE_NVM IPMI command
        for i in range(7):
            if (int(payload.get_string(offset + 2 + i, 1)) !=
                    b"\xb8\x00\x0b\x18\x7b\x00\x00" [i]):
                return False

        return True

    def run(self):
        payload_content = common_util.BYTE_ARRAY("FILE", self.input_file)
        log.debug("Payload size is {}".format(payload_content.size()))

        # Skip JSON and old signature if they exist
        has_json = self.is_JSON(payload_content)
        log.debug("has_json = {}".format(has_json))
        payload_offset = 0 if not has_json else self.skip_JSON(payload_content)
        sig_offset = payload_offset
        prev_sig = self.already_signed(payload_content, payload_offset)
        payload_offset += 1024 if prev_sig else 0
        log.debug("payload_offset={}".format(payload_offset))
        json_string = common_util.BYTE_ARRAY()
        if has_json:
            json_string.append_data(
                payload_content.data[GUID_LEN + SIZEOF_LEN_FIELD:sig_offset])
            # json_string.append_data(payload_content.data[:sig_offset])
        payload = common_util.BYTE_ARRAY()
        log.debug("".join("{:02x} ".format(x) for x in json_string.data))
        log.debug(bytearray(json_string.data).decode("utf-8"))

        # Determine if platform is RC or DC
        dc_pr = self.is_Darby_PR(payload_content, sig_offset)

        if has_json:
            j_data = json.loads(bytearray(json_string.data).decode("utf-8"))
            log.debug(json.dumps(j_data, sort_keys=True, indent=4))

        log.debug(self.bitstream_type)
        should_swizzle = self.bitstream_type == database.CONTENT_SR
        log.debug("should_swizzle={}".format(should_swizzle))

        if should_swizzle and not prev_sig:
            payload.append_data_swizzled(payload_content.data[payload_offset:])
        else:
            payload.append_data(payload_content.data[payload_offset:])
        # Append the data to be multiple of 128 Bytes aligned
        while payload.size() % 128:
            payload.append_byte(0)

        log.debug("Payload size is {}".format(payload.size()))
        # Create block 0, root entry, and block 0 entry
        if dc_pr:
            block0 = self.make_block0_dc(payload, payload.size())
            # Remove first 0x2000 bytes.
            payload = payload.data[int(0x2000):]
            root_entry = self.make_root_entry_dc(self.pub_root_key)
            CSK = self.make_csk_entry_dc(self.root_key, self.pub_CSK)
            block0_entry = self.make_block0_entry_dc(block0, self.CSK)
            block1 = self.make_block1_dc(block0=block0,
                                      root_entry=root_entry,
                                      CSK=CSK,
                                      block0_entry=block0_entry)

        else:    
            block0 = self.make_block0(payload, payload.size())
            root_entry = self.make_root_entry(self.pub_root_key)
            CSK = self.make_csk_entry(self.root_key, self.pub_CSK)
            block0_entry = self.make_block0_entry(block0, self.CSK)
            # Make block 1 using the "keychain" we just made components for
            block1 = self.make_block1(root_entry=root_entry,
                                      CSK=CSK,
                                      block0_entry=block0_entry)

        output_fd = open(self.output_file_name, "wb")
        # Write to the output file
        if has_json:
            mod_json_string = common_util.BYTE_ARRAY()
            log.debug("r={}, c={}, h={}".format(
                self.root_key,
                self.CSK,
                "".join("{:02x} ".format(x) for x in self.root_hash.data),
            ))
            sig = {}
            sig["root_key"] = self.root_key
            sig["CSK"] = self.CSK
            sig["root_hash"] = " ".join("0x{:02x}".format(x)
                                        for x in self.root_hash.data)
            sig["root_pub_key-X"] = " ".join(
                "0x{:02x}".format(x) for x in self.pub_root_key.data[:32])
            sig["root_pub_key-Y"] = " ".join(
                "0x{:02x}".format(x) for x in self.pub_root_key.data[-32:])
            sig["CSK_pub_key-X"] = " ".join("0x{:02x}".format(x)
                                            for x in self.pub_CSK.data[:32])
            sig["CSK_pub_key-Y"] = " ".join("0x{:02x}".format(x)
                                            for x in self.pub_CSK.data[-32:])
            j_data["signature"] = sig
            log.debug(json.dumps(j_data, sort_keys=True, indent=4))
            mod_json_string.append_data(METADATA_GUID)
            j_string = json.dumps(j_data, sort_keys=True)
            mod_json_string.append_dword(len(j_string))
            mod_json_string.append_data(j_string)
            mod_json_string.tofile(output_fd)
        self.finalize(output_fd, block0, block1, payload)

    def is_JSON(self, contents):
        log.debug("GUID: {} vs. file {}".format(
            METADATA_GUID,
            "".join([chr(i) for i in contents.data[:GUID_LEN]])))
        return METADATA_GUID == "".join(
            [chr(i) for i in contents.data[:GUID_LEN]])

    def skip_JSON(self, contents):
        leng = contents.get_dword(GUID_LEN)
        log.debug("length of json={}".format(leng))
        return leng + GUID_LEN + SIZEOF_LEN_FIELD

    def already_signed(self, contents, offset):
        if contents.get_dword(offset) != database.DESCRIPTOR_BLOCK_MAGIC_NUM:
            log.info("Bitstream not previously signed")
            return False

        if contents.get_dword(offset + 4) % 128:
            log.info("Bitstream not previously signed")
            return False

        con_type = contents.get_dword(offset + 8)

        if (con_type >> 8) & 0xFF != self.cert_type:
            common_util.assert_in_error(
                False,
                "Attempting to change cert type from {} to {}".format(
                    self.database.get_cert_type_from_enum(con_type & 0xFF),
                    self.database.get_cert_type_from_enum(self.bitstream_type),
                ),
            )
        if con_type & 0xFF != self.bitstream_type:
            common_util.assert_in_error(
                False,
                "Attempting to re-sign type {} to type {}".format(
                    self.database.get_type_from_enum(con_type & 0xFF),
                    self.database.get_type_from_enum(self.bitstream_type),
                ),
            )

        if contents.get_dword(offset +
                              128) != database.SIGNATURE_BLOCK_MAGIC_NUM:
            log.info("Bitstream not previously signed")
            return False

        if contents.get_dword(offset + 144) != database.ROOT_ENTRY_MAGIC_NUM:
            log.info("Bitstream not previously signed")
            return False

        log.warning("Bitstream is already signed - removing signature blocks")
        return True


class RHP_reader(_READER_BASE):
    def __init__(self, args, hsm_manager, config):
        super(RHP_reader, self).__init__(args, hsm_manager, config)
        self.root_key = args.root_key
        self.pub_root_key_c = self.hsm_manager.get_public_key(args.root_key)
        common_util.assert_in_error(self.pub_root_key_c is not None,
                                    "Cannot retrieve root public key")
        self.pub_root_key = self.pub_root_key_c.get_X_Y()
        self.pub_root_key_perm = self.pub_root_key_c.get_permission()
        self.pub_root_key_id = self.pub_root_key_c.get_ID()
        self.pub_root_key_type = self.pub_root_key_c.get_content_type()
        if isinstance(self.pub_root_key_type, str):
            self.pub_root_key_type = self.database.SUPPORTED_TYPES[
                self.pub_root_key_type].ENUM
        common_util.assert_in_error(self.pub_root_key_type is not None,
                                    "Cannot retrieve root public key type")
        common_util.assert_in_error(
            self.pub_root_key_type == self.bitstream_type,
            "Root key content type mismatch: key={}, type={}".format(
                self.pub_root_key_type, self.bitstream_type),
        )

    def run(self):
        # Make root entry to get the hash
        root_entry = self.make_root_entry(self.pub_root_key)
        common_util.assert_in_error(self.root_hash.size() >= 32,
                                    "Cannot retrieve root hash")
        common_util.assert_in_error(self.s10_root_hash.size() >= 32,
                                    "Cannot retrieve Quartus hash")
        payload = common_util.BYTE_ARRAY()

        # Add hash to payload and pad
        payload.append_data(self.root_hash.data)

        # Append the data to be multiple of 128 Bytes aligned
        while payload.size() < 48:
            payload.append_byte(0)

        if self.bitstream_type == database.CONTENT_PR:
            # Add hash to payload and pad
            payload.append_data(self.s10_root_hash.data)

        # Append the data to be multiple of 128 Bytes aligned
        while payload.size() % 128:
            payload.append_byte(0)

        # Make block 0 from the payload
        block0 = self.make_block0(payload, payload.size())

        # Make block 1 with no signing required
        block1 = self.make_block1()

        output_fd = open(self.output_file_name, "wb")
        # Write to the output file
        self.finalize(output_fd, block0, block1, payload)
