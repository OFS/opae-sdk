#!/usr/bin/env python

import unittest

from bitstream.engineering_cert import EngineeringCert
from bitstream.factory import BitstreamFactory
from flows.Command import Command
from flows.SignCommand import SignCommand
from sign.css import Css


class SignEngCertApproveCommand(SignCommand):
    '''
    Approve a Sign Request to a EngCert File *.cert
    '''

    def __init__(self):
        super(SignEngCertApproveCommand,self).__init__()
        self.name = 'sign_eng_cert_approve'

        self.visible_valid_approve_roles = Css().standard_approve_roles
        self.valid_approve_roles = Css().debug_approve_roles


    def add_parser(self, parser):
        super(SignEngCertApproveCommand,self).add_parser(parser=parser)
        self.add_request_id_argument()

        self.parser.add_argument('cert_file', metavar='cert-to-approve', help="input eng cert file (*.cert) to be approved")

        self.parser.add_argument('--approval_type', default=None,
                            type=str, choices=self.valid_approve_roles,
                            metavar='{' + ','.join(self.visible_valid_approve_roles) +'}',
                            help='specify approver role')

    def execute(self, args):
        super(SignEngCertApproveCommand, self).execute(args=args)
        self.execute_cmd(cert_file=args.cert_file, \
                         request_id=args.request_id, \
                         approval_type=args.approval_type, \
                         signtool=args.signtool)

    def execute_cmd(self, cert_file, request_id, approval_type, signtool):
        super(SignEngCertApproveCommand, self).execute_cmd(signtool=signtool)

        cert = BitstreamFactory().generate_from_file(filename=cert_file)
        assert(isinstance(cert, EngineeringCert))

        if request_id is None:
            request_id = input('Request ID: ')

        self.css.sign_cert_approve(sign_request_id=request_id, cert=cert, approval_role=approval_type)

        self.log(Command.SUCCESS + "EngCertChain File Approved: " \
                 + cert_file + " [RequestID:" + str(request_id) + "]")

class SignEngCertApproveCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignEngCertApproveCommand()
        self.assertNotEqual(cmd, None)


if __name__ == '__main__':
    unittest.main()
