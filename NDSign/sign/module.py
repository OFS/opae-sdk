#!/usr/bin/env python

from abc import ABCMeta
from base64 import b64encode
import unittest

from bitstream.cmf_descriptor import CmfDescriptor
from bitstream.engineering_cert_header import EngineeringCertHeader
from bitstream.fragment_collection import BitstreamFragmentCollection
from bitstream.key import EcdsaPublicKey
from bitstream.signature import Signature
from bitstream.signature_chain_public_key_entry import SignatureChainPublicKeyEntry
from util.convert import Convert


class CssModule(BitstreamFragmentCollection):
    '''
    Intput and output Data Structures to CSS tools always use CssModule as a container
    '''
    __metaclass__ = ABCMeta
    def __init__(self, fragments=None, max_size=None, max_fragment_size=None):
        super(CssModule,self).__init__(fragments=fragments, max_size=max_size, max_fragment_size=max_fragment_size)

    def css_hash_properties_str(self):

        return_string = "  CSS Module SHA256-Hash: " + \
            Convert().bytearray_to_hex_string(byte_array=self.sha256(), endianness='big') + "\n"
        return_string += "  CSS Module SHA256-Hash(Base64): " + b64encode( self.sha256() ) + "\n"

        return_string += "  CSS Module SHA384-Hash: " + \
            Convert().bytearray_to_hex_string(byte_array=self.sha384(), endianness='big') + "\n"
        return_string += "  CSS Module SHA348-Hash(Base64): " + b64encode( self.sha384() ) + "\n"

        return return_string

    def fragment_properties_str(self):
        return_string = super(CssModule, self).fragment_properties_str()
        return_string += self.css_hash_properties_str()
        return return_string

class CmfDescriptorCssModule(CssModule):
    '''
    CmfDescriptorCssModule is a CmfDescriptor + Signature
    '''
    def __init__(self, cmf_descriptor=None, signature=None):

        if cmf_descriptor is None: cmf_descriptor = CmfDescriptor()
        if signature is None: signature = Signature()

        fragments = [cmf_descriptor, signature]
        super(CmfDescriptorCssModule,self).__init__(fragments=fragments, max_size=4096+112, max_fragment_size=4096)

    def cmf_descriptor(self):
        return self.fragment(0)

    def signature(self):
        return self.fragment(1)

    def initialize(self):
        self.cmf_descriptor().initialize()
        self.signature().initialize()
        return self

class EngineeringCertHeaderCssModule(CssModule):
    '''
    EngineeringCertHeaderCssModule is a EngineeringCertHeader + Signature
    '''
    def __init__(self, engineering_cert_header=None, signature=None):

        if engineering_cert_header is None: engineering_cert_header = EngineeringCertHeader()
        if signature is None: signature = Signature()

        fragments = [engineering_cert_header, signature]
        super(EngineeringCertHeaderCssModule,self).__init__(fragments=fragments, max_size=4096+112, max_fragment_size=4096)

    def engineering_cert_header(self):
        return self.fragment(0)

    def signature(self):
        return self.fragment(1)

    def initialize(self, uid, hmac, pub_key_hash):
        self.engineering_cert_header().initialize(uid, hmac, pub_key_hash)
        self.signature().initialize()
        return self

class PublicKeyCssModule(CssModule):
    '''
    PublicKeyCssModule is really a SignatureChainPublicKeyEntry decorator
    '''
    def __init__(self, sigchain_pubkey=None, enable_multi=False):
        if sigchain_pubkey is None: sigchain_pubkey = SignatureChainPublicKeyEntry(enable_multi=enable_multi)
        fragments = [sigchain_pubkey]
        super(PublicKeyCssModule,self).__init__(fragments=fragments, max_size=sigchain_pubkey.max_size)

    def public_key_entry(self):
        return self.fragment(0)

    def key(self):
        return self.public_key_entry().data()

    def signature(self):
        return self.public_key_entry().signature()

    def initialize(self, codesign_public_key):
        self.fragment(0).initialize(codesign_public_key=codesign_public_key)
        return self

class CmfDescriptorCssModuleTest(unittest.TestCase):

    def test_constructor(self):
        m = CmfDescriptorCssModule()
        self.assertNotEqual(m, None)
        m.initialize()
        m.validate()

class EngineeringCertHeaderCssModuleTest(unittest.TestCase):

    def test_constructor(self):
        m = EngineeringCertHeaderCssModule()
        self.assertNotEqual(m, None)
        m.initialize('0x1', '0x2', '0x3')
        m.validate()

class PublicKeyCssModuleTest(unittest.TestCase):

    def test_constructor(self):
        m = PublicKeyCssModule()
        self.assertNotEqual(m, None)
        key = EcdsaPublicKey().initialize(sexp_file="../test/keys/public0_p384.kp", binary=True)
        m.initialize(key)
        m.validate()

