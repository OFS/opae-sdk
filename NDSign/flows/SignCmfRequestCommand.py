#!/usr/bin/env python

import os
import unittest

from bitstream.factory import BitstreamFactory
from flows.ServerModeCommand import ServerModeCommand
from flows.Command import Command
from flows.SignCommand import SignCommand
from keys.vault import Vault
from networking.cssserver import CssClient
from sign.css import Css


class SignCmfRequestCommand(SignCommand):
    '''
    Issue a sign request for a CMF file / zip of CMF files
    '''

    def __init__(self):
        super(SignCmfRequestCommand,self).__init__()
        self.name = 'sign_cmf_request'

    def add_parser(self, parser):
        super(SignCmfRequestCommand,self).add_parser(parser=parser)
        self.add_signkeyfile_argument()
        self.add_css_project_argument()
        self.add_css_keyhash_argument()
        self.parser.add_argument('keychainfile', metavar='key-chain-file')
        self.parser.add_argument('infile', metavar='in-file')
        self.parser.add_argument('--id-map-file', metavar='<file that maps module hash and request id>')

    def execute(self, args):
        super(SignCmfRequestCommand, self).execute(args=args)

        self.execute_cmd(sign_keypair_file=args.signkeyfile, keychainfile=args.keychainfile, cmf_file=args.infile, \
                         id_map_file=args.id_map_file, css_project=args.css_project, \
                         css_keyhash=args.css_keyhash, signtool=args.signtool)

    def initialize_sign_params(self, keychainfile, sign_keypair_file, css_project, css_keyhash):
        self.set_sign_keychain_file(keychainfile)
        self.set_sign_keypair_file(sign_keypair_file)
        self.set_css_project(css_project)
        self.set_css_keyhash(css_keyhash)
        ##################
        # JDS' laptop
        #self.css.css_secure_token_cert_keyhash = "0VZyoNq5ChG5+f++hHPUxMk4YvA="
        # JDS' desktop
        #self.css.css_secure_token_cert_keyhash = "TIqHIwDOhgg4tdAGTUyjGXOHZsM="
        ##################

    def execute_cmd(self, sign_keypair_file, keychainfile, cmf_file, id_map_file, css_project, css_keyhash, signtool):
        self.css = Css()
        super(SignCmfRequestCommand, self).execute_cmd(signtool=signtool)

        cmf = self.load_cmf(cmf_file)
        self.initialize_sign_params(keychainfile, sign_keypair_file, css_project, css_keyhash)

        self.css.sign_cmf_request(cmf, id_map_file=id_map_file)
        self.log(Command.SUCCESS + "Sign request(s) have been initiated for: " + cmf_file)

    def load_cmf(self, cmf_filename):
        cmf = BitstreamFactory().generate_from_file(filename=cmf_filename)
        return cmf

class CmfSignCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignCmfRequestCommand()
        self.assertNotEqual(cmd, None)

    def test_set_css_project(self):
        cmd = SignCmfRequestCommand()

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
