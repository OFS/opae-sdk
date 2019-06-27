#!/usr/bin/env python

import os
import unittest

from bitstream.engineering_cert import EngineeringCert
from bitstream.factory import BitstreamFactory
from flows.Command import Command
from flows.SignCommand import SignCommand
from keys.vault import Vault
from networking.cssserver import CssClient
from sign.css import Css
from util.convert import Convert

class SignEngCertCommand(SignCommand):
    '''
    Sign an engineering certificate
    '''

    def __init__(self):
        super(SignEngCertCommand,self).__init__()
        self.name='sign_eng_cert'

    def add_parser(self, parser):
        super(SignEngCertCommand,self).add_parser(parser=parser)
        self.add_signkeyfile_argument()
        self.add_css_project_argument()
        self.add_css_keyhash_argument()
        self.add_server_argument()
        parser.add_argument('keychainfile', metavar="key-chain-file", help="keychain to use for signing (.qky file of the key signing the cert)")
        parser.add_argument('cert', metavar="in-file", help="engineering cert to sign (created from create_eng_cert)")
        parser.add_argument('outfile', metavar='out-file', help="Name of output file")

    def execute(self, args):
        super(SignEngCertCommand,self).execute(args=args)
        if args.server is None and 'NDSIGN_SERVER' in os.environ: args.server = os.environ['NDSIGN_SERVER']
        self.execute_cmd(cert=args.cert, keychainfile=args.keychainfile, outfile=args.outfile,
                         sign_keypair_file=args.signkeyfile, css_project=args.css_project, css_keyhash=args.css_keyhash,
                         signtool=args.signtool, server=args.server)

    def execute_cmd(self, cert, keychainfile, outfile, sign_keypair_file, css_project, css_keyhash, signtool, server):
        self.sign_cert(sign_keypair_file, keychainfile, cert, outfile,css_project,css_keyhash,signtool,server)

    # TODO: There's a lot in common with SignCmfCommand here, but maybe that's ok. Maybe SignCommand should have a helper that
    #       can be called for the initial setup?
    def sign_cert(self, sign_keypair_file, keychainfile, unsigned_input, outfile, css_project, css_keyhash, signtool, server):
        if server is not None:
            self.css = CssClient()
            server_list = server.split(':')
            for srv in server_list:
                port_servername = srv.split('@',1)
                if len(port_servername) == 1:
                    port = ServerModeCommand.DEFAULT_PORT
                    servername = port_servername[0]
                else:
                    assert(len(port_servername) == 2)
                    port = int(port_servername[0])
                    servername = port_servername[1]
                self.log('Adding Server ' + servername + ' on port ' + str(port), msg_verbose_level=2)
                self.css.add_server(hostname=servername, port=port)
        else:
            self.css = Css()
        super(SignEngCertCommand, self).execute_cmd(signtool=signtool)

        cert = self.load_cert(unsigned_input)
        self.initialize_sign_params(keychainfile, sign_keypair_file, css_project, css_keyhash)

        signed_cert = self.css.sign_cert(cert)
        self.save_cert(filename=outfile, cert=signed_cert)

    def load_cert(self, filename):
        cert = BitstreamFactory().generate_from_file(filename=filename)
        return cert

    def save_cert(self, filename, cert):
        cert.save(filename=filename)
        self.log(Command.SUCCESS + "Signed File Saved to: " + filename)

        if self.debug:
            readback = BitstreamFactory().generate_from_file(filename=filename)
            readback.validate()

    def initialize_sign_params(self, keychainfile, sign_keypair_file, css_project, css_keyhash):
        self.set_sign_keychain_file(keychainfile)
        self.set_sign_keypair_file(sign_keypair_file)
        self.set_css_project(css_project)
        self.set_css_keyhash(css_keyhash)


class SignEngCertCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignEngCertCommand()
        self.assertNotEqual(cmd, None)

    def test_execute(self):
        cmd = SignEngCertCommand()
        orig = BitstreamFactory().generate_from_file(filename='../test/files/example_eng_cert.cert')
        orig_hash =  orig.signature_descriptor().block0_hash()
        cmd.execute_cmd(css_project=None, css_keyhash=None, signtool='ltsign', server=None,
                        sign_keypair_file='../test/keys/public0_p384.kp',
                        keychainfile="../test/keys/codesign1.qky",
                        cert='../test/files/example_eng_cert.cert',
                        outfile='../work_dir/example_cert.cert.signed')
        cert = BitstreamFactory().generate_from_file(filename='../work_dir/example_cert.cert.signed')

        # The full header match really checks these again, but it's nice to specifically check some things
        self.assertEqual(orig.engineering_cert_header().uid(), cert.engineering_cert_header().uid())
        self.assertEqual(orig.engineering_cert_header().hmac(), cert.engineering_cert_header().hmac())
        self.assertEqual(orig.engineering_cert_header().pub_key_hash(), cert.engineering_cert_header().pub_key_hash())
        self.assertEqual(orig.signature_descriptor().block0_hash(), cert.signature_descriptor().block0_hash())

        # Check that the header matches and the signature descriptor does not.
        self.assertEqual(orig.engineering_cert_header().get_raw_byte_array(), cert.engineering_cert_header().get_raw_byte_array())
        self.assertNotEqual( orig.signature_descriptor().get_raw_byte_array(), cert.signature_descriptor().get_raw_byte_array())

    def test_set_css_project(self):
        cmd = SignEngCertCommand()

        cmd.css = Css()
        cmd.css.sign_keychain_file = "foo/bAr.qky"
        cmd.set_css_project(None)
        self.assertEqual(cmd.css.css_project, None)
        cmd.css.use_code_sign_server = True
        cmd.set_css_project(None)
        self.assertEqual(cmd.css.css_project, "bAr")

        cmd.set_sign_keychain_file("css\\STRATIX10ROOTKEY.qky")
        print cmd.css.sign_keychain_file
        cmd.set_css_project(None)

        self.assertEqual(cmd.css.css_project, "STRATIX10ROOTKEY")
        self.assertEqual(cmd.css.sign_keychain_file, os.path.join(Vault().root_dir()) + '/css/STRATIX10ROOTKEY.qky')

if __name__ == '__main__':
    unittest.main()
