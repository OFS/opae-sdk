#!/usr/bin/env python

import unittest

from key import EcdsaPublicKey
from magic import MagicNumber
from signature import Signature
from signature_chain_entry import SignatureChainEntry


class SignatureChainPublicKeyEntry(SignatureChainEntry):

    def __init__(self, max_size=None, enable_multi=False):
        data = EcdsaPublicKey(enable_multi=enable_multi, in_sigchain_pubkey_entry=True)
        signature = Signature(enable_multi=enable_multi)
        super(SignatureChainPublicKeyEntry,self).__init__(data=data, signature=signature, max_size=max_size)
        self.expected_magic.append((MagicNumber.SIGNATURE_PUBLIC_KEY_ENTRY,0x0))

    def initialize(self, codesign_public_key):
        self.header().initialize(size=self.header().max_size)
        self.set_public_key(codesign_public_key)
        self.signature().initialize()
        self.update_magic()
        self.update()
        return self

    def set_public_key(self, public_key):
        assert(isinstance(public_key, EcdsaPublicKey))
        public_key.validate()
        self.fragments[1] = public_key
        assert(self.sign_keychain_file() == public_key)
        assert(self.number_of_fragments() == 3)
        assert(self.get_public_key() == public_key)

    def get_public_key(self):
        if self.data().size() == 0:
            return None
        else:
            return self.data()

    def sign_keychain_file(self):
        return self.data()

    def validate(self):

        # these assersion's are enforced by css tool and therefore validated here
        assert(self.length() == 256)
        assert(self.data_length() == 120)
        assert(self.signature_length() == 112)
        assert(self.sha_legnth() == 0)
        assert(self.get_value(offset=0x14, size=4) == 0)

        #assert(self.get_value(offset=0x90, size=4) == 0x74881520)
        #assert(self.get_value(offset=0x9C, size=4) == 0x30548820)

        if self.signature_length() != 112:
            raise ValueError('Signature length for public key entry is not 112 Bytes as expected')

        assert(self.number_of_fragments() == 3)
        return super(SignatureChainPublicKeyEntry,self).validate()

class SignatureChainPublicKeyEntryTest(unittest.TestCase):

    def test_constructor(self):

        key = EcdsaPublicKey().initialize(sexp_file="../test/keys/public0_p384.kp", binary=True)
        public_key_entry = SignatureChainPublicKeyEntry().initialize(key)
        public_key_entry.validate()

        # CSS assertions
        self.assertEqual(public_key_entry.get_value(0x0),0x92540917)
        self.assertEqual(public_key_entry.get_value(0x4),256)
        self.assertEqual(public_key_entry.get_value(0x8),120)
        self.assertEqual(public_key_entry.get_value(0xC),112)
        self.assertEqual(public_key_entry.get_value(0x10),0)
        self.assertEqual(public_key_entry.get_value(0x14),0)
        self.assertEqual(public_key_entry.get_value(0x18),0x58700660)
        self.assertEqual(public_key_entry.get_value(0x90),0x74881520)
        self.assertEqual(public_key_entry.get_value(0x94),48)
        self.assertEqual(public_key_entry.get_value(0x98),48)
        self.assertEqual(public_key_entry.get_value(0x9C),0x30548820)

        public_key_entry.save(filename="../work_dir/codesign1.module")

    def test_gen_codesign2(self):
        k = SignatureChainPublicKeyEntry().initialize(EcdsaPublicKey().initialize(sexp_file="../test/keys/public1_p384.kp"))
        k.save(filename="../work_dir/codesign2.module")

if __name__ == '__main__':
    unittest.main()
