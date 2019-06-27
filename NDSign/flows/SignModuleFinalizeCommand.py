#!/usr/bin/env python

import unittest

from bitstream.factory import BitstreamFactory
from flows.Command import Command
from flows.SignCommand import SignCommand
from sign.module import PublicKeyCssModule


class SignModuleFinalizeCommand(SignCommand):
    '''
    Finalize the request of signing a module file *.module
    '''

    def __init__(self):
        super(SignModuleFinalizeCommand,self).__init__()
        self.name = 'sign_module_finalize'

    def add_parser(self, parser):
        super(SignModuleFinalizeCommand,self).add_parser(parser=parser)
        self.add_request_id_argument()
        self.parser.add_argument('keychainfilein', metavar='input-qky-to-sign-with', help="input keychain file (*.qky) to sign with")
        self.parser.add_argument('modulefile', metavar='input-module-to-sign', help="input module file (*.module) to sign")
        self.parser.add_argument('keychainfileout', metavar='output-signed-qky', help="output signed keychain file (*.qky)")
        self.parser.add_argument('--multi', metavar='multi-root-key-format')

    def execute(self, args):
        super(SignModuleFinalizeCommand, self).execute(args=args)
        self.execute_cmd(request_id=args.request_id, \
                         qky_in=args.keychainfilein, \
                         module_file=args.modulefile, \
                         qky_out=args.keychainfileout, \
                         enable_multi=args.multi, \
                         signtool=args.signtool)

    def execute_cmd(self, request_id, qky_in, module_file, qky_out, enable_multi, signtool):
        super(SignModuleFinalizeCommand, self).execute_cmd(signtool=signtool)

        self.set_sign_keychain_file(qky_in)

        if request_id is None:
            request_id = input('Request ID: ')

        enable_multi = True if enable_multi else False
        module = PublicKeyCssModule(enable_multi=enable_multi).load(module_file)
        signed_module_file = module_file.rsplit('.', 1)[0] + "_signed" + ".module"

        # Create a complete Signature Chain
        qky_signed = self.css.complete_sigchain_with_approved_module(sign_request_id=request_id, \
            module=module, signed_module_file=signed_module_file, enable_multi=enable_multi)

        # Save the Signature Chain as *.qky file
        qky_signed.save(filename=qky_out)

        self.log(Command.SUCCESS + "Signed KeyChain File Saved to: " + qky_out)

class CmfSignModuleFinalizeCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignModuleFinalizeCommand()
        self.assertNotEqual(cmd, None)

if __name__ == '__main__':
    unittest.main()