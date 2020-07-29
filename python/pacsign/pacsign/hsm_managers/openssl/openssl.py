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
# Related to openssl library call
#
##########################
import glob
import os
from ctypes import CFUNCTYPE, Structure, Union, POINTER, CDLL
from ctypes import (byref, c_void_p, c_ulong, c_uint, c_int, c_char, c_char_p,
                    c_longlong, c_long, c_size_t, util)
from sys import platform as _platform
import re

from pacsign import common_util
from pacsign.logger import log


OPENSSL_VERSION_PATTERN = (r'OpenSSL\s+'
                           r'(?P<version>\d+(?:\.\d+)*(?:[a-z]+)?)\s+'
                           r'(?P<om>FIPS)?\s*'
                           r'(?P<day>\d+)\s+(?P<month>\w+)\s+(?P<year>\d{4})')
OPENSSL_VERSION_RE = re.compile(OPENSSL_VERSION_PATTERN)

# Constant
# 1. Curve type
NID_X9_62_prime256v1 = 415
NID_secp384r1 = 715
NID_brainpoolP384r1 = 931
NID_sm2 = 1172
# 2. BIO related
BIO_C_SET_FILENAME = 108
BIO_CLOSE = 0x01
BIO_FP_READ = 0x02
BIO_FP_WRITE = 0x04
# 3. Group
OPENSSL_EC_NAMED_CURVE = 0x001
POINT_CONVERSION_UNCOMPRESSED = 4
# 4. For initialization
OPENSSL_INIT_ADD_ALL_CIPHERS = 0x00000004
OPENSSL_INIT_ADD_ALL_DIGESTS = 0x00000008
OPENSSL_INIT_LOAD_CONFIG = 0x00000040
# Flow control
DIRECT_OPENSSL_SHA = True

PEM_PASSWORD_WARNING = """The specified password is considered insecure.
         Intel recommends at least 13 characters of password.
         You are recommended to change the password using OpenSSL executable.

         Syntax:

             openssl ec -in <input PEM> -out <output PEM> -aes256
"""


class CURVE_INFO:
    def __init__(
        self,
        enum,
        name,
        openssl_name,
        size,
        curve_magic_num,
        sha_magic_num,
        supported_families,
    ):

        self.enum = enum
        self.name = name
        self.openssl_name = openssl_name
        self.size = size
        self.curve_magic_num = curve_magic_num
        self.sha_magic_num = sha_magic_num
        self.supported_families = supported_families


CURVE_INFO_DATABASE = [
    # Add more to the list
    CURVE_INFO(
        NID_X9_62_prime256v1,
        "secp256r1",
        "prime256v1",
        32,
        0xC7B88C74,
        0xDE64437D,
        ["PAC_CARD"],
    ),
    CURVE_INFO(
        NID_secp384r1,
        "secp384r1",
        "secp384r1",
        48,
        0x08F07B47,
        0xEA2A50E9,
        ["PAC_CARD"],
    ),
]


def get_curve_info_from_name(name):

    curve_info = None
    for db in CURVE_INFO_DATABASE:
        if db.name == name:
            curve_info = db
            break
    return curve_info


def get_curve_info_from_enum(enum):

    curve_info = None
    for db in CURVE_INFO_DATABASE:
        if db.enum == enum:
            curve_info = db
            break
    return curve_info


def get_curve_info_from_curve_magic_num(curve_magic_num):

    curve_info = None
    for db in CURVE_INFO_DATABASE:
        if db.curve_magic_num == curve_magic_num:
            curve_info = db
            break
    return curve_info


def get_supported_curve_info_names():

    names = []
    for db in CURVE_INFO_DATABASE:
        names.append(db.name)
    return names


def get_supported_curve_info_curve_magic_nums():

    curve_magic_nums = []
    for db in CURVE_INFO_DATABASE:
        curve_magic_nums.append("0x%08X" % db.curve_magic_num)
    return curve_magic_nums


def get_supported_curve_info_sha_magic_nums():

    sha_magic_nums = []
    for db in CURVE_INFO_DATABASE:
        sha_magic_nums.append("0x%08X" % db.sha_magic_num)
    return sha_magic_nums


def get_sha_magic_num_size(sha_magic_num):

    size = None
    for db in CURVE_INFO_DATABASE:
        if db.sha_magic_num == sha_magic_num:
            size = db.size
            break
    return size


CMPFUNC = CFUNCTYPE(c_int, POINTER(c_char), c_int, c_int, c_char_p)


def password_callback(buf, bufsiz, verify, cb_temp):

    comment = ""
    if cb_temp is not None:
        for char in cb_temp:
            if char != 0:
                comment += chr(char)
    common_util.assert_in_error(
        bufsiz >= 4,
        (
            "Minimum password must be 4 characters,"
            + " but only %d buffer size is being provided"
        )
        % bufsiz,
    )
    try:
        if verify:
            (password, error) = common_util.get_password([(
                  "Enter PEM passphrase (4 to %d characters per OpenSSL " +
                  "spec, Intel recommends minumum 13 characters): ") %
                 (bufsiz - 1),
                 ("Re-enter PEM passphrase (4 to %d characters per OpenSSL " +
                  "spec, Intel recommends minumum 13 characters): ") %
                 (bufsiz - 1), ],
                4, bufsiz - 1, comment,)
        else:
            (password, error) = common_util.get_password(
                [
                    (
                        "Enter PEM passphrase (4 to %d characters per OpenSSL"
                        + " spec, Intel recommends minumum 13 characters): "
                    )
                    % (bufsiz - 1)
                ],
                4,
                bufsiz - 1,
                comment,
            )
        common_util.assert_in_error(len(error) == 0, error)
    except Exception:
        common_util.assert_in_error(False, "Fail to get passphrase")
    password_size = len(password)
    if password_size < 13:
        common_util.print_warning(PEM_PASSWORD_WARNING)
    for i in range(password_size):
        buf[i] = password[i]
    del password
    password = []
    return password_size


callback = CMPFUNC(password_callback)


class OPENSSL_SIGNATURE(Structure):

    _fields_ = [("r", c_void_p), ("s", c_void_p)]  # void *  # void *


class OPENSSL_SHA256(Structure):

    _fields_ = [
        ("h", c_ulong * 8),
        ("N1", c_ulong),
        ("Nh", c_ulong),
        ("data", c_ulong * 16),
        ("num", c_uint),
        ("md_len", c_uint),
    ]


class OPENSSL_SHA512_DATA(Union):

    _fields_ = [("d", c_longlong * 16), ("p", c_char * 128)]


class OPENSSL_SHA512(Structure):

    _anonymous_ = ("data",)
    _fields_ = [
        ("h", c_longlong * 8),
        ("N1", c_longlong),
        ("Nh", c_longlong),
        ("data", OPENSSL_SHA512_DATA),
        ("num", c_uint),
        ("md_len", c_uint),
    ]


class OPENSSL_AES_KEY(Structure):

    _fields_ = [("rd_key", c_uint * 60), ("rounds", c_int)]


class openssl:
    # Look for something like: OpenSSL 1.1.1d FIPS  10 Sep 2019
    def _find_openssl_so(self, version, *paths):
        candidates = list(paths)
        crypto = util.find_library('crypto')
        if crypto:
            candidates.insert(0, crypto)

        for c in candidates:
            dll = CDLL(c)
            if dll is None:
                log.warn('could not open: %s', c)
                continue
            try:
                dll.OpenSSL_version.argtypes = [c_int]
                dll.OpenSSL_version.restype = c_char_p
            except AttributeError:
                log.debug('"%s: does not have OpenSSL_version', c)
                continue

            c_version = dll.OpenSSL_version(0).decode("utf-8")
            m = OPENSSL_VERSION_RE.match(c_version)
            if m is None:
                log.warn('"%s" is not a valid OpenSSL version', c_version)
                continue

            if m.group('version') == version:
                return dll

            log.debug('OpenSSL version "%s" is not equal to "%s"',
                      c_version,
                      version)

    def __init__(self, version='1.1.1g'):
        path = "%s/library" % os.path.dirname(os.path.abspath(__file__))
        self.nanotime = None
        if _platform == "win32" or _platform == "win64":
            dll_name = 'libcrypto-1_1-x64.dll'
        else:
            dll_name = '*libcrypto*.so'

        dlls = glob.glob(os.path.join(path, dll_name))

        self.lib = self._find_openssl_so(version, *dlls)
        common_util.assert_in_error(self.lib, "Failed to find crypto library")

        # Initialize OPEN algorithm
        self.lib.OPENSSL_init_crypto.argtypes = [c_uint, c_void_p]
        self.lib.OPENSSL_init_crypto.restype = None

        # Create new key
        self.lib.EC_KEY_new.argtypes = []
        self.lib.EC_KEY_new.restype = c_void_p

        # Create new group with the curve e
        self.lib.EC_GROUP_new_by_curve_name.argtypes = [c_uint]
        self.lib.EC_GROUP_new_by_curve_name.restype = c_void_p

        # pair the group and key
        self.lib.EC_KEY_set_group.argtypes = [c_void_p, c_void_p]
        self.lib.EC_KEY_set_group.restype = c_int

        # Generate key
        self.lib.EC_KEY_generate_key.argtypes = [c_void_p]
        self.lib.EC_KEY_generate_key.restype = c_int

        # Create BIO
        self.lib.BIO_new.argtypes = [c_void_p]
        self.lib.BIO_new.restype = c_void_p

        # Create BIO method
        self.lib.BIO_s_file.argtypes = []
        self.lib.BIO_s_file.restype = c_void_p

        # Read PEM file
        self.lib.BIO_ctrl.argtypes = [c_void_p, c_int, c_long, c_char_p]
        self.lib.BIO_ctrl.restype = c_int

        # Create encryption
        self.lib.EVP_get_cipherbyname.argtypes = [c_char_p]
        self.lib.EVP_get_cipherbyname.restype = c_void_p

        # Write EC Param to PEM
        self.lib.PEM_write_bio_ECPKParameters.argtypes = [c_void_p, c_void_p]
        self.lib.PEM_write_bio_ECPKParameters.restype = c_int

        # Write key to PEM
        self.lib.PEM_write_bio_ECPrivateKey.argtypes = [
            c_void_p,
            c_void_p,
            c_void_p,
            c_char_p,
            c_int,
            c_void_p,
            c_void_p,
        ]
        self.lib.PEM_write_bio_ECPrivateKey.restype = c_int

        # Write public key to PEM
        self.lib.PEM_write_bio_EC_PUBKEY.argtypes = [c_void_p, c_void_p]
        self.lib.PEM_write_bio_EC_PUBKEY.restype = c_int

        # Read Private key from PEM
        self.lib.PEM_read_bio_ECPrivateKey.argtypes = [
            c_void_p,
            c_void_p,
            c_void_p,
            c_char_p,
        ]
        self.lib.PEM_read_bio_ECPrivateKey.restype = c_void_p

        # Read Public key from PEM
        self.lib.PEM_read_bio_PUBKEY.argtypes = [
            c_void_p, c_void_p, c_void_p, c_char_p]
        self.lib.PEM_read_bio_PUBKEY.restype = c_void_p

        # Get public key
        self.lib.EC_KEY_get0_public_key.argtypes = [c_void_p]
        self.lib.EC_KEY_get0_public_key.restype = c_void_p

        # Convert a public key to key
        self.lib.EVP_PKEY_get1_EC_KEY.argtypes = [c_void_p]
        self.lib.EVP_PKEY_get1_EC_KEY.restype = c_void_p

        # Create new Big Number
        self.lib.BN_new.argtypes = []
        self.lib.BN_new.restype = c_void_p

        # Get group from key
        self.lib.EC_KEY_get0_group.argtypes = [c_void_p]
        self.lib.EC_KEY_get0_group.restype = c_void_p

        # Get EC curve from group
        self.lib.EC_GROUP_get_curve_name.argtypes = [c_void_p]
        self.lib.EC_GROUP_get_curve_name.restype = c_int

        # Generate key from curve
        self.lib.EC_KEY_new_by_curve_name.argtypes = [c_int]
        self.lib.EC_KEY_new_by_curve_name.restype = c_void_p

        # Set public key coordinate info
        self.lib.EC_KEY_set_public_key_affine_coordinates.argtypes = [
            c_void_p,
            c_void_p,
            c_void_p,
        ]
        self.lib.EC_KEY_set_public_key_affine_coordinates.restype = c_int

        # Verify key
        self.lib.EC_KEY_check_key.argtypes = [c_void_p]
        self.lib.EC_KEY_check_key.restype = c_int

        # Set Group ASN1 flag
        self.lib.EC_GROUP_set_asn1_flag.argtypes = [c_void_p, c_int]
        self.lib.EC_GROUP_set_asn1_flag.restype = None

        # Set Group conversion
        self.lib.EC_GROUP_set_point_conversion_form.argtypes = [
            c_void_p, c_int]
        self.lib.EC_GROUP_set_point_conversion_form.restype = None

        # Get XY from of a public key
        self.lib.EC_POINT_get_affine_coordinates_GFp.argtypes = [
            c_void_p,
            c_void_p,
            c_void_p,
            c_void_p,
            c_void_p,
        ]
        self.lib.EC_POINT_get_affine_coordinates_GFp.restype = c_int

        # Get number of bit of Big Number
        self.lib.BN_num_bits.argtypes = [c_void_p]
        self.lib.BN_num_bits.restype = c_int

        # Convert Big Number to Byte array
        self.lib.BN_bn2bin.argtypes = [c_void_p, c_char_p]
        self.lib.BN_bn2bin.restype = c_int

        # Convert Byte array to Big Number
        self.lib.BN_bin2bn.argtypes = [c_char_p, c_int, c_void_p]
        self.lib.BN_bin2bn.restype = c_void_p

        # Convert Big Number to char *
        self.lib.BN_bn2hex.argtypes = [c_void_p]
        self.lib.BN_bn2hex.restype = c_char_p

        self.lib.BN_copy.argtypes = [c_void_p, c_void_p]
        self.lib.BN_copy.restype = c_void_p

        self.lib.ECDSA_SIG_set0.argtypes = [c_void_p, c_void_p, c_void_p]
        self.lib.ECDSA_SIG_set0.restype = c_int

        self.lib.ECDSA_SIG_get0.argtypes = [
            c_void_p, POINTER(c_void_p), POINTER(c_void_p)]
        self.lib.ECDSA_SIG_get0.restype = None

        # Convert enum to char *
        self.lib.OBJ_nid2ln.argtypes = [c_int]
        self.lib.OBJ_nid2ln.restype = c_char_p

        # Free Big Number
        self.lib.BN_clear_free.argtypes = [c_void_p]
        self.lib.BN_clear_free.restype = None

        # Free key
        self.lib.EC_KEY_free.argtypes = [c_void_p]
        self.lib.EC_KEY_free.restype = None

        # Free public key
        self.lib.EVP_PKEY_free.argtypes = [c_void_p]
        self.lib.EVP_PKEY_free.restype = None

        # Free group
        self.lib.EC_GROUP_free.argtypes = [c_void_p]
        self.lib.EC_GROUP_free.restype = None

        # Free bio
        self.lib.BIO_free_all.argtypes = [c_void_p]
        self.lib.BIO_free_all.restype = None

        # Sign
        self.lib.ECDSA_do_sign.argtypes = [c_char_p, c_int, c_void_p]
        self.lib.ECDSA_do_sign.restype = POINTER(OPENSSL_SIGNATURE)

        # New signature
        self.lib.ECDSA_SIG_new.argtypes = []
        self.lib.ECDSA_SIG_new.restype = POINTER(OPENSSL_SIGNATURE)

        # Verify
        self.lib.ECDSA_do_verify.argtypes = [
            c_char_p,
            c_int,
            POINTER(OPENSSL_SIGNATURE),
            c_void_p,
        ]
        self.lib.ECDSA_do_verify.restype = c_int

        # Free signature
        self.lib.ECDSA_SIG_free.argtypes = [c_void_p]
        self.lib.ECDSA_SIG_free.restype = None

        # Free generic
        self.lib.CRYPTO_free.argtypes = [c_void_p]
        self.lib.CRYPTO_free.restype = None

        # Initialize
        self.lib.OPENSSL_init_crypto(
            OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS |
            OPENSSL_INIT_LOAD_CONFIG, None)

        # SHA256
        self.lib.SHA256.argtypes = [c_void_p, c_size_t, c_char_p]
        self.lib.SHA256.restype = c_char_p

        self.lib.SHA256_Init.argtypes = [POINTER(OPENSSL_SHA256)]
        self.lib.SHA256_Init.restype = c_int

        self.lib.SHA256_Update.argtypes = [
            POINTER(OPENSSL_SHA256), c_void_p, c_size_t]
        self.lib.SHA256_Update.restype = c_int

        self.lib.SHA256_Final.argtypes = [c_char_p, POINTER(OPENSSL_SHA256)]
        self.lib.SHA256_Final.restype = c_int

        # SHA384
        self.lib.SHA384.argtypes = [c_void_p, c_size_t, c_char_p]
        self.lib.SHA384.restype = c_char_p

        self.lib.SHA384_Init.argtypes = [POINTER(OPENSSL_SHA512)]
        self.lib.SHA384_Init.restype = c_int

        self.lib.SHA384_Update.argtypes = [
            POINTER(OPENSSL_SHA512), c_void_p, c_size_t]
        self.lib.SHA384_Update.restype = c_int

        self.lib.SHA384_Final.argtypes = [c_char_p, POINTER(OPENSSL_SHA512)]
        self.lib.SHA384_Final.restype = c_int

        # SHA512
        self.lib.SHA512.argtypes = [c_void_p, c_size_t, c_char_p]
        self.lib.SHA512.restype = c_char_p

        self.lib.SHA512_Init.argtypes = [POINTER(OPENSSL_SHA512)]
        self.lib.SHA512_Init.restype = c_int

        self.lib.SHA512_Update.argtypes = [
            POINTER(OPENSSL_SHA512), c_void_p, c_size_t]
        self.lib.SHA512_Update.restype = c_int

        self.lib.SHA512_Final.argtypes = [c_char_p, POINTER(OPENSSL_SHA512)]
        self.lib.SHA512_Final.restype = c_int

    def __del__(self):

        self.close()

    def __exit__(self):

        self.close()

    def close(self):

        if self.lib is not None:
            del self.lib
            self.lib = None

    def generate_group(self, nid):

        group = self.lib.EC_GROUP_new_by_curve_name(nid)
        self.lib.EC_GROUP_set_asn1_flag(group, OPENSSL_EC_NAMED_CURVE)
        self.lib.EC_GROUP_set_point_conversion_form(
            group, POINT_CONVERSION_UNCOMPRESSED
        )
        return group

    def generate_key(self, group):

        key = self.lib.EC_KEY_new()
        common_util.assert_in_error(
            self.lib.EC_KEY_set_group(key, group) != 0,
            "generate_key() failed to EC_KEY_set_group()",
        )
        common_util.assert_in_error(
            self.lib.EC_KEY_generate_key(key) != 0,
            "generate_key() failed to EC_KEY_generate_key()",
        )
        return key

    def generate_private_pem(self, pem, group, key, encrypt):

        bio = self.lib.BIO_new(self.lib.BIO_s_file())
        status = self.lib.BIO_ctrl(
            bio,
            BIO_C_SET_FILENAME,
            BIO_CLOSE | BIO_FP_WRITE,
            pem.encode("utf-8"))
        common_util.assert_in_error(
            status > 0, "Fail to open file %s for BIO write" % pem
        )
        status = self.lib.PEM_write_bio_ECPKParameters(bio, group)
        common_util.assert_in_error(
            status > 0, "Fail to write EC Param to PEM BIO")
        if encrypt is None or not encrypt:
            status = self.lib.PEM_write_bio_ECPrivateKey(
                bio, key, None, None, 0, None, None
            )
        else:
            enc = self.lib.EVP_get_cipherbyname("aes256".encode("utf-8"))
            status = self.lib.PEM_write_bio_ECPrivateKey(
                bio, key, enc, None, 0, callback, ("Writing %s" %
                                                   pem).encode("utf-8"))
        common_util.assert_in_error(
            status > 0, "Fail to write EC Private Key to PEM BIO"
        )
        self.lib.BIO_free_all(bio)

    def generate_public_pem(self, pem, key):

        bio = self.lib.BIO_new(self.lib.BIO_s_file())
        status = self.lib.BIO_ctrl(
            bio,
            BIO_C_SET_FILENAME,
            BIO_CLOSE | BIO_FP_WRITE,
            pem.encode("utf-8"))
        common_util.assert_in_error(
            status > 0, "Fail to open file %s for BIO write" % pem
        )
        status = self.lib.PEM_write_bio_EC_PUBKEY(bio, key)
        common_util.assert_in_error(
            status > 0, "Fail to write EC Publick Key to PEM BIO"
        )
        self.lib.BIO_free_all(bio)

    def read_private_key(self, private_pem):

        bio = self.lib.BIO_new(self.lib.BIO_s_file())
        status = self.lib.BIO_ctrl(
            bio,
            BIO_C_SET_FILENAME,
            BIO_CLOSE | BIO_FP_READ,
            private_pem.encode("utf-8"),
        )
        common_util.assert_in_error(
            status > 0, "Fail to read file %s for BIO read" % private_pem
        )
        key = self.lib.PEM_read_bio_ECPrivateKey(
            bio, None, callback, ("Reading %s" % private_pem).encode("utf-8")
        )
        common_util.assert_in_error(
            key is not None,
            "Fail to read EC Private Key from PEM BIO %s" %
            private_pem)
        self.lib.BIO_free_all(bio)
        return key

    def read_public_key(self, public_pem):

        bio = self.lib.BIO_new(self.lib.BIO_s_file())
        status = self.lib.BIO_ctrl(
            bio,
            BIO_C_SET_FILENAME,
            BIO_CLOSE | BIO_FP_READ,
            public_pem.encode("utf-8"))
        common_util.assert_in_error(
            status > 0, "Fail to read file %s for BIO read" % public_pem
        )
        pub_key = self.lib.PEM_read_bio_PUBKEY(bio, None, None, None)
        common_util.assert_in_error(
            pub_key is not None,
            "Fail to read EC Public Key from PEM BIO %s" % public_pem,
        )
        key = self.lib.EVP_PKEY_get1_EC_KEY(pub_key)
        common_util.assert_in_error(
            key is not None, "Fail to get EC Key from public key"
        )
        self.lib.EVP_PKEY_free(pub_key)
        self.lib.BIO_free_all(bio)
        return key

    def generate_ec_key_using_xy_and_curve_info(self, xy, curve_info):

        common_util.assert_in_error(
            len(xy) == (curve_info.size * 2),
            "%s xy size should be %d Bytes, but found %d Bytes"
            % (curve_info.name, curve_info.size * 2, len(xy)),
        )
        (x, y) = self.get_bignums_from_byte_array(xy)
        key = self.lib.EC_KEY_new_by_curve_name(curve_info.enum)
        status = self.lib.EC_KEY_set_public_key_affine_coordinates(key, x, y)
        common_util.assert_in_error(
            status > 0, "Fail to set public key affine coordinates"
        )
        status = self.lib.EC_KEY_check_key(key)
        common_util.assert_in_error(status > 0, "Invalid key")
        self.lib.BN_clear_free(x)
        self.lib.BN_clear_free(y)
        return key

    def get_bignum_from_byte_array(self, byte_array):

        cpointer = common_util.CHAR_POINTER(len(byte_array))
        cpointer.assign_data(byte_array)
        bignum = self.lib.BN_bin2bn(cpointer.data, len(byte_array), None)
        del cpointer
        return bignum

    def get_bignums_from_byte_array(self, byte_array):

        common_util.assert_in_error(
            len(byte_array) == 64 or len(byte_array) == 96,
            (
                "get_bignums_from_byte_array() expects data size to be "
                + "64 or 96 Bytes, but found %d Bytes"
            )
            % len(byte_array),
        )
        byte_array_size = int(len(byte_array) / 2)
        return [
            self.get_bignum_from_byte_array(byte_array[:byte_array_size]),
            self.get_bignum_from_byte_array(byte_array[byte_array_size:]),
        ]

    def convert_byte_array_to_char_pointer(self, byte_array):

        cpointer = common_util.CHAR_POINTER(len(byte_array))
        cpointer.assign_data(byte_array)
        return cpointer

    def get_sha256(self, data, size, sha):

        if DIRECT_OPENSSL_SHA:
            self.lib.SHA256(data, size, sha.data)
        else:
            ctx = OPENSSL_SHA256()
            self.lib.SHA256_Init(byref(ctx))
            self.lib.SHA256_Update(byref(ctx), data, size)
            self.lib.SHA256_Final(sha.data, byref(ctx))

    def get_sha384(self, data, size, sha):

        if DIRECT_OPENSSL_SHA:
            self.lib.SHA384(data, size, sha.data)
        else:
            ctx = OPENSSL_SHA512()
            self.lib.SHA384_Init(byref(ctx))
            self.lib.SHA384_Update(byref(ctx), data, size)
            self.lib.SHA384_Final(sha.data, byref(ctx))

    def get_sha512(self, data, size, sha):

        if DIRECT_OPENSSL_SHA:
            self.lib.SHA512(data, size, sha.data)
        else:
            ctx = OPENSSL_SHA512()
            self.lib.SHA512_Init(byref(ctx))
            self.lib.SHA512_Update(byref(ctx), data, size)
            self.lib.SHA512_Final(sha.data, byref(ctx))

    def get_byte_array_sha(self, type, bytes):

        common_util.assert_in_error(
            type == 32 or type == 48 or type == 64,
            (
                "Expect SHA supported type size is 32, 48 or "
                + "64 Bytes, but found %d Bytes"
            )
            % type,
        )
        common_util.assert_in_error(
            len(bytes) > 0 and (len(bytes) % 128) == 0,
            (
                "Data to be hashed must have size greater than "
                + "zero and multiple of 128 Bytes, but found %d Bytes"
            )
            % len(bytes),
        )
        common_util.assert_in_error(
            memoryview(bytes).c_contiguous,
            "Byte array is not contiguous in memory")
        cpointer = c_char_p(bytes.buffer_info()[0])
        sha = common_util.CHAR_POINTER(type)
        if type == 32:
            self.get_sha256(cpointer, bytes.buffer_info()[1], sha)
        elif type == 48:
            self.get_sha384(cpointer, bytes.buffer_info()[1], sha)
        else:
            self.get_sha512(cpointer, bytes.buffer_info()[1], sha)
        del cpointer
        return sha
