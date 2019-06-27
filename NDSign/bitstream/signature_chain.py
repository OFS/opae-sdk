#!/usr/bin/env python

import unittest

from util.convert import Convert
from fragment_collection import BitstreamFragmentCollection
from key import EcdsaPublicKey
from magic import MagicNumber
from signature_chain_block0_entry import SignatureChainBlock0Entry
from signature_chain_entry import SignatureChainEntry
from signature_chain_public_key_entry import SignatureChainPublicKeyEntry
from signature_chain_root_entry import SignatureChainRootEntry
from signature_chain_multi_root_entry import SignatureChainMultiRootEntry

class SignatureChain(BitstreamFragmentCollection):
    '''
    A collection of SignatureChainEntry Objects
    '''

    def __init__(self, signature_chain_entries=None, max_size=None):
        if signature_chain_entries != None:
            for entry in signature_chain_entries:
                assert(isinstance(entry, SignatureChainEntry))

        super(SignatureChain,self).__init__(fragments=signature_chain_entries, max_size=max_size)
        self.entries_pending_read = None

    def initialize(self, fragments=None, size=0):
        self.entries_pending_read = 0
        super(SignatureChain, self).initialize()
        return self

    def number_of_signature_entries(self):
        return self.number_of_fragments()

    def signature_entry(self, index):
        assert(index >= 0 and index < self.number_of_signature_entries())
        return self.fragment(index)

    def append(self, signature_chain_entry):

        if isinstance(signature_chain_entry, SignatureChain):
            for entry in signature_chain_entry.fragments:
                self.append(entry)
            return

        assert(isinstance(signature_chain_entry, SignatureChainEntry))

        if self.number_of_signature_entries() == 0:
            assert(isinstance(signature_chain_entry, SignatureChainRootEntry))
        else:
            assert(isinstance(signature_chain_entry, SignatureChainPublicKeyEntry) or \
                   isinstance(signature_chain_entry, SignatureChainBlock0Entry))

            # Block0 entry terminates the chain, i.e. can't add to the chain once that is added
            assert(not isinstance(self.fragment(self.number_of_fragments()-1),SignatureChainBlock0Entry))

        super(SignatureChain, self).append(fragments=[signature_chain_entry])

    def last_entry(self):
        self.validate()

        if self.number_of_signature_entries() == 0:
            return None

        return self.signature_entry(self.number_of_signature_entries()-1)

    def last_key(self):
        self.validate()

        if self.number_of_signature_entries() == 0:
            return None

        if not isinstance(self.signature_entry(self.number_of_signature_entries()-1),SignatureChainBlock0Entry):
            sc_key_entry = self.signature_entry(self.number_of_signature_entries()-1)
        else:
            assert(self.number_of_signature_entries() >= 2)
            sc_key_entry = self.signature_entry(self.number_of_signature_entries()-2)

        assert(not isinstance(sc_key_entry,SignatureChainBlock0Entry))

        key = sc_key_entry.get_public_key()
        assert(isinstance(key,EcdsaPublicKey))
        key.validate()

        return key

    def validate(self):
        assert(self.entries_pending_read is not None)
        assert(self.entries_pending_read == 0)

        for f in self.fragments:
            assert(isinstance(f, SignatureChainEntry))

        if self.number_of_signature_entries() > 0:
            assert(isinstance(self.fragment(0), SignatureChainRootEntry))

        # Update: in some rare cases (signed directly with root key), the public key entry will not be present,
        #         so only check if there are at least 3 entries
        #if self.number_of_signature_entries() > 1:
        if self.number_of_signature_entries() > 2:
            assert(isinstance(self.fragment(1), SignatureChainPublicKeyEntry))

        return super(SignatureChain, self).validate()

    def read(self, fp):
        if fp.closed: return 0
        assert(self.size() == 0)

        # In Falcon Mesa's Signature Descriptor fields, there're no infomation on number of entries. 
        # Therefore, we assume entries_pending_read is None for signature chain from FM 
        while self.entries_pending_read is None or self.entries_pending_read > 0:

            if fp.closed: break
            assert(not fp.closed)

            peek_entry_header = fp.peek(8)
            if len(peek_entry_header) < 8: break

            assert(len(peek_entry_header) >= 8)
            magic = Convert().bytearray_to_integer(peek_entry_header, offset=0, num_of_bytes=4)
            length = Convert().bytearray_to_integer(peek_entry_header, offset=4, num_of_bytes=4)

            if magic == MagicNumber.SIGNATURE_SINGLE_ROOT_ENTRY:
                signature_chain_entry = SignatureChainRootEntry(max_size=length)
            elif magic == MagicNumber.SIGNATURE_MULTI_ROOT_ENTRY:
                signature_chain_entry = SignatureChainMultiRootEntry(max_size=length)
            elif magic == MagicNumber.SIGNATURE_PUBLIC_KEY_ENTRY:
                # This is a Public Key Entry
                # However, the Public Key magic number has two possible values (no enablement and enablement valid)
                # FYI, enablement valid magic number is used when the public key is part of a Multi Root Entry.
                magic_bytes = fp.peek(28)
                if len(magic_bytes) < 28: raise ValueError('Unknown Stream - Too Short')
                # Read Public Key magic (at offset 0x18)
                pubkey_magic = Convert().bytearray_to_integer(magic_bytes, offset=0x18, num_of_bytes=4)

                # Enable multi root format if magic number is PUBLIC_KEY_MULTI_ROOT
                enable_multi = pubkey_magic == MagicNumber().PUBLIC_KEY_MULTI_ROOT

                signature_chain_entry = SignatureChainPublicKeyEntry(max_size=length, enable_multi=enable_multi)

            elif magic == MagicNumber.SIGNATURE_BLOCK0_ENTRY:
                signature_chain_entry = SignatureChainBlock0Entry(max_size=length)
            else:
                if self.entries_pending_read is None:
                    break
                raise ValueError('Unknown Signature entry found')

            signature_chain_entry.read(fp)
            self.append(signature_chain_entry=signature_chain_entry)

            if self.entries_pending_read is None:
                # Block0 entry terminates the chain, i.e. can't add to the chain once that is added
                if isinstance(signature_chain_entry, SignatureChainBlock0Entry): 
                    break
            else:
                self.entries_pending_read -= 1
            
        if self.entries_pending_read is None:
            self.entries_pending_read = 0

        self.validate()
        return 0

class SignatureChainTest(unittest.TestCase):

    def test_constructor(self):
        chain = SignatureChain()
        chain.initialize()
        chain.validate()

    def test_signature_creation(self):
        sigchain0 = SignatureChain().initialize()
        sigchain0.validate()
        self.assertEqual(sigchain0.size(),0)

        test_dir = '../test'
        work_dir = '../work_dir'

        root_key = SignatureChainRootEntry().initialize(codesign_public_key=EcdsaPublicKey().initialize(sexp_file=test_dir+"/keys/root_p384.kp"))
        sigchain0.append(signature_chain_entry=root_key)
        sigchain0.validate()
        self.assertEqual(sigchain0.size(),152)
        sigchain0.save(work_dir+"/sigchain_rootkey.qky")

        sigchain1 = SignatureChain()
        #sigchain1.entries_pending_read = sigchain0.number_of_signature_entries()
        sigchain1.load(work_dir+"/sigchain_rootkey.qky")
        public_key1 = SignatureChainPublicKeyEntry().initialize(codesign_public_key=EcdsaPublicKey().initialize(sexp_file=test_dir+"/keys/public0_p384.kp"))
        sigchain1.append(public_key1)
        sigchain1.validate()
        self.assertEqual(sigchain1.size(),408)
        sigchain1.save(work_dir+"/sigchain_pubkey0.qky")
        assert(sigchain1.number_of_signature_entries() == 2)

        sigchain2 = SignatureChain()
        #sigchain2.entries_pending_read = sigchain1.number_of_signature_entries()
        sigchain2.load(work_dir+"/sigchain_pubkey0.qky")
        public_key2 = SignatureChainPublicKeyEntry().initialize(codesign_public_key=EcdsaPublicKey().initialize(sexp_file=test_dir+"/keys/public1_p384.kp"))
        sigchain2.append(public_key2)
        sigchain2.validate()
        self.assertEqual(sigchain2.size(),664)
        sigchain2.save(work_dir+"/sigchain_pubkey1.qky")

        sigchain3 = SignatureChain()
        sigchain3.entries_pending_read = sigchain2.number_of_signature_entries()
        sigchain3.load(work_dir+"/sigchain_pubkey1.qky")
        blk0 = SignatureChainBlock0Entry().initialize()
        sigchain3.append(blk0)
        sigchain3.validate()
        self.assertEqual(sigchain3.size(),800)
        sigchain3.save(work_dir+"/sigchain_final.qky")

        sigchain4 = SignatureChain()
        sigchain4.entries_pending_read = sigchain3.number_of_signature_entries()
        sigchain4.load(work_dir+"/sigchain_final.qky")
        sigchain4.validate()
        self.assertEqual(sigchain4.size(),800)

    def test_engineering_signature_creation(self):
        sigchain0 = SignatureChain().initialize()
        sigchain0.validate()
        self.assertEqual(sigchain0.size(),0)

        test_dir = '../test'
        work_dir = '../work_dir'

        root_key = SignatureChainRootEntry().initialize(hash_sel=2, codesign_public_key=EcdsaPublicKey().initialize(sexp_file=test_dir+"/keys/root_p384.kp"))
        self.assertEqual(root_key.hash_sel(), 2)
        sigchain0.append(signature_chain_entry=root_key)
        sigchain0.validate()
        self.assertEqual(sigchain0.size(),152)
        sigchain0.save(work_dir+"/sigchain_rootkey.qky")