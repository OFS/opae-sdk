#!/usr/bin/env python

import unittest

from bitstream.factory import BitstreamFactory
from flows.Command import Command
from flows.SignCommand import SignCommand
from bitstream.signature_chain import SignatureChain


class SignKeyApproveCommand(SignCommand):
    '''
    Approve a Sign Request to a Key File *.qky
    '''

    def __init__(self):
        super(SignKeyApproveCommand,self).__init__()
        self.name = 'sign_key_approve'

        self.visible_valid_approve_roles = ["reviewer", "manager", "security"]

        max_reviewers = 12
        self.valid_approve_roles = self.visible_valid_approve_roles + ["reviewer" + str(i) for i in range(1, max_reviewers+1)]

    def add_parser(self, parser):
        super(SignKeyApproveCommand,self).add_parser(parser=parser)
        self.add_request_id_argument()

        self.parser.add_argument('keychainfile', metavar='qky-to-approve', help="input keychain file (*.qky) to be approved")

        self.parser.add_argument('--approval_type', default=None,
                            type=str, choices=self.valid_approve_roles,
                            metavar='{' + ','.join(self.visible_valid_approve_roles) +'}',
                            help='specify approver role')

    def execute(self, args):
        super(SignKeyApproveCommand, self).execute(args=args)
        self.execute_cmd(qky_file=args.keychainfile, \
                         request_id=args.request_id, \
                         approval_type=args.approval_type, \
                         signtool=args.signtool)

    def execute_cmd(self, qky_file, request_id, approval_type, signtool):
        super(SignKeyApproveCommand, self).execute_cmd(signtool=signtool)

        qky = BitstreamFactory().generate_from_file(filename=qky_file)
        assert(isinstance(qky, SignatureChain))

        if request_id is None:
            request_id = input('Request ID: ')

        self.css.sign_key_approve(sign_request_id=request_id, qky_signature_chain=qky, approval_role=approval_type)

        self.log(Command.SUCCESS + "KeyChain File Approved: " \
                 + qky_file + " [RequestID:" + str(request_id) + "]")

class CmfSignKeyApproveCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignKeyApproveCommand()
        self.assertNotEqual(cmd, None)


if __name__ == '__main__':
    unittest.main()