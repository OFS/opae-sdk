#!/usr/bin/env python

import unittest

from bitstream.engineering_cert import EngineeringCert
from bitstream.factory import BitstreamFactory
from flows.Command import Command
from flows.SignCommand import SignCommand


class SignEngCertFinalizeCommand(SignCommand):
    '''
    Create and request to Sign a Cert File *.cert
    '''

    def __init__(self):
        super(SignEngCertFinalizeCommand,self).__init__()
        self.name = 'sign_eng_cert_finalize'

    def add_parser(self, parser):
        super(SignEngCertFinalizeCommand,self).add_parser(parser=parser)
        self.add_signkeyfile_argument()
        self.add_request_id_argument()
        self.parser.add_argument('cert_infile', metavar='input-cert-to-sign', help="input eng cert file (*.cert) to sign")
        self.parser.add_argument('cert_outfile', metavar='output-signed-cert',
                                 help="output signed eng cert file (*.cert)", nargs='?')

    def execute(self, args):
        super(SignEngCertFinalizeCommand, self).execute(args=args)
        self.execute_cmd(sign_keypair_file=args.signkeyfile, \
                         request_id=args.request_id, \
                         cert_infile=args.cert_infile , \
                         cert_outfile=args.cert_outfile , \
                         signtool=args.signtool)

    def execute_cmd(self, sign_keypair_file, request_id, cert_infile, cert_outfile, signtool):
        super(SignEngCertFinalizeCommand, self).execute_cmd(signtool=signtool)

        self.set_sign_keychain_file(None)
        self.set_sign_keypair_file(sign_keypair_file, required=True)

        cert = BitstreamFactory().generate_from_file(filename=cert_infile)

        if request_id is None:
            request_id = input('Request ID: ')

        cert_signed = self.css.sign_cert_finalize(sign_request_id=request_id, cert=cert)

        if cert_outfile is None:
            cert_outfile = cert_infile

        cert_signed.save(filename=cert_outfile)
        self.log(Command.SUCCESS + "Signed EngCertChain File Saved to: " + cert_outfile)

class CmfSignEngCertFinalizeCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignEngCertFinalizeCommand()
        self.assertNotEqual(cmd, None)

if __name__ == '__main__':
    unittest.main()
