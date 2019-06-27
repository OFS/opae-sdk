#!/usr/bin/env python

import os
import unittest

from bitstream.factory import BitstreamFactory
from bitstream.zip import BitstreamZip
from bitstream.cmf import Cmf
from bitstream.signature import Signature
from flows.ServerModeCommand import ServerModeCommand
from flows.Command import Command
from flows.SignCommand import SignCommand
from keys.vault import Vault
from networking.cssserver import CssClient
from sign.css import Css
from sign.module import CmfDescriptorCssModule

#
# This allows for ephemeral signing as per bug 591232
#
class SignEphCommand(SignCommand):
    '''
    Sign a File (usually a CMF or zip)
    '''

    def __init__(self):
        super(SignEphCommand,self).__init__()
        self.name = 'sign_ephemeral'

    def add_parser(self, parser):
        super(SignEphCommand,self).add_parser(parser=parser)
        self.add_signkeyfile_argument()
        self.add_css_project_argument()
        self.add_css_keyhash_argument()
        self.add_server_argument()
        self.parser.add_argument('keychainfile', metavar='key-chain-file')
        self.parser.add_argument('infile', metavar='in-file')
        self.parser.add_argument('outfile', metavar='out-file', nargs='?')

    def execute(self, args):
        super(SignEphCommand, self).execute(args=args)

        if args.server is None:
            if 'NDSIGN_SERVER' in os.environ:
                args.server = os.environ['NDSIGN_SERVER']

        self.execute_cmd(sign_keypair_file=args.signkeyfile, keychainfile=args.keychainfile, unsigned_file=args.infile, \
                         signed_file=args.outfile, css_project=args.css_project, css_keyhash=args.css_keyhash, \
                         signtool=args.signtool, server=args.server)

    def initialize_sign_params(self, keychainfile, sign_keypair_file, css_project, css_keyhash):
        self.set_sign_keychain_file(keychainfile)
        self.set_sign_keypair_file(sign_keypair_file)
        self.set_css_project(css_project)
        self.set_css_keyhash(css_keyhash)

    def execute_cmd(self, sign_keypair_file, keychainfile, unsigned_file, signed_file, css_project, css_keyhash, signtool, server):
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

        super(SignEphCommand, self).execute_cmd(signtool=signtool)

        self.initialize_sign_params(keychainfile, sign_keypair_file, css_project, css_keyhash)
        #unsigned_data = BitstreamFactory().generate_from_file(filename=unsigned_file)
        try:
            zipInput = BitstreamZip().load(unsigned_file)
        except AssertionError:
            print("Error, " + unsigned_file + " is not a valid zip file")
            return -1

        zipOutput = BitstreamZip()

        inputmodules = {}
        for f in zipInput.fragments:
            zipOutput.append(f, zipInput.fragments[f])
            if Css.needs_signed(zipInput.fragments[f]):
                module = CmfDescriptorCssModule(cmf_descriptor=zipInput.fragments[f].cmf_descriptor(), signature=Signature().initialize())
                inputmodules[f + ".module"] = module.get_raw_byte_array()
        signed_data = self.css.sign_eph(inputmodules)

        # for each piece of data, we need to do the triple combine into the output zip file
        for f in signed_data:
            f_truc = f[:-7] #Cut of the '.module' part
            cmf_des = zipInput.fragments[f_truc].signature_descriptor().signatures()
            sig_found = 0
            for i in range(0,4):
                if cmf_des.signature_chain(i).size() == 0:
                    sig_found = 1
                    cmf_des.signature_chain(i).append(signed_data[f])
                    assert(cmf_des.signature_chain(i).size() == signed_data[f].size())
                    break
            assert(sig_found)
            zipInput.fragments[f_truc].update()
            zipInput.fragments[f_truc].validate()

        zipInput.save(filename=signed_file)
        self.log(Command.SUCCESS + "Signed File Saved to: " + signed_file)

        if self.debug:
            readback_data = BitstreamFactory().generate_from_file(filename=signed_file)
            readback_data.validate()

class SignEphCommandTest(unittest.TestCase):
    def test_execution(self):
        val = SignEphCommand()
        self.assertNotEqual(val, None)
        # No other tests are possible due to the cmf_sign tool being specific to css

if __name__ == '__main__':
    unittest.main()
