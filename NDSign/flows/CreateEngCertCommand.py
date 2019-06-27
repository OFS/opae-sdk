#!/usr/bin/env python

import unittest

from flows.Command import Command
from bitstream.engineering_cert import EngineeringCert
from bitstream.factory import BitstreamFactory
from util.convert import Convert

class CreateEngCertCommand(Command):
    '''
    Generate an engineering certificate
    '''

    def __init__(self):
        super(CreateEngCertCommand,self).__init__()
        self.name='create_eng_cert'

    def add_parser(self, parser):
        super(CreateEngCertCommand,self).add_parser(parser=parser)
        parser.add_argument('keychainfile', metavar="key-chain-file", help="keychain (qky file) for the engineering root public key (this will be embedded in the cert)")
        parser.add_argument('uid', metavar="unique-id", help="Unique ID of the device the certificate will apply to (8 byte value, must be hex format starting with 0x, little-endian)")
        parser.add_argument('hmac', metavar="HMAC", help="Device identity HMAC of the device the certificate will apply to (32-byte value in hex, starting with 0x, little-endian) (See Nadder_Config_Data spec section 7.6.4)")
        parser.add_argument('outfile', metavar='out-file', help="Name of output file")

    def execute(self, args):
        super(CreateEngCertCommand,self).execute(args=args)
        self.execute_cmd(uid=args.uid, hmac=args.hmac, keychainfile=args.keychainfile, outfile=args.outfile)

    def execute_cmd(self, uid, hmac, keychainfile, outfile):
        self.gen_cert(uid, hmac, keychainfile, outfile)
        if self.debug:
            readback = BitstreamFactory().generate_from_file(filename=outfile)
            readback.validate()

    def gen_cert(self, uid, hmac, keychainfile, outfile):
        cert = EngineeringCert()
        ###
        # Temporary workaround - Andy says version should be 0 always, but for now it needs to be 1 to get through release signing.
        pub_version_num = 0
        if self.debug: print "Note: certificates created in debug mode cannot be used for release signing and vice-versa"
        else: pub_version_num = 1
        cert.initialize(uid, hmac, self.extract_pub_key_hash(keychainfile), pub_version_num)
        ###
        # The line below should be used instead of the temporary workaround block above
        #cert.initialize(uid, hmac, self.extract_pub_key_hash(keychainfile))
        cert.save(outfile)

    def extract_pub_key_hash(self, keychainfile):
        frag = BitstreamFactory().generate_from_file(filename=keychainfile)
        key = frag.signature_entry(0).get_public_key().public_key_sha384()
        return Convert().bytearray_to_hex_string(key)


class CreateEngCertCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = CreateEngCertCommand()
        self.assertNotEqual(cmd, None)

    def test_execute(self):
        cmd = CreateEngCertCommand()
        uid = '0x1102030405060708'
        hmac ='0x1102030405060708091011121314151617181920212223242526272829303132'
        keyhash = '0xB9C933A3D44B990944C12DF5DC6D17992B550113C16180C3546023AAD6144E1D0B60EC1B408BD36DD8A10913F1C3B1AF'
        cmd.execute_cmd(keychainfile="../test/keys/codesign1.qky", uid=uid, hmac=hmac,
                        outfile='../work_dir/unittest_eng_cert.cert')
        cert = BitstreamFactory().generate_from_file(filename='../work_dir/unittest_eng_cert.cert')
        cert.validate()
        self.assertEqual(uid, Convert().bytearray_to_hex_string(cert.engineering_cert_header().uid()))
        self.assertEqual(hmac, Convert().bytearray_to_hex_string(cert.engineering_cert_header().hmac()))
        self.assertEqual(keyhash, Convert().bytearray_to_hex_string(cert.engineering_cert_header().pub_key_hash()))

    def test_extract_pub_key_hash(self):
        self.assertEqual('0xB9C933A3D44B990944C12DF5DC6D17992B550113C16180C3546023AAD6144E1D0B60EC1B408BD36DD8A10913F1C3B1AF',
                         CreateEngCertCommand().extract_pub_key_hash("../test/keys/codesign1.qky"))

if __name__ == '__main__':
    unittest.main()
