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
# Handle how the openssl signing here
#
# If customer does not want to use openssl (prefer HSM instead),
#   this is the module that need to be replaced
#   with something that interface with HSM code/API
#   Bitstream/Keychain[QKY]/database format is unlikely to change
#   regardless you are using openssl or HSM
#
##########################
import os
from pacsign import common_util
from pacsign.logger import log
from pacsign.hsm_managers.openssl import openssl
from ctypes import *


class HSM_MANAGER(object):
    def __init__(self, cfg_file):

        self.openssl = openssl.openssl()

    def __del__(self):

        self.clean()

    def __exit__(self):

        self.clean()

    def clean(self):

        pass

    def make_private_pem(self, ecparam, private_pem, encrypt):

        curve_info = openssl.get_curve_info_from_name(ecparam)
        common_util.assert_in_error(
            curve_info is not None,
            "Expects EC curve name to be %s but found %s"
            % (openssl.get_supported_curve_info_names(), ecparam),
        )
        group = self.openssl.generate_group(curve_info.enum)
        key = self.openssl.generate_key(group)
        self.openssl.generate_private_pem(private_pem, group, key, encrypt)
        self.openssl.lib.EC_GROUP_free(group)
        self.openssl.lib.EC_KEY_free(key)

    def make_public_pem(self, private_pem, public_pem):

        key = self.openssl.read_private_key(private_pem)
        self.openssl.generate_public_pem(public_pem, key)
        self.openssl.lib.EC_KEY_free(key)

    def get_public_key(self, public_pem):

        return _PUBLIC_KEY(public_pem, self.openssl)

    def sign(self, sha, key):
        private_key = self.get_private_key(key)
        data = common_util.BYTE_ARRAY()
        private_key.sign(sha, data)
        log.debug("Signature len={}".format(data.size()))
        log.debug("".join("{:02x} ".format(x) for x in data.data))

        return data

    def get_private_key(self, private_pem):
        pem = private_pem.replace("_public_", "_private_", 1)
        return _PRIVATE_KEY(pem, self.openssl)


class _KEY(object):
    def __init__(self, file, openssl):

        assert openssl is not None
        self.file = file
        self.openssl = openssl
        self.xy = common_util.BYTE_ARRAY()
        self.key = None
        self.curve_info = None

    def __del__(self):

        self.clean()

    def __exit__(self):

        self.clean()

    def clean(self):

        if self.xy is not None:
            self.xy.null_data()
            del self.xy
            self.xy = None

        if self.key is not None:
            self.openssl.lib.EC_KEY_free(self.key)
            self.key = None

    def retrive_key_info(self):

        assert self.key is not None
        assert self.curve_info is None

        # Checking support type
        group = self.openssl.lib.EC_KEY_get0_group(self.key)
        common_util.assert_in_error(
            group is not None, "generate_openssl_key() failed to EC_KEY_get0_group()"
        )
        type = self.openssl.lib.EC_GROUP_get_curve_name(group)
        self.curve_info = openssl.get_curve_info_from_enum(type)
        common_util.assert_in_error(
            self.curve_info is not None,
            "EC curve enum (%d) name (%s) is not supported"
            % (type, self.openssl.lib.OBJ_nid2ln(type)),
        )
        log.debug(
            "Curve info={}, group={}, type={}".format(
                self.curve_info, group, self.openssl.lib.OBJ_nid2ln(type)
            )
        )

        # Get XY
        pub = self.openssl.lib.EC_KEY_get0_public_key(self.key)
        x = self.openssl.lib.BN_new()
        y = self.openssl.lib.BN_new()
        common_util.assert_in_error(
            self.openssl.lib.EC_POINT_get_affine_coordinates_GFp(
                group, pub, x, y, None
            ),
            "Fail to EC_POINT_get_affine_coordinates_GFp()",
        )
        temp = common_util.get_byte_size(self.openssl.lib.BN_num_bits(x))
        common_util.assert_in_error(
            self.curve_info.size >= temp,
            (
                "Public key X size (%d Bytes) does not aligned "
                + "with EC curve (%d Bytes)"
            )
            % (temp, self.curve_info.size),
        )
        temp = common_util.get_byte_size(self.openssl.lib.BN_num_bits(y))
        common_util.assert_in_error(
            self.curve_info.size >= temp,
            (
                "Public key Y size (%d Bytes) does not aligned "
                + "with EC curve (%d Bytes)"
            )
            % (temp, self.curve_info.size),
        )

        # Copy X and Y
        cpointer = self.get_char_point_from_bignum(x, self.curve_info.size)
        self.xy.append_data(cpointer.data)
        log.debug("".join("{:02x} ".format(x) for x in self.xy.data))
        del cpointer

        cpointer = self.get_char_point_from_bignum(y, self.curve_info.size)
        self.xy.append_data(cpointer.data)
        log.debug("".join("{:02x} ".format(x) for x in self.xy.data))
        del cpointer
        log.debug(self.xy)

        self.openssl.lib.BN_clear_free(x)
        self.openssl.lib.BN_clear_free(y)

    def check_pem_file(self):

        common_util.assert_in_error(
            os.path.exists(self.file), "PEM file %s does not exist" % self.file
        )
        lines = open(self.file, "r")
        private_tracking = 0
        encrypted_tracking = 0
        public_tracking = 0
        for line in lines:
            if line.find("-----BEGIN EC PRIVATE KEY-----") == 0:
                common_util.assert_in_error(
                    private_tracking == 0,
                    (
                        "Basic checking on %s failed. Keyword "
                        + '"-----BEGIN EC PRIVATE KEY-----" is detected twice'
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    encrypted_tracking == 0,
                    (
                        "Basic checking on %s failed. Encrypted "
                        + "Private Key is already detected"
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    public_tracking == 0,
                    ("Basic checking on %s failed. Public Key " + "is already detected")
                    % self.file,
                )
                private_tracking += 1
            elif line.find("-----BEGIN ENCRYPTED PRIVATE KEY-----") == 0:
                common_util.assert_in_error(
                    encrypted_tracking == 0,
                    (
                        "Basic checking on %s failed. Keyword "
                        + '"-----BEGIN ENCRYPTED PRIVATE KEY-----" is '
                        + "detected twice"
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    private_tracking == 0,
                    (
                        "Basic checking on %s failed. EC Private "
                        + "Key is already detected"
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    public_tracking == 0,
                    ("Basic checking on %s failed. Public " + "Key is already detected")
                    % self.file,
                )
                encrypted_tracking += 1
            elif line.find("-----BEGIN PUBLIC KEY-----") == 0:
                common_util.assert_in_error(
                    public_tracking == 0,
                    (
                        "Basic checking on %s failed. Keyword "
                        + '"-----BEGIN PUBLIC KEY-----" is detected twice'
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    private_tracking == 0,
                    (
                        "Basic checking on %s failed. EC Private "
                        + "Key is already detected"
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    encrypted_tracking == 0,
                    (
                        "Basic checking on %s failed. Encrypted "
                        + "Private Key is already detected"
                    )
                    % self.file,
                )
                public_tracking += 1
            elif line.find("-----END EC PRIVATE KEY-----") == 0:
                common_util.assert_in_error(
                    encrypted_tracking == 0,
                    (
                        "Basic checking on %s failed. Encrypted Private "
                        + "Key is already detected"
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    public_tracking == 0,
                    ("Basic checking on %s failed. Public " + "Key is already detected")
                    % self.file,
                )
                common_util.assert_in_error(
                    private_tracking != 0,
                    (
                        "Basic checking on %s failed. Keyword "
                        + '"-----BEGIN EC PRIVATE KEY-----" is not detected '
                        + 'before keyword "-----END EC PRIVATE KEY-----"'
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    private_tracking == 1,
                    (
                        "Basic checking on %s failed. Keyword "
                        + '"-----END EC PRIVATE KEY-----" is detected twice'
                    )
                    % self.file,
                )
                private_tracking += 1
            elif line.find("-----END ENCRYPTED PRIVATE KEY-----") == 0:
                common_util.assert_in_error(
                    private_tracking == 0,
                    (
                        "Basic checking on %s failed. EC Private "
                        + "Key is already detected"
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    public_tracking == 0,
                    ("Basic checking on %s failed. Public " + "Key is already detected")
                    % self.file,
                )
                common_util.assert_in_error(
                    encrypted_tracking != 0,
                    (
                        "Basic checking on %s failed. Keyword "
                        + '"-----BEGIN ENCRYPTED PRIVATE KEY-----" '
                        + "is not detected before keyword "
                        + '"-----END ENCRYPTED PRIVATE KEY-----"'
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    encrypted_tracking == 1,
                    (
                        "Basic checking on %s failed. Keyword "
                        + '"-----END ENCRYPTED PRIVATE KEY-----" '
                        + "is detected twice"
                    )
                    % self.file,
                )
                encrypted_tracking += 1
            elif line.find("-----END PUBLIC KEY-----") == 0:
                common_util.assert_in_error(
                    private_tracking == 0,
                    (
                        "Basic checking on %s failed. EC Private "
                        + "Key is already detected"
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    encrypted_tracking == 0,
                    (
                        "Basic checking on %s failed. Encrypted Private "
                        + "Key is already detected"
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    public_tracking != 0,
                    (
                        "Basic checking on %s failed. Keyword "
                        + '"-----BEGIN PUBLIC KEY-----" is not '
                        + 'detected before keyword "-----END PUBLIC KEY-----"'
                    )
                    % self.file,
                )
                common_util.assert_in_error(
                    public_tracking == 1,
                    (
                        "Basic checking on %s failed. Keyword "
                        + '"-----END PUBLIC KEY-----" is detected twice'
                    )
                    % self.file,
                )
                public_tracking += 1
        lines.close()
        common_util.assert_in_error(
            (private_tracking == 2 and encrypted_tracking == 0 and public_tracking == 0)
            or (
                private_tracking == 0
                and encrypted_tracking == 2
                and public_tracking == 0
            )
            or (
                private_tracking == 0
                and encrypted_tracking == 0
                and public_tracking == 2
            ),
            (
                "Basic checking on %s failed. Problem in detected "
                + "EC Private, Encrypted Private or Public Key"
            )
            % self.file,
        )
        return public_tracking == 2

    def get_char_point_from_bignum(self, bignum, size):

        common_util.assert_in_error(
            size == 32 or size == 48,
            (
                "get_char_point_from_bignum() expects data size to be "
                + "32 or 48 Bytes, but found %d Bytes"
            )
            % size,
        )
        cpointer = common_util.CHAR_POINTER(size)
        bn_byte = self.openssl.lib.BN_bn2bin(bignum, cpointer.data)
        common_util.assert_in_error(
            bn_byte <= size,
            ("BN_bn2bin() expects BN size to be %d Bytes, " + "but found %d Bytes")
            % (size, bn_byte),
        )
        if bn_byte != size:
            cpointer1 = common_util.CHAR_POINTER(size)
            cpointer1.assign_partial_data(cpointer.data, 0, size - bn_byte, bn_byte)
            del cpointer
            return cpointer1
        else:
            return cpointer

    def match_xy(self, public_key):

        x = self.openssl.lib.BN_new()
        y = self.openssl.lib.BN_new()
        common_util.assert_in_error(
            self.openssl.lib.EC_POINT_get_affine_coordinates_GFp(
                self.openssl.lib.EC_KEY_get0_group(public_key),
                self.openssl.lib.EC_KEY_get0_public_key(public_key),
                x,
                y,
                None,
            ),
            "match_xy() failed to EC_POINT_get_affine_coordinates_GFp()",
        )

        cpointer = self.get_char_point_from_bignum(x, self.curve_info.size)
        cpointer.compare_data(
            self.xy.data[: self.curve_info.size],
            (
                "X of Public key from QKY does not match "
                + "with private key from PEM file"
            ),
        )
        del cpointer

        cpointer = self.get_char_point_from_bignum(y, self.curve_info.size)
        cpointer.compare_data(
            self.xy.data[self.curve_info.size :],
            (
                "Y of Public key from QKY does not match "
                + "with private key from PEM file"
            ),
        )
        del cpointer

        self.openssl.lib.BN_clear_free(x)
        self.openssl.lib.BN_clear_free(y)

    def verify_signature(self, sha, rs):

        common_util.assert_in_error(
            len(sha.data) == 32 or len(sha.data) == 48 or len(sha.data) == 64,
            (
                "verify_signature() data size to be veried must be 32, "
                + "48 or 64 Bytes, but found %d Bytes"
            )
            % len(sha.data),
        )
        common_util.assert_in_error(
            len(rs) == (self.curve_info.size * 2),
            "Key expects RS size to be %d Bytes, but found %d Bytes"
            % (self.curve_info.size * 2, len(rs)),
        )
        (r, s) = self.openssl.get_bignums_from_byte_array(rs)
        signature = self.openssl.lib.ECDSA_SIG_new()
        self.openssl.lib.ECDSA_SIG_set0(signature, r, s)

        common_util.assert_in_error(
            self.openssl.lib.ECDSA_do_verify(
                sha.data, len(sha.data), signature, self.key
            ),
            "verify_signature() failed to ECDSA_do_verify()",
        )
        self.openssl.lib.ECDSA_SIG_free(signature)

    def get_content_type(self):
        if "_fim_" in self.file:
            return 0  # TODO: Fix this

        if "_bmc_" in self.file:
            return 1

        if "_pr_" in self.file:
            return 2

        return None


class _PRIVATE_KEY(_KEY):
    def __init__(self, file, openssl):

        super(_PRIVATE_KEY, self).__init__(file, openssl)
        public_pem = self.check_pem_file()
        common_util.assert_in_error(
            public_pem is False,
            (
                "Expect the PEM file %s to be private PEM, "
                + "but detected as public PEM"
            )
            % self.file,
        )
        self.key = self.openssl.read_private_key(self.file)
        self.retrive_key_info()

    def sign(self, sha, data, fixed_RS_size=0):
        log.debug("Sign: sha_size:{}".format(len(sha)))
        common_util.assert_in_error(
            len(sha) == 32 or len(sha) == 48 or len(sha) == 64,
            (
                "Supported SHA is SHA256, SHA314 and SHA512, "
                + "but found sha size of %d"
            )
            % (len(sha) * 8),
        )
        signature = self.openssl.lib.ECDSA_do_sign(sha, len(sha), self.key)
        common_util.assert_in_error(
            self.openssl.lib.ECDSA_do_verify(sha, len(sha), signature, self.key),
            "Fail to verify after the signing",
        )

        r = c_void_p(None)
        s = c_void_p(None)
        self.openssl.lib.ECDSA_SIG_get0(signature, byref(r), byref(s))

        # assign r
        cpointer = self.get_char_point_from_bignum(r, self.curve_info.size)
        log.debug(
            "Sign: curve_info.size={}, cpointer.size={}".format(
                self.curve_info.size, cpointer.size()
            )
        )
        data.append_data(cpointer.data)
        del cpointer

        if fixed_RS_size:
            assert fixed_RS_size == 48 or fixed_RS_size == 64
            for _ in range(self.curve_info.size, fixed_RS_size):
                data.append_byte(0)

        # assign s
        cpointer = self.get_char_point_from_bignum(s, self.curve_info.size)
        data.append_data(cpointer.data)
        del cpointer

        if fixed_RS_size:
            assert fixed_RS_size == 48 or fixed_RS_size == 64
            for _ in range(self.curve_info.size, fixed_RS_size):
                data.append_byte(0)

        # Free signature
        self.openssl.lib.ECDSA_SIG_free(signature)


class _PUBLIC_KEY(_KEY):
    def __init__(self, file, openssl):

        if type(file) is str:
            super(_PUBLIC_KEY, self).__init__(file, openssl)
            public_pem = self.check_pem_file()
            if public_pem:
                self.key = self.openssl.read_public_key(self.file)
            else:
                self.key = self.openssl.read_private_key(self.file)
        else:
            super(_PUBLIC_KEY, self).__init__("", openssl)
            self.key = file
        self.retrive_key_info()
        log.debug("Got key info")
        log.debug("".join("{:02x} ".format(x) for x in self.xy.data))
        log.debug(self.xy)

    def get_X_Y(self):
        return self.xy

    def get_permission(self):
        return 0xFFFFFFFF

    def get_ID(self):
        if "_root_" in self.file:
            return 0xFFFFFFFF

        if (
            "_csk" in self.file
            and self.file.rsplit("_csk", 1)[1].split("_")[0].isnumeric()
        ):
            return int(self.file.rsplit("_csk", 1)[1].split("_")[0])

        return None
