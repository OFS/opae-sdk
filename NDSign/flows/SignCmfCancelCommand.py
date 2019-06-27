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


class SignCmfCancelCommand(SignCommand):
    '''
    Cancel a sign request for a CMF file / zip of CMF files
    '''

    def __init__(self):
        super(SignCmfCancelCommand,self).__init__()
        self.name = 'sign_cmf_cancel'

    def add_parser(self, parser):
        super(SignCmfCancelCommand,self).add_parser(parser=parser)
        self.add_css_project_argument()
        self.parser.add_argument('--id-map-file', metavar='<file that maps module hash and request id>')
        self.parser.add_argument('infile', metavar='The nadder_audit.zip file')
		
    def execute(self, args):
        super(SignCmfCancelCommand, self).execute(args=args)

        self.execute_cmd(css_project=args.css_project, signtool=args.signtool, \
						 id_map_file=args.id_map_file, cmf_file=args.infile)

    def initialize_sign_params(self, css_project):
        self.set_css_project(css_project)

    def execute_cmd(self,css_project,signtool, id_map_file,cmf_file):
        self.css = Css()
        super(SignCmfCancelCommand, self).execute_cmd(signtool=signtool)
			
        cmf = self.load_cmf(cmf_file)
        self.initialize_sign_params(css_project)

        self.css.sign_cmf_cancel(cmf,id_map_file=id_map_file)
        self.log(Command.SUCCESS + "Cancel request has been completed for: " + cmf_file)

    def load_cmf(self, cmf_filename):
        cmf = BitstreamFactory().generate_from_file(filename=cmf_filename)
        return cmf
		
class CmfSignCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignCmfCancelCommand()
        self.assertNotEqual(cmd, None)

    def test_set_css_project(self):
        cmd = SignCmfCancelCommand()

        cmd.css = Css()
        cmd.set_css_project(None)
        self.assertEqual(cmd.css.css_project, None)
        cmd.css.use_code_sign_server = True
        cmd.set_css_project(None)
        self.assertEqual(cmd.css.css_project, "bAr")

        cmd.set_css_project(None)

        self.assertEqual(cmd.css.css_project, "STRATIX10ROOTKEY")

if __name__ == '__main__':
    unittest.main()
