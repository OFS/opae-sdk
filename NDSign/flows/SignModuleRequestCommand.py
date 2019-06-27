#!/usr/bin/env python

import unittest


from flows.Command import Command
from flows.SignCommand import SignCommand
from sign.module import PublicKeyCssModule


class SignModuleRequestCommand(SignCommand):
    '''
    Request to Sign a Module File *.module
    '''

    def __init__(self):
        super(SignModuleRequestCommand,self).__init__()
        self.name = 'sign_module_request'

    def add_parser(self, parser):
        super(SignModuleRequestCommand,self).add_parser(parser=parser)
        self.add_permissions_argument()
        self.add_cancel_id_argument()
        self.add_css_project_argument()
        self.parser.add_argument('--skip-css', action='store_true', default=False,
                                 help='Do not send the request through CSS')
        self.parser.add_argument('keychainfile', metavar='keychain-to-sign-with', help="input keychain file (*.qky) to sign with")
        self.parser.add_argument('modulefile', metavar='module-to-be-signed', help="the module file (*.module) to pass on to the approvers")
        self.parser.add_argument('--multi', metavar='multi-root-key-format')

    def execute(self, args):
        super(SignModuleRequestCommand, self).execute(args=args)
        self.execute_cmd(keychain_file=args.keychainfile, \
                         module_file=args.modulefile, \
                         enable_multi=args.multi, \
                         perm=args.permissions, \
                         cancel_id=args.cancel_id, \
                         css_project=args.css_project, \
                         skip_css=args.skip_css, \
                         signtool=args.signtool)

    def execute_cmd(self, keychain_file, module_file, enable_multi, perm, cancel_id, css_project, skip_css, signtool):
        super(SignModuleRequestCommand, self).execute_cmd(signtool=signtool)

        self.validate_permission(perm)
        self.css.permission = perm
        self.validate_cancel_id(cancel_id)
        self.css.cancel_id = cancel_id
        self.set_sign_keychain_file(keychain_file)
        self.set_css_project(css_project)

        if self.css.use_code_sign_server and not skip_css:
            self.css.css_project = css_project
        else:
            self.css.css_project = None

        enable_multi = True if enable_multi else False
        module = PublicKeyCssModule(enable_multi=enable_multi).load(module_file)

        req_id = self.css.sign_module_request(css_module=module)

        self.log(Command.SUCCESS + "Module File to be approved and then signed saved to: " \
                 + module_file + " [RequestID:" + str(req_id) + "]")

        return req_id

class CmfSignModuleRequestCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignModuleRequestCommand()
        self.assertNotEqual(cmd, None)


    def test_sign_module_request(self):
        pass


if __name__ == '__main__':
    unittest.main()
