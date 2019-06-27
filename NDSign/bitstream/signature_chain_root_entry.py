#!/usr/bin/env python

import unittest

from util.convert import Convert
from fragment import BitstreamFragment
from fragment_collection import BitstreamFragmentCollection
from key import EcdsaPublicKey
from magic import MagicNumber
from signature_chain_entry import SignatureChainEntry


class SignatureChainRootEntry(SignatureChainEntry):

    def __init__(self, max_size=None):
        data = BitstreamFragmentCollection(fragments=[BitstreamFragment(max_size=0x8), EcdsaPublicKey()])
        data.max_size = data.fragment(0).max_size + data.fragment(1).max_size
        signature = BitstreamFragment(max_size=0)
        super(SignatureChainRootEntry,self).__init__(header=None, data=data, signature=signature, max_size=max_size)
        self.expected_magic.append((MagicNumber.SIGNATURE_SINGLE_ROOT_ENTRY,0x0))

        assert(self.header().max_size == 0x18)
        assert(self.data().max_size == 128)
        assert(self.signature().max_size == 0)

    def initialize(self, codesign_public_key, hash_sel=None):
        self.header().initialize(size=self.header().max_size)
        self.data().fragment(0).initialize(size=self.data().fragment(0).max_size)

        if hash_sel:
            self.set_hash_sel(int(hash_sel))
        else:
            # Default selection is Intel.
            self.set_hash_sel(0)
        
        self.set_public_key(codesign_public_key)
        self.update_magic()
        self.update()
        return self

    def set_public_key(self, public_key):
        assert(isinstance(public_key, EcdsaPublicKey))
        public_key.validate()
        assert(public_key.cancel_id() == 0xffffffff)

        self.data().fragments[1] = public_key
        assert(self.sign_keychain_file() == public_key)
        assert(self.number_of_fragments() == 3)
        self.set_raw_value(byte_array=self.sign_keychain_file().msw_public_key_sha384(), offset=0x18)
        self.update()
        assert(self.get_public_key() == public_key)

    def get_public_key(self):
        if self.data().size() == 0 or self.data().number_of_fragments() != 2:
            return None
        else:
            return self.data().fragment(1)

    def sign_keychain_file(self):
        return self.data().fragments[1]

    def msw_public_key_hash(self):
        return self.get_value(offset=0x18)

    def public_key_sha384(self):
        return self.sign_keychain_file().public_key_sha384()

    def validate(self):
        if self.signature_length() != 0:
            raise ValueError('Signature length for root entry is not zero')

        if self.msw_public_key_hash() != Convert().bytearray_to_integer(self.sign_keychain_file().msw_public_key_sha384()):
            raise ValueError('Signature root entry sha384 hash is not correct')

        if self.get_public_key().cancel_id() != 0xffffffff:
            raise ValueError("Root Key Cancel ID is not set to -1. It's set to: " + str(self.get_public_key().cancel_id()))

        # CASE:458059 validation
        assert(self.get_value(offset=0x34) == 0xffffffff)

        assert(self.number_of_fragments() == 3)
        self.header().validate()
        return super(SignatureChainRootEntry,self).validate()

    def efuses(self):
        public_key_hash = self.public_key_sha384()

        efuses = []
        for i in range(0, len(public_key_hash), 4):
            efuses.append(hex(Convert().bytearray_to_integer(byte_array=public_key_hash, offset=i, num_of_bytes=4, endianness="little")).rstrip("L"))
        return efuses

    def fragment_properties_str(self):
        return_string = super(SignatureChainRootEntry, self).fragment_properties_str()
        return_string += "  SHA384 of Public Key: " + \
            Convert().bytearray_to_hex_string(self.public_key_sha384(), endianness='big') + \
            " (Big Endian)\n"

        return_string += "  Efuses: " + str(self.efuses()) + "\n"
        return return_string

class SignatureChainRootEntryTest(unittest.TestCase):

    def test_constructor(self):

        root_key = EcdsaPublicKey()
        root_key.initialize(sexp_file="../test/keys/root_p384.kp", binary=True)
        root_key.set_cancel_id(cancel_id=0xffffffff)

        root_entry = SignatureChainRootEntry()
        root_entry.initialize(codesign_public_key=root_key)
        print root_entry

        root_entry.header().validate()
        root_entry.data().validate()
        root_entry.signature().validate()

        root_entry.validate()

        root_entry.save("../work_dir/root.qky")

        print root_entry

if __name__ == '__main__':
    unittest.main()
