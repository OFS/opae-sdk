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
from opae.admin.utils import common_util

PRINT_PAYLOAD = True
ADD_OPTIONS = False
PRINT_JSON = True


class FILE_TYPE_DATABASE(object):
    def __init__(self, min_csk, max_csk, enum, permission, supported_size=None):
        self.MIN_CODE_SIGNING_KEY_ENTRIES = min_csk
        self.MAX_CODE_SIGNING_KEY_ENTRIES = max_csk
        common_util.assert_in_error(
            self.MAX_CODE_SIGNING_KEY_ENTRIES >= self.MIN_CODE_SIGNING_KEY_ENTRIES,
            "Impossible Code Signing Key entry count [min: %d, max: %d]"
            % (self.MIN_CODE_SIGNING_KEY_ENTRIES, self.MAX_CODE_SIGNING_KEY_ENTRIES),
        )
        self.ENUM = enum
        self.PERMISSION = permission
        self.SUPPORTED_SIZE = supported_size


class FAMILY_DATABASE(object):
    def __init__(
        self,
        family,
        supported_types,
        max_csk,
        signature_max_size,
        supported_cancels,
        cert_types,
    ):

        self.FAMILY = family
        # You can set the types to None, if this family has only one type data
        self.SUPPORTED_TYPES = supported_types
        self.MAX_CODE_SIGNING_KEY_ENTRIES = max_csk
        self.SIGNATURE_MAX_SIZE = signature_max_size
        self.SUPPORTED_CANCELS = (
            supported_cancels
        )  # You can set cancels to None, if it does not support cancel
        self.SUPPORTED_CERT_TYPES = cert_types
        common_util.assert_in_error(
            self.SIGNATURE_MAX_SIZE > 0
            and self.SIGNATURE_MAX_SIZE <= 880
            and (self.SIGNATURE_MAX_SIZE % 4) == 0,
            (
                "Maximum signature reserved field should greater than zero, "
                + "less than 880 Bytes and multiple of 4 Bytes, but found %d"
            )
            % self.SIGNATURE_MAX_SIZE,
        )
        for key in self.SUPPORTED_TYPES:
            common_util.assert_in_error(
                self.SUPPORTED_TYPES[key].MIN_CODE_SIGNING_KEY_ENTRIES
                <= self.MAX_CODE_SIGNING_KEY_ENTRIES,
                (
                    "File type (%s) minimum Code Signing Key entry (%d) "
                    + "cannot be more than family (%s) maximum "
                    + "Code Signing Key entry (%d)"
                )
                % (
                    key,
                    self.SUPPORTED_TYPES[key].MIN_CODE_SIGNING_KEY_ENTRIES,
                    self.FAMILY,
                    self.MAX_CODE_SIGNING_KEY_ENTRIES,
                ),
            )
            common_util.assert_in_error(
                self.SUPPORTED_TYPES[key].MAX_CODE_SIGNING_KEY_ENTRIES
                <= self.MAX_CODE_SIGNING_KEY_ENTRIES,
                (
                    "File type (%s) maximum Code Signing Key entry (%d) "
                    + "cannot be more than family (%s) maximum "
                    + "Code Signing Key entry (%d)"
                )
                % (
                    key,
                    self.SUPPORTED_TYPES[key].MAX_CODE_SIGNING_KEY_ENTRIES,
                    self.FAMILY,
                    self.MAX_CODE_SIGNING_KEY_ENTRIES,
                ),
            )
        self.CURRENT_TYPE = None
        self.CURRENT_TYPE_NAME = None
        self.CURRENT_CERT_TYPE = None
        self.CURRENT_CERT_TYPE_NAME = None

    def get_type_from_enum(self, enum):

        bs_type = None
        for key in self.SUPPORTED_TYPES:
            if self.SUPPORTED_TYPES[key].ENUM == enum:
                self.CURRENT_TYPE = self.SUPPORTED_TYPES[key]
                self.CURRENT_TYPE_NAME = key
                bs_type = key
                break
        return bs_type

    def get_cert_type_from_enum(self, enum):

        cert_type = None
        for key in self.SUPPORTED_CERT_TYPES:
            if self.SUPPORTED_CERT_TYPES[key] == enum:
                self.CURRENT_CERT_TYPE = self.SUPPORTED_CERT_TYPES[key]
                self.CURRENT_CERT_TYPE_NAME = key
                cert_type = key
                break
        return cert_type


# Permissions
SIGN_SR = 0x00000001
SIGN_BMC = 0x00000002
SIGN_PR = 0x00000004

# Content type
CONTENT_SR = 0
CONTENT_BMC = 1
CONTENT_PR = 2

# Bitstream type
BITSTREAM_TYPE_UPDATE = 0
BITSTREAM_TYPE_CANCEL = 1
BITSTREAM_TYPE_RK_256 = 2
BITSTREAM_TYPE_RK_384 = 3

# For each supported family, the content type is very likely to be
#  different as well as supported signing chain entry
FAMILY_LIST = {
    "PAC_CARD": FAMILY_DATABASE(
        "PAC_CARD",
        {
            "FIM": FILE_TYPE_DATABASE(1, 1, CONTENT_SR, SIGN_SR),
            "SR": FILE_TYPE_DATABASE(1, 1, CONTENT_SR, SIGN_SR),
            "BBS": FILE_TYPE_DATABASE(1, 1, CONTENT_SR, SIGN_SR),
            "BMC_FW": FILE_TYPE_DATABASE(1, 1, CONTENT_BMC, SIGN_BMC),
            "BMC": FILE_TYPE_DATABASE(1, 1, CONTENT_BMC, SIGN_BMC),
            "AFU": FILE_TYPE_DATABASE(1, 1, CONTENT_PR, SIGN_PR),
            "GBS": FILE_TYPE_DATABASE(1, 1, CONTENT_PR, SIGN_PR),
            "PR": FILE_TYPE_DATABASE(1, 1, CONTENT_PR, SIGN_PR),
        },
        1,
        880,
        [i for i in range(0, 128)],
        {
            "UPDATE": BITSTREAM_TYPE_UPDATE,
            "CANCEL": BITSTREAM_TYPE_CANCEL,
            "RK_256": BITSTREAM_TYPE_RK_256,
            "RK_384": BITSTREAM_TYPE_RK_384,
        },
    )
}

# As long as we are still using the same crypto IP/FW the constant
# here should not change Define it here so that signer + keychain
#  can access same data
DESCRIPTOR_BLOCK_MAGIC_NUM = 0xB6EAFD19
SIGNATURE_BLOCK_MAGIC_NUM = 0xF27F28D7
ROOT_ENTRY_MAGIC_NUM = 0xA757A046
CODE_SIGNING_KEY_ENTRY_MAGIC_NUM = 0x14711C2F
BLOCK0_MAGIC_NUM = 0x15364367

DC_PLATFORM_NUM = 0x97566593
DC_CSK_MAGIC_NUM = 0x92540917
DC_SIGNATURE_MAGIC_NUM = 0x74881520
DC_ROOT_ENTRY_MAGIC = 0x89259036
DC_XY_KEY_MAGIC = 0x58700660
PR_IDENTIFIER = 0x5250

NID_X9_62_prime256v1 = 415
NID_secp384r1 = 715


class CURVE_INFO:
    def __init__(
        self,
        enum,
        name,
        openssl_name,
        size,
        curve_magic_num,
        dc_curve_magic_num,
        dc_sig_hash_magic_num,
        sha_magic_num,
        supported_families,
    ):

        self.enum = enum
        self.name = name
        self.openssl_name = openssl_name
        self.size = size
        self.curve_magic_num = curve_magic_num
        self.dc_curve_magic_num = dc_curve_magic_num
        self.dc_sig_hash_magic_num = dc_sig_hash_magic_num
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
        0x21339360,
        0x00113305,
        0xDE64437D,
        ["PAC_CARD"],
    ),
    CURVE_INFO(
        NID_secp384r1,
        "secp384r1",
        "secp384r1",
        48,
        0x08F07B47,
        0x54326648,
        0x30548820,
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
