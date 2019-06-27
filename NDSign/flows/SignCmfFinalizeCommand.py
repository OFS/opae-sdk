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


class SignCmfFinalizeCommand(SignCommand):
    '''
    Finalize a sign request for a CMF file / zip of CMF files
    '''

    def __init__(self):
        super(SignCmfFinalizeCommand,self).__init__()
        self.name = 'sign_cmf_finalize'

    def add_parser(self, parser):
        super(SignCmfFinalizeCommand,self).add_parser(parser=parser)
        self.add_signkeyfile_argument()
        self.add_css_project_argument()
        self.add_css_keyhash_argument()
        self.parser.add_argument('keychainfile', metavar='key-chain-file')
        self.parser.add_argument('infile', metavar='in-file')
        self.parser.add_argument('outfile', metavar='out-file', nargs='?')
        self.parser.add_argument('--sign-request-id', metavar='<sign_request_id>')
        self.parser.add_argument('--id-map-file', metavar='<file that maps module hash and request id>')

    def execute(self, args):
        super(SignCmfFinalizeCommand, self).execute(args=args)

        self.execute_cmd(sign_keypair_file=args.signkeyfile, keychainfile=args.keychainfile, cmf_file=args.infile, \
                         signed_cmf_file=args.outfile, sign_request_id=args.sign_request_id, id_map_file=args.id_map_file, \
                         css_project=args.css_project, css_keyhash=args.css_keyhash, signtool=args.signtool)

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

    def execute_cmd(self, sign_keypair_file, keychainfile, cmf_file, signed_cmf_file, sign_request_id, id_map_file, css_project, css_keyhash, signtool):
        self.css = Css()
        super(SignCmfFinalizeCommand, self).execute_cmd(signtool=signtool)

        cmf = self.load_cmf(cmf_file)
        self.initialize_sign_params(keychainfile, sign_keypair_file, css_project, css_keyhash)

        signed_cmf = self.css.sign_cmf_finalize(cmf, sign_request_id=sign_request_id, id_map_file=id_map_file)
        self.save_cmf(cmf_filename=signed_cmf_file, cmf=signed_cmf)

    def load_cmf(self, cmf_filename):
        cmf = BitstreamFactory().generate_from_file(filename=cmf_filename)
        return cmf

    def save_cmf(self, cmf_filename, cmf):
        cmf.save(filename=cmf_filename)
        self.log(Command.SUCCESS + "Signed File Saved to: " + cmf_filename)

        if self.debug:
            readback_cmf = BitstreamFactory().generate_from_file(filename=cmf_filename)
            readback_cmf.validate()

class CmfSignCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignCmfFinalizeCommand()
        self.assertNotEqual(cmd, None)

    def test_set_css_project(self):
        cmd = SignCmfFinalizeCommand()

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
