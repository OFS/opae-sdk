#!/usr/bin/env python

import unittest


from bitstream.engineering_cert import EngineeringCert
from bitstream.factory import BitstreamFactory
from flows.Command import Command
from flows.SignCommand import SignCommand
from util.convert import Convert

class SignEngCertRequestCommand(SignCommand):
    '''
    Create and Request to sign a EngCert File *.cert
    '''

    def __init__(self):
        super(SignEngCertRequestCommand,self).__init__()
        self.name = 'sign_eng_cert_request'

    def add_parser(self, parser):
        super(SignEngCertRequestCommand,self).add_parser(parser=parser)
        self.add_css_project_argument()
        parser.add_argument('--skip-css', action='store_true', default=False, help='Create a *.cert file without sending a request through CSS')
        parser.add_argument('keychainfile', metavar='qky-to-sign-with', help="input keychain file (*.qky) to sign with")
        parser.add_argument('certfilein', metavar="cert-file-in", help="The cert file to sign. This will be modified and rewritten out to the output cert-to-be-signed file")
        parser.add_argument('certfileout', metavar='cert-to-be-signed', help="output eng cert file (*.cert) to pass on to reviewers.")

    def execute(self, args):
        super(SignEngCertRequestCommand, self).execute(args=args)
        self.execute_cmd(qky_in=args.keychainfile,
                         cert_in=args.certfilein,
                         cert_out=args.certfileout,
                         css_project=args.css_project,
                         skip_css=args.skip_css,
                         signtool=args.signtool)

    def execute_cmd(self, qky_in, cert_in, cert_out, css_project, skip_css, signtool):
        super(SignEngCertRequestCommand, self).execute_cmd(signtool=signtool)

        cert = BitstreamFactory().generate_from_file(filename=cert_in)
        assert(isinstance(cert, EngineeringCert))

        self.set_css_project(css_project)
        self.set_sign_keychain_file(qky_in)

        if self.css.use_code_sign_server and not skip_css:
            self.css.css_project = css_project
        else:
            self.css.css_project = None

        req_id, cert_css = self.css.sign_cert_request(cert=cert, skip_css_request=skip_css)

        cert_css.save(filename=cert_out)
        self.log(Command.SUCCESS + "EngCert File to be approved and then signed saved to: " \
                 + cert_out + " [RequestID:" + str(req_id) + "]")

        return req_id

class SignEngCertRequestCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = SignEngCertRequestCommand()
        self.assertNotEqual(cmd, None)


    def test_sign_request(self):
        from flows.CreateRootKeyCommand import CreateRootKeyCommand
        create_root_key_cmd = CreateRootKeyCommand()

        root_qky = "../work_dir/root_req.qky"
        create_root_key_cmd.execute_cmd(keypair_file=None, keychainfile=root_qky, signtool="ltsign")

        cert = "../work_dir/example_eng_cert_sign_request.cert"
        sign_req_cmd = SignEngCertRequestCommand()
        sign_req_cmd.execute_cmd(qky_in=root_qky, cert_in='../test/files/example_eng_cert.cert', cert_out=cert+'.1', signtool="ltsign", skip_css=False, css_project=None)
        sign_req_cmd.execute_cmd(qky_in=root_qky, cert_in='../test/files/example_eng_cert.cert', cert_out=cert+'.2', signtool="css", skip_css=True, css_project=None)

        cert1 = BitstreamFactory().generate_from_file(filename=cert+'.1')
        self.assertEqual(cert1.engineering_cert_header().uid(), cert1.engineering_cert_header().uid())
        self.assertEqual(cert1.engineering_cert_header().hmac(), cert1.engineering_cert_header().hmac())
        self.assertEqual(cert1.engineering_cert_header().pub_key_hash(), cert1.engineering_cert_header().pub_key_hash())

        cert2 = BitstreamFactory().generate_from_file(filename=cert+'.2')
        self.assertEqual(cert2.engineering_cert_header().uid(), cert2.engineering_cert_header().uid())
        self.assertEqual(cert2.engineering_cert_header().hmac(), cert2.engineering_cert_header().hmac())
        self.assertEqual(cert2.engineering_cert_header().pub_key_hash(), cert2.engineering_cert_header().pub_key_hash())


if __name__ == '__main__':
    unittest.main()
