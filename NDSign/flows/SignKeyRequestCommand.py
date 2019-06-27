#!/usr/bin/env python

import unittest


from flows.Command import Command
from flows.SignCommand import SignCommand


class SignKeyRequestCommand(SignCommand):
    '''
    Create and request to Sign a Key File *.qky
    '''

    def __init__(self):
        super(SignKeyRequestCommand,self).__init__()
        self.name = 'sign_key_request'

    def add_parser(self, parser):
        super(SignKeyRequestCommand,self).add_parser(parser=parser)
        self.add_permissions_argument()
        self.add_cancel_id_argument()
        self.add_keyfile_argument()
        self.add_css_project_argument()
        self.parser.add_argument('--skip-css', action='store_true', default=False,
                                 help='Create a *.qky file without sending a request through CSS')
        self.parser.add_argument('keychainfilein', metavar='qky-to-sign-with', help="input keychain file (*.qky) to sign with")
        self.parser.add_argument('keychainfileout', metavar='qky-to-be-signed', help="output keychain file (*.qky) to pass on to the approvers")

    def execute(self, args):
        super(SignKeyRequestCommand, self).execute(args=args)
        self.execute_cmd(keypair_file=args.keyfile, \
                         qky_in=args.keychainfilein, \
                         qky_out=args.keychainfileout, \
                         perm=args.permissions, \
                         cancel_id=args.cancel_id, \
                         css_project=args.css_project, \
                         skip_css=args.skip_css, \
                         signtool=args.signtool)

    def execute_cmd(self, keypair_file, qky_in, qky_out, perm, cancel_id, css_project, skip_css, signtool):
        super(SignKeyRequestCommand, self).execute_cmd(signtool=signtool)

        self.validate_permission(perm)
        self.css.permission = perm
        self.validate_cancel_id(cancel_id)
        self.css.cancel_id = cancel_id
        self.set_sign_keychain_file(qky_in)
        self.set_css_project(css_project)

        if self.css.use_code_sign_server and not skip_css:
            self.css.css_project = css_project
        else:
            self.css.css_project = None

        if keypair_file is None:
            keypair_file = self.css.keypair_file_from_sigchain_file(sigchain_file=qky_out)

        req_id, qky_css = self.css.sign_key_request(keypair_file=keypair_file, skip_css_request=skip_css)

        qky_css.save(filename=qky_out)
        self.log(Command.SUCCESS + "KeyChain File to be approved and then signed saved to: " \
                 + qky_out + " [RequestID:" + str(req_id) + "]")

        return req_id

class CmfSignKeyRequestCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignKeyRequestCommand()
        self.assertNotEqual(cmd, None)


    def test_sign_request(self):
        from flows.CreateRootKeyCommand import CreateRootKeyCommand
        create_root_key_cmd = CreateRootKeyCommand()

        root_qky = "../work_dir/root_req.qky"
        create_root_key_cmd.execute_cmd(keypair_file=None, keychainfile=root_qky, signtool="ltsign")

        qky = "../work_dir/cs1_req.qky"
        sign_key_cmd = SignKeyRequestCommand()
        sign_key_cmd.execute_cmd(keypair_file=None, qky_in=root_qky, qky_out=qky, signtool="ltsign", perm=0x1, cancel_id=0, skip_css=False, css_project=None)


if __name__ == '__main__':
    unittest.main()