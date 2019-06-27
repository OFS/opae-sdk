#!/usr/bin/env python

import unittest

from bitstream.factory import BitstreamFactory
from flows.Command import Command
from flows.SignCommand import SignCommand


class SignKeyFinalizeCommand(SignCommand):
    '''
    Create and request to Sign a Key File *.qky
    '''

    def __init__(self):
        super(SignKeyFinalizeCommand,self).__init__()
        self.name = 'sign_key_finalize'

    def add_parser(self, parser):
        super(SignKeyFinalizeCommand,self).add_parser(parser=parser)
        self.add_signkeyfile_argument()
        self.add_request_id_argument()
        self.parser.add_argument('keychainfilein', metavar='input-qky-to-sign', help="input keychain file (*.qky) to sign")
        self.parser.add_argument('keychainfileout', metavar='output-signed-qky',
                                 help="output signed keychain file (*.qky)", nargs='?')

    def execute(self, args):
        super(SignKeyFinalizeCommand, self).execute(args=args)
        self.execute_cmd(sign_keypair_file=args.signkeyfile, \
                         request_id=args.request_id, \
                         qky_in=args.keychainfilein, \
                         qky_out=args.keychainfileout, \
                         signtool=args.signtool)

    def execute_cmd(self, sign_keypair_file, request_id, qky_in, qky_out, signtool):
        super(SignKeyFinalizeCommand, self).execute_cmd(signtool=signtool)

        self.set_sign_keychain_file(None)
        self.set_sign_keypair_file(sign_keypair_file, required=True)

        qky_signature_chain = BitstreamFactory().generate_from_file(filename=qky_in)

        if request_id is None:
            request_id = input('Request ID: ')

        qky_signed = self.css.sign_key_finalize(sign_request_id=request_id, qky_signature_chain=qky_signature_chain)

        if qky_out is None:
            qky_out = qky_in

        qky_signed.save(filename=qky_out)
        self.log(Command.SUCCESS + "Signed KeyChain File Saved to: " + qky_out)

class CmfSignKeyFinalizeCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignKeyFinalizeCommand()
        self.assertNotEqual(cmd, None)

if __name__ == '__main__':
    unittest.main()