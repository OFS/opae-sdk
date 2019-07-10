##########################
#
# Handle PKCS#11 signing and key management here
#
##########################

import common_util
import pkcs11
import pkcs11.util
from pkcs11.util import ec
from pkcs11 import ObjectClass, Mechanism, Attribute
import json

from logger import log


class HSM_MANAGER(object):
    def __init__(self, cfg_file=None):
        common_util.assert_in_error(
            cfg_file, "PKCS11 HSM manager requires a configuration file"
        )
        self.session = None
        with open(cfg_file, "r") as read_file:
            self.j_data = json.load(read_file)
        j_data = self.j_data

        lib = pkcs11.lib(j_data["lib_path"])
        common_util.assert_in_error(
            j_data["library_version"] == list(lib.library_version),
            "PKCS11 HSM manager library version mismatch",
        )
        common_util.assert_in_error(
            j_data["cryptoki_version"] == list(lib.cryptoki_version),
            "PKCS11 HSM manager cryptoki version mismatch",
        )
        token = lib.get_token(token_label=j_data["token"]["label"])
        self.session = token.open(user_pin=j_data["token"]["user_password"])
        self.curve = j_data["curve"]

        self.ecparams = self.session.create_domain_parameters(
            pkcs11.KeyType.EC,
            {
                pkcs11.Attribute: pkcs11.util.ec.encode_named_curve_parameters(
                    self.curve
                )
            },
            local=True,
        )

    def __del__(self):
        self.clean()

    def __exit__(self):
        self.clean()

    def clean(self):
        if self.session:
            self.session.close()

    def get_key(self, key, attrib):
        keys = self.j_data["token"]["keys"]
        local_key = None
        for k in keys:
            if key == k["label"]:
                local_key = k
                break
        if not local_key:
            raise pkcs11.NoSuchKey

        key_ = self.session.get_key(
            label=local_key["label"], object_class=attrib)

        return key_, local_key

    def get_public_key(self, public_key):
        try:
            key_, local_key = self.get_key(public_key, ObjectClass.PUBLIC_KEY)
            key_ = key_[Attribute.EC_POINT]

            common_util.assert_in_error(
                key_[0] == 0x04, "PKCS11 HSM manager key not in DER format"
            )
            common_util.assert_in_error(
                key_[1] == 0x41, "PKCS11 HSM manager key not in DER format"
            )
            common_util.assert_in_error(
                key_[2] == 0x04, "PKCS11 HSM manager key not in DER format"
            )
        except pkcs11.NoSuchKey:
            log.error("No such key")
        except pkcs11.MultipleObjectsReturned:
            log.error("multiple")
        return _PUBLIC_KEY(key_[3:], local_key)

    def sign(self, sha, key):
        try:
            key_, _ = self.get_key(key, ObjectClass.PRIVATE_KEY)
        except pkcs11.NoSuchKey:
            log.error("No such key")
        except pkcs11.MultipleObjectsReturned:
            log.error("multiple")

        rs = common_util.BYTE_ARRAY()
        rs.append_data(key_.sign(sha, mechanism=Mechanism.ECDSA))
        log.debug("RS length is {}".format(rs.size()))
        log.debug("".join("{:02x} ".format(x) for x in rs.data))

        return rs


class _PUBLIC_KEY(object):
    def __init__(self, xy, key_info):
        self.xy = common_util.BYTE_ARRAY()
        self.xy.append_data(xy)
        log.debug("Got key info")
        log.debug("".join("{:02x} ".format(x) for x in self.xy.data))
        log.debug(self.xy)
        self.key_info = key_info

    def get_X_Y(self):
        return self.xy

    def get_permission(self):
        try:
            perm = int(self.key_info["permissions"])
        except ValueError:
            perm = int(self.key_info["permissions"], 16)

        return perm

    def get_ID(self):
        if self.key_info["is_root"]:
            return 0xFFFFFFFF

        try:
            csk_id = int(self.key_info["csk_id"])
        except ValueError:
            csk_id = int(self.key_info["csk_id"], 16)

        return csk_id

    def get_content_type(self):
        return self.key_info["type"]
