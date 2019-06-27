#!/usr/bin/env python

import unittest

from bitstream.factory import BitstreamFactory
from flows.Command import Command
from flows.SignCommand import SignCommand
from bitstream.signature_chain import SignatureChain
from sign.module import PublicKeyCssModule


class SignModuleApproveCommand(SignCommand):
    '''
    Approve a Sign Request to a module file *.module
    '''

    def __init__(self):
        super(SignModuleApproveCommand,self).__init__()
        self.name = 'sign_module_approve'

        self.visible_valid_approve_roles = ["reviewer", "manager", "security"]

        max_reviewers = 12
        self.valid_approve_roles = self.visible_valid_approve_roles + ["reviewer" + str(i) for i in range(1, max_reviewers+1)]

    def add_parser(self, parser):
        super(SignModuleApproveCommand,self).add_parser(parser=parser)
        self.add_request_id_argument()

        self.parser.add_argument('modulefile', metavar='module-to-approve', help="input module file (*.module) to be approved")

        self.parser.add_argument('--approval_type', default=None,
                            type=str, choices=self.valid_approve_roles,
                            metavar='{' + ','.join(self.visible_valid_approve_roles) +'}',
                            help='specify approver role')
        self.parser.add_argument('--multi', metavar='multi-root-key-format')

    def execute(self, args):
        super(SignModuleApproveCommand, self).execute(args=args)
        self.execute_cmd(module_file=args.modulefile, \
                         request_id=args.request_id, \
                         approval_type=args.approval_type, \
                         enable_multi=args.multi, \
                         signtool=args.signtool)

    def execute_cmd(self, module_file, request_id, approval_type, enable_multi, signtool):
        super(SignModuleApproveCommand, self).execute_cmd(signtool=signtool)

        enable_multi = True if enable_multi else False
        module = PublicKeyCssModule(enable_multi=enable_multi).load(module_file)

        if request_id is None:
            request_id = input('Request ID: ')

        self.css.sign_module_approve(css_module=module, sign_request_id=request_id, approval_role=approval_type)

        self.log(Command.SUCCESS + "KeyChain File Approved: " \
                 + module_file + " [RequestID:" + str(request_id) + "]")

class CmfSignModuleApproveCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignModuleApproveCommand()
        self.assertNotEqual(cmd, None)


if __name__ == '__main__':
    unittest.main()