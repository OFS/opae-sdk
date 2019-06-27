#!/usr/bin/env python

import unittest


from flows.Command import Command
from flows.SignCommand import SignCommand
from sign.css import Css


class SignKeyCommand(SignCommand):
    '''
    Sign a Key File *.qky
    '''

    def __init__(self):
        super(SignKeyCommand,self).__init__()
        self.name = 'sign_key'

    def add_parser(self, parser):
        super(SignKeyCommand,self).add_parser(parser=parser)
        self.add_permissions_argument()
        self.add_cancel_id_argument()
        self.add_keyfile_argument()
        self.add_signkeyfile_argument()
        self.add_css_project_argument()
        self.parser.add_argument('keychainfile', metavar='key-chain-file')
        self.parser.add_argument('signedkeychainfile', metavar='signed-key-chain-file')

    def execute(self, args):
        super(SignKeyCommand, self).execute(args=args)
        self.execute_cmd(sign_keypair_file=args.signkeyfile, \
                         sign_keychain_file=args.keychainfile, \
                         keypair_file=args.keyfile, \
                         signed_keychain_file=args.signedkeychainfile, \
                         perm=args.permissions, \
                         cancel_id=args.cancel_id, \
                         css_project=args.css_project, \
                         signtool=args.signtool)

    def execute_cmd(self, sign_keypair_file, sign_keychain_file, keypair_file, signed_keychain_file, perm, cancel_id, css_project, signtool):
        super(SignKeyCommand, self).execute_cmd(signtool=signtool)

        self.validate_permission(perm)
        self.validate_cancel_id(cancel_id)
        self.css.sign_keychain_file = sign_keychain_file
        self.css.sign_keypair_file = sign_keypair_file
        self.css.permission = perm
        self.css.cancel_id = cancel_id

        if self.css.use_code_sign_server:
            if not self.debug:
                raise ValueError(self.name + " command is not a valid command when css is enabled. Use sign_key_request, sign_key_approve, sign_key_finalize commands")

        self.set_sign_keychain_file(sign_keychain_file)
        self.set_sign_keypair_file(sign_keypair_file)
        self.set_css_project(css_project)


        if keypair_file is None:
            keypair_file = self.css.keypair_file_from_sigchain_file(sigchain_file=signed_keychain_file)

        signed_keychain = self.css.sign_key(keypair_file=keypair_file)

        signed_keychain.save(filename=signed_keychain_file)
        self.log(Command.SUCCESS + "Signed KeyChain File Saved to: " + signed_keychain_file)


class CmfSignCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignKeyCommand()
        self.assertNotEqual(cmd, None)

        css = Css()

        css.sign_keychain_file = "../work_dir/root_b.qky"
        root_key = css.create_root_key(keypair_file=css.generate_keypair_file())
        root_key.save(filename=css.sign_keychain_file)

        new_keychain = css.sign_key(keypair_file="../test/keys/public0_p384.kp")
        new_keychain.save("../work_dir/codesign_a.qky")

    def test_id_validation(self):
        cmd = SignKeyCommand()

        cmd.validate_id(0x1, "valid")
        cmd.validate_id(0xFFFFFFFF, "valid")

        try:
            cmd.validate_id(-1,"not_valid")
            self.assertTrue(False)
        except ValueError as error_msg:
            print "Exception caught as expected: " + str(error_msg)

        try:
            cmd.validate_id(0x100000000,"not_valid")
            self.assertTrue(False)
        except ValueError as error_msg:
            print "Exception caught as expected: " + str(error_msg)


if __name__ == '__main__':
    unittest.main()
