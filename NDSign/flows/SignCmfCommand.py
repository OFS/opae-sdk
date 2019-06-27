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


class SignCmfCommand(SignCommand):
    '''
    Sign a CMF File
    '''

    def __init__(self):
        super(SignCmfCommand,self).__init__()
        self.name = 'sign_cmf'

    def add_parser(self, parser):
        super(SignCmfCommand,self).add_parser(parser=parser)
        self.add_signkeyfile_argument()
        self.add_css_project_argument()
        self.add_css_keyhash_argument()
        self.add_server_argument()
        self.parser.add_argument('keychainfile', metavar='key-chain-file')
        self.parser.add_argument('infile', metavar='in-file')
        self.parser.add_argument('outfile', metavar='out-file', nargs='?')

    def execute(self, args):
        super(SignCmfCommand, self).execute(args=args)

        if args.server is None:
            if 'NDSIGN_SERVER' in os.environ:
                args.server = os.environ['NDSIGN_SERVER']

        self.execute_cmd(sign_keypair_file=args.signkeyfile, keychainfile=args.keychainfile, cmf_file=args.infile, \
                         signed_cmf_file=args.outfile, css_project=args.css_project, css_keyhash=args.css_keyhash, \
                         signtool=args.signtool, server=args.server)

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

    def execute_cmd(self, sign_keypair_file, keychainfile, cmf_file, signed_cmf_file, css_project, css_keyhash, signtool, server):
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
        super(SignCmfCommand, self).execute_cmd(signtool=signtool)

        cmf = self.load_cmf(cmf_file)
        self.initialize_sign_params(keychainfile, sign_keypair_file, css_project, css_keyhash)

        signed_cmf = self.css.sign_cmf(cmf)
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
        cmd = SignCmfCommand()
        self.assertNotEqual(cmd, None)

    def test_execution(self):
        cmd = SignCmfCommand()
        cmd.execute_cmd(sign_keypair_file="../test/keys/public0_p384.kp", keychainfile="../test/keys/codesign1.qky", \
                        cmf_file="../test/files/example.cmf", signed_cmf_file="../work_dir/unittest_signed.cmf", \
                        css_project=None, css_keyhash=None, signtool="ltsign", server=None)

    def test_set_css_project(self):
        cmd = SignCmfCommand()

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
