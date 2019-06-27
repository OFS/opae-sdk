#!/usr/bin/env python

import io
import re
import unittest

from fragment import BitstreamFragment
from magic import MagicNumber
from util.convert import Convert
from util.fileutil import FileUtils


class EcdsaPublicKey(BitstreamFragment):
    '''
       * Note that SDM full bootrom only understands ECDSA384
          [see 7.1.1.2.1 of Nadder Firmware Security MAS.docx]

        # To future self if you want to implement the openssl version of this
        #  https://wiki.openssl.org/index.php/Command_Line_Elliptic_Curve_Operations
        #  openssl ecparam -genkey -name prime256v1 -out root_key.pem
        #  openssl ecparam -genkey -name secp384r1 -out root_key.pem
        #  openssl ec -in root_key.pem -pubout -out ecpubkey.pem
    '''

    def __init__(self, enable_multi=False, in_sigchain_pubkey_entry=False):
        super(EcdsaPublicKey,self).__init__(byte_array=None, max_size=120)
        if enable_multi:
            self.expected_magic.append((MagicNumber.PUBLIC_KEY_MULTI_ROOT,0x0))
        else:
            self.expected_magic.append((MagicNumber.PUBLIC_KEY_SINGLE_ROOT,0x0))

        self._x_size = None
        self._y_size = None
        self._in_sigchain_pubkey_entry = in_sigchain_pubkey_entry

    def extract_entry_from_sexp(self, keyword, sexp, binary=True):

        if not binary:
            sexp = re.sub(r'[\s+]','',sexp)

        match = re.search(r'\(([1-9][0-9]*:)?' + keyword, sexp)

        if match is None:
            return None

        match_prefix = sexp[match.start()+1:match.end()-len(keyword)]
        if match_prefix.endswith(':'):
            match_prefix = match_prefix.rstrip(':')
            assert(len(match_prefix) > 0)
            match_prefix = int(match_prefix)
            assert(match_prefix > 0)

        parenthesis_count = 1
        str_ptr = match.end()
        while str_ptr < len(sexp):

            if sexp[str_ptr] == '(':
                parenthesis_count += 1
            elif sexp[str_ptr] == ')':
                parenthesis_count -= 1

            if parenthesis_count == 0:
                break

            int_prefix_match = re.search(r'\(?[1-9][0-9]*:', sexp[str_ptr:])
            if int_prefix_match != None:
                prefix = sexp[str_ptr:][int_prefix_match.start():int_prefix_match.end()]
                prefix = prefix.rstrip(':')
                prefix = prefix.lstrip('(')
                str_ptr += int_prefix_match.end() + int(prefix)
            else:
                str_ptr += 1

        return sexp[match.end():str_ptr]

    def initialize(self, sexp=None, sexp_file=None, binary=True, fuse_enable=None, contrib=None, permission=None, cancel_id=None):

        if sexp_file is not None:
            if binary:
                with FileUtils().read_binary_file_open(filename=sexp_file) as fp:
                    sexp_ba = bytearray(fp.read())
                    sexp = str(sexp_ba)
            else:
                with io.open(sexp_file, "r") as fp:
                    sexp = fp.read()

        assert(sexp != None)

        sexp_public_key = self.extract_entry_from_sexp(keyword="public-key", sexp=sexp, binary=binary)

        if sexp_public_key is None:
            raise ValueError('Could not find public-key in provided s-expression:\n' + sexp)

        for key_type in ["secp256r1", "secp384r1", "prime256v1"]:
            key_lookup_string = "ecdsa-" + key_type + "-sha384"
            sexp_public_key_xy = self.extract_entry_from_sexp(keyword=key_lookup_string, sexp=sexp_public_key, binary=binary)
            if sexp_public_key_xy is not None:
                break

        if sexp_public_key is None:
            raise ValueError('Could not find valid key type in provided s-expression:\n' + sexp)

        curve = {}
        curve['x'] = None
        curve['y'] = None

        # x,y order does not matter here.
        # We parse sexp string by independently looking for qx# and qy# strings within sexp_public_key_xy string
        for c in curve:
            curve_value = self.extract_entry_from_sexp(keyword="q"+c, sexp=sexp_public_key_xy, binary=binary)
            if curve_value is None: raise ValueError('Could not find ec ' + c + ' curve in provided s-expression:\n' + sexp)
            if curve_value.startswith('#') and curve_value.endswith('#'):
                curve_value = curve_value.strip('#')
                curve_size = len(curve_value)//2
                curve_value = int(curve_value,16)
                assert(curve_size == 48)
            elif curve_value.startswith('48:'):
                curve_value = self.extract_entry_from_sexp(keyword="q"+c,sexp=sexp_public_key_xy, binary=binary)
                curve_value_ba = bytearray()
                curve_value_ba.extend(curve_value[3:])
                curve_size = len(curve_value_ba)
                assert(curve_size == 48)
                curve_value = Convert().bytearray_to_integer(curve_value_ba, endianness='big')
            else:
                raise ValueError('ec ' + c + ' curve has an unknown format in provided s-expression:\n' + sexp)
            curve[c] = (curve_value, curve_size)

        super(EcdsaPublicKey,self).initialize(size=(0x18+curve['x'][1]+curve['y'][1]))
        self._x_size = curve['x'][1]
        self._y_size = curve['y'][1]
        if self.magic_number() == MagicNumber.PUBLIC_KEY_SINGLE_ROOT:
            self.set_value(value=curve['x'][1], offset=0x4, size=4)
            self.set_value(value=curve['y'][1], offset=0x8, size=4)
        self.set_elliptic_curve_type(key_type)

        if self.magic_number() != MagicNumber.PUBLIC_KEY_SINGLE_ROOT:
            # fuse enablement and contribution field is new in Multi-Root Entry
            if fuse_enable is None:
                # set key fuse enablement to 0
                self.set_fuse_enable(fuse_enable=0x0)
            else:
                self.set_fuse_enable(fuse_enable=fuse_enable)

            if self._in_sigchain_pubkey_entry:
                # In Signature chain Public Key Entry, the offset 0x8 of a public key
                #   is reserved (write as 0) if enablement (i.e. in multi root format) (Section 7.6.1.3)
                # I will re-use the contrib field which is at offset 0x8 since there's no good name
                self.set_contrib(contrib=0x0)
            else:
                if contrib is None:
                    # set key contribution to all F's
                    self.set_contrib(contrib=0xFFFFFFFF)
                else:
                    self.set_contrib(contrib=contrib)

        if permission is None:
            # set key permission to all F's by default
            self.set_permission(perm=0xFFFFFFFF)
        else:
            self.set_permission(perm=permission)

        if cancel_id is None:
            # set key cancellation to all F's by default
            self.set_cancel_id(cancel_id=0xFFFFFFFF)
        else:
            self.set_cancel_id(cancel_id=cancel_id)

        self.set_value(value=curve['x'][0], offset=0x18, size=self.x_size(), endianness="big")
        self.set_value(value=curve['y'][0], offset=0x18+self.x_size(), size=self.y_size(), endianness='big')

        self.update()

        return self

    def public_key_sha384(self):
        return self.sha384(offset=0x18, size=(self.x_size() + self.y_size()))

    def msw_public_key_sha384(self):
        # The most significant Word of this Big Endian Hash is BYTE0 to BYTE4
        return self.public_key_sha384()[0:4]

    def fuse_enable(self):
        assert(self.magic_number() != MagicNumber.PUBLIC_KEY_SINGLE_ROOT)
        return self.get_value(offset=0x4, size=4)

    def set_fuse_enable(self, fuse_enable):
        assert(self.magic_number() != MagicNumber.PUBLIC_KEY_SINGLE_ROOT)
        self.set_value(fuse_enable, offset=0x4, size=4)

    def contrib(self):
        assert(self.magic_number() != MagicNumber.PUBLIC_KEY_SINGLE_ROOT)
        return self.get_value(offset=0x8, size=4)

    def set_contrib(self, contrib):
        assert(self.magic_number() != MagicNumber.PUBLIC_KEY_SINGLE_ROOT)
        self.set_value(contrib, offset=0x8, size=4)

    def permission(self):
        return self.get_value(offset=0x10, size=4)

    def set_permission(self, perm):
        self.set_value(perm, offset=0x10, size=4)

    def cancel_id(self):
        return self.get_value(offset=0x14, size=4)

    def set_cancel_id(self, cancel_id):
        self.set_value(cancel_id, offset=0x14, size=4)

    def x_size(self):
        if self.magic_number() != MagicNumber.PUBLIC_KEY_SINGLE_ROOT:
            if self._x_size is None:
                # TODO: Find a better way to set this
                self._x_size = 48
            return self._x_size
        return self.get_value(offset=0x4,size=4)

    def x(self):
        return self.get_value(offset=0x18, size=self.x_size(), endianness='big')

    def y_size(self):
        if self.magic_number() != MagicNumber.PUBLIC_KEY_SINGLE_ROOT:
            if self._y_size is None:
                # TODO: Find a better way to set this
                self._y_size = 48
            return self._y_size
        return self.get_value(offset=0x8,size=4)

    def y(self):
        return self.get_value(offset=0x18+self.x_size(), size=self.x_size(), endianness='big')

    def elliptic_curve_type(self):
        curve_magic = self.get_value(offset=0xC,size=4)
        if curve_magic == MagicNumber.ELLIPTIC_CURVE_SECP256R1:
            # prime256v1 is an alias for secp256r1
            return "prime256v1"
        elif curve_magic == MagicNumber.ELLIPTIC_CURVE_SECP384R1:
            return "secp384r1"
        else:
            return "unknown"

    def set_elliptic_curve_type(self, curve_type):
        if curve_type == "secp256r1" or curve_type == "prime256v1":
            self.set_value(value=MagicNumber.ELLIPTIC_CURVE_SECP256R1, offset=0xC, size=4)
        elif curve_type == "secp384r1":
            self.set_value(value=MagicNumber.ELLIPTIC_CURVE_SECP384R1, offset=0xC, size=4)
        else:
            raise ValueError('Eliptic Curve Type ' + str(curve_type) + ' is not valid')

    def fragment_properties_str(self):
        return_string = super(EcdsaPublicKey, self).fragment_properties_str()
        return_string += "  Curve Type: " + self.elliptic_curve_type() + "\n"
        return_string += "  X: " + hex(self.x()) + "\n"
        return_string += "  Y: " + hex(self.y()) + "\n"

        return_string += "  KeyHash (qX||qY): " + \
            Convert().bytearray_to_hex_string(byte_array=self.public_key_sha384(), endianness='big') + "\n"

        return_string += "  Permissions: " + hex(self.permission()).rstrip('L') + "\n"
        return_string += "  Cancel ID: " + hex(self.cancel_id()).rstrip('L') + "\n"
        return_string += "\n"
        return return_string

    def read(self, fp):
        if fp.closed: return
        if self.max_size is None:
            self.append(byte_array=bytearray(fp.read()))
            fp.close()
        else:
            read_size = self.max_size - self.size()
            if read_size > 0:
                # Check if the following read_size bytearray is a public key or not
                ba = bytearray(fp.peek(read_size))
                magic_num = Convert().bytearray_to_integer(ba[0:4])
                if magic_num not in MagicNumber.PUBLIC_KEY_MAGICS:
                    return

                # Read the bytearray once confirm that it is a valid public key
                self.append(bytearray(fp.read(read_size)))
                if self.size() != self.max_size:
                    fp.close()

        self.validate()

    def validate(self):
        # Public key entry can be empty in multi root format
        if self.size() == 0:
             return

        if self.elliptic_curve_type() == "unknown":
            raise ValueError('elliptic curve type is unknown')

        return super(EcdsaPublicKey,self).validate()

class EcdsaPublicKeyTest(unittest.TestCase):

    def test_constructor(self):
        key = EcdsaPublicKey()

        sexp_filename="../test/keys/ascii_key.skpi"
        with io.open(sexp_filename, "r") as fp:
            sexp = fp.read()

        key.initialize(sexp=sexp, binary=False)
        key.validate()

        print "Public Key X size is " + str(key.x_size())
        print "X: " + hex(key.x())
        self.assertEquals(key.x(), 0x593d3f98d8eacb6eea77d57920ef81649ff05d07a41cfe3fcfbaa27a57507dffab80b5eb11a28e4ca775eab53cc4faa0)
        print "Public Key Y size is " + str(key.y_size())
        print "Y: " + hex(key.y())
        self.assertEquals(key.y(), 0xa34877346cd52350d074371adbf940e587f58b95db3ef5512e929a4c1401a78933616244dccc7dde014d92a75ab761e1)
        self.assertEquals(Convert().bytearray_to_integer(key.msw_public_key_sha384()),0x39A02A37)

        print key

    def test_keypair_from_css(self):
        key = EcdsaPublicKey()

        kp_filename="../test/keys/root_p384.kp"
        with io.open(kp_filename, "rb") as fp:
            sexp_ba = bytearray(fp.read())

        sexp = str(sexp_ba)
        key.initialize(sexp=sexp, binary=True)
        key.validate()

        print "Public Key X size is " + str(key.x_size())
        print "X: " + hex(key.x())
        self.assertEquals(key.x(), 0x67515ae1dacd203dba060efbf351c922f286a13e3f2fe9c1c8a4406c339dc50486413941e616916b7662253da0c3837d)
        print "Public Key Y size is " + str(key.y_size())
        print "Y: " + hex(key.y())
        self.assertEquals(key.y(), 0x95564976bb6c7194323cf98afa22704b5ab93aa4c5af7fc39069e7a736ded7d2ff5d3e29e27224848d9cbce27ba62473)

    def test_public1_key(self):
        k = EcdsaPublicKey()
        k.initialize(sexp_file="../test/keys/public1_p384.kp", binary=True)

    def test_usb_secure_token_key(self):
        token_key = EcdsaPublicKey().initialize(sexp_file="../keys/css/PSGTest1_Debug_pubkey.bin")
        token_key.validate()
        #print hex(token_key.x())
        self.assertEquals(token_key.x(), 0x228301edba5c91c9e7f39ff21b773e67b1cacfa1814c7d8034409844557ad29b5f340d9bbed489dc40eaa9f5ef5825c0)
        #print hex(token_key.y())
        self.assertEquals(token_key.y(), 0xb9c0fa4d415ca2b9fa473eb8f4ea2b9ac61bd30cb5b93fba3577e4da328637266ab2d74f29661e80d8f53dd4fb0961e6)


if __name__ == '__main__':
    unittest.main()
