#!/usr/bin/env python

import unittest

from util.convert import Convert
from fragment import BitstreamFragment
from fragment_collection import BitstreamFragmentCollection
from key import EcdsaPublicKey
from magic import MagicNumber
from signature_chain_root_entry import SignatureChainRootEntry


class SignatureChainMultiRootEntry(SignatureChainRootEntry):

    def __init__(self, max_size=None):
        data = BitstreamFragmentCollection(fragments=[BitstreamFragment(max_size=0x8), \
                                            EcdsaPublicKey(enable_multi=True), EcdsaPublicKey(enable_multi=True), EcdsaPublicKey(enable_multi=True)])
        data.max_size = data.fragment(0).max_size + data.fragment(1).max_size + data.fragment(2).max_size + data.fragment(3).max_size
        signature = BitstreamFragment(max_size=0)
        super(SignatureChainRootEntry,self).__init__(header=None, data=data, signature=signature, max_size=max_size)
        self.expected_magic.append((MagicNumber.SIGNATURE_MULTI_ROOT_ENTRY,0x0))

        assert(self.header().max_size == 0x18)
        assert(self.data().max_size == 8 + 120 * 3)
        assert(self.signature().max_size == 0)

    def initialize(self, hash_sel=None, codesign_public_key0=None, codesign_public_key1=None, codesign_public_key2=None):
        self.header().initialize(size=self.header().max_size)
        self.data().fragment(0).initialize(size=self.data().fragment(0).max_size)

        if hash_sel:
            self.set_hash_sel(int(hash_sel))
        else:
            # Default selection is Intel.
            self.set_hash_sel(0)

        if codesign_public_key0 is None:
            raise ValueError("Missing public key. At least one public key is required for creating a Multi-Root Entry. ")
        self.set_public_key(codesign_public_key0)
        if codesign_public_key1 is not None:
            self.set_public_key(codesign_public_key1, 1)

            # Update contribution fields
            # Not necessary, because Andy is not expecting any key, stored in CSS, would have more than 1 key.
            # The following is described in Andy's doc, so I will leave it as it is.
            pubkey0 = self.get_public_key(0)
            pubkey0.set_contrib(contrib=0xFFFF0000)
            pubkey1 = self.get_public_key(1)
            pubkey1.set_contrib(contrib=0x0000FFFF)

        if codesign_public_key2 is not None:
            self.set_public_key(codesign_public_key2, 2)

            # Update contribution fields
            # Not necessary, because Andy is not expecting any key, stored in CSS, would have more than 1 key.

        self.update_magic()
        self.update()

        return self

    def set_public_key(self, public_key, public_key_index=0):
        assert(isinstance(public_key, EcdsaPublicKey))
        public_key.validate()
        assert(public_key.cancel_id() == 0xffffffff)

        self.data().fragments[public_key_index+1] = public_key
        assert(self.sign_keychain_file(public_key_index) == public_key)
        assert(self.number_of_fragments() == 3)
        self.set_msw_public_keys_hash()
        self.update()
        assert(self.get_public_key(public_key_index) == public_key)

    def sign_keychain_file(self, public_key_index=0):
        return self.data().fragments[public_key_index+1]

    def get_public_key(self, public_key_index=0):
        if self.data().size() == 0 or self.data().number_of_fragments() != 4:
            return None
        pubkey = self.data().fragment(public_key_index+1)
        if pubkey.size() == 0:
            return None
        return pubkey

    def get_public_key_size(self, public_key_index=0):
        pubkey = self.get_public_key(public_key_index)
        if pubkey is None:
            return 0
        return pubkey.size()

    def public_key_sha384(self):
        pubkeys_size = self.get_public_key_size(public_key_index=0) + \
                        self.get_public_key_size(public_key_index=1) + self.get_public_key_size(public_key_index=2)
        pubkey_sha384 = self.sha384(offset=0x20, size=(pubkeys_size))
        # The most significant Word of this Big Endian Hash is BYTE0 to BYTE4
        return pubkey_sha384[0:4]

    def set_msw_public_keys_hash(self):
        msw_hash = self.public_key_sha384()
        self.set_raw_value(byte_array=msw_hash, offset=0x18)

    def validate(self):
        if self.signature_length() != 0:
            raise ValueError('Signature length for multi root entry is not zero')

        if self.msw_public_key_hash() != Convert().bytearray_to_integer(self.public_key_sha384()):
            raise ValueError('Signature multi root entry sha384 hash is not correct')

        # 3 fragments: header, data, signature
        assert(self.number_of_fragments() == 3)

        self.validate_public_keys()

        self.header().validate()
        return super(SignatureChainRootEntry,self).validate()

    def validate_public_keys(self):
        codesign_public_key0 = self.get_public_key(public_key_index=0)
        codesign_public_key1 = self.get_public_key(public_key_index=1)
        codesign_public_key2 = self.get_public_key(public_key_index=2)

        if codesign_public_key0.cancel_id() != 0xffffffff:
            raise ValueError("Root Key Cancel ID is not set to -1. It's set to: " + str(codesign_public_key0.cancel_id()))

        if codesign_public_key1 is not None:
            if codesign_public_key1.cancel_id() != 0xffffffff:
                raise ValueError("Root Key Cancel ID is not set to -1. It's set to: " + str(codesign_public_key1.cancel_id()))

        if codesign_public_key2 is not None:
            if codesign_public_key2.cancel_id() != 0xffffffff:
                raise ValueError("Root Key Cancel ID is not set to -1. It's set to: " + str(codesign_public_key2.cancel_id()))


class SignatureChainMultiRootEntryTest(unittest.TestCase):

    def test_constructor(self):

        root_key0 = EcdsaPublicKey(enable_multi=True)
        root_key0.initialize(sexp_file="../test/keys/root_p384.kp", binary=True)
        root_key0.set_cancel_id(cancel_id=0xffffffff)

        root_key1 = EcdsaPublicKey(enable_multi=True)
        root_key1.initialize(sexp_file="../test/keys/public0_p384.kp", binary=True)
        root_key1.set_cancel_id(cancel_id=0xffffffff)

        root_key2 = EcdsaPublicKey(enable_multi=True)
        root_key2.initialize(sexp_file="../test/keys/public1_p384.kp", binary=True)
        root_key2.set_cancel_id(cancel_id=0xffffffff)

        multi_root_entry = SignatureChainMultiRootEntry()
        multi_root_entry.initialize(codesign_public_key0=root_key0, codesign_public_key1=root_key1, codesign_public_key2=root_key2)
        print multi_root_entry

        multi_root_entry.header().validate()
        multi_root_entry.data().validate()
        multi_root_entry.signature().validate()

        multi_root_entry.validate()

        multi_root_entry.save("../work_dir/multi_root.qky")

        print multi_root_entry

if __name__ == '__main__':
    unittest.main()
