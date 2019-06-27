#!/usr/bin/env python
from abc import ABCMeta, abstractmethod
import os
import unittest

from flows.Command import Command
from keys.vault import Vault
from sign.css import Css


class SignCommand(Command):

    __metaclass__ = ABCMeta
    def __init__(self):
        super(SignCommand,self).__init__()
        self.css = None

    @abstractmethod
    def add_parser(self, parser):
        super(SignCommand,self).add_parser(parser=parser)
        self.add_signtool_argument()

    @abstractmethod
    def execute(self, args):
        super(SignCommand, self).execute(args=args)

    @abstractmethod
    def execute_cmd(self, signtool):
        if self.css is None: self.css = Css()
        assert(self.css is not None)
        if self.debug: self.css.debug = True
        if self.verbose_level > 1: self.css.verbose = True
        if signtool == "css": self.css.use_code_sign_server = True

    def add_signkeyfile_argument(self):
        self.parser.add_argument('--signkeyfile', help='raw key pair file (*.kp) used to perform signing operation (ltsign only)')

    def add_signtool_argument(self):
        self.parser.add_argument('--signtool', dest="signtool", default='css', \
                            type=str, required=False, choices=["ltsign", "css"],
                            help='specify tool used to perform signing (default is css)')

    def add_keyfile_argument(self):
        self.parser.add_argument('--keyfile', help='raw ECDSA public key file to be signed in sexp binary format (*.kp:ltsign,*_pubkey.bin:css)')

    def add_request_id_argument(self):
        self.parser.add_argument('--request_id', default=None, \
                            type=int, help='sign request id value that is produced by the sign_key_request command')

    def add_permissions_argument(self):
        self.parser.add_argument('--permissions', metavar='<perm>', \
                                 type=lambda x: int(x,0), default=0x1, help='permissions id in HEX value (default is 0x1)')

    def add_cancel_id_argument(self):
        self.parser.add_argument('--cancel_id', metavar='<cancel_id>', \
                                 type=lambda x: int(x,0), default=0x0, help='cancel id in HEX value (default is 0x0)')

    def add_css_project_argument(self):
        self.parser.add_argument('--css-project', metavar='{STRATIX10ROOTKEY,STRATIX10LimitedSigningKEY}', default=None, \
                            type=str, required=False,
                            help='css project that maps to the signing key used to perform signing operation (css only)')

    def add_css_keyhash_argument(self):
        self.parser.add_argument('--css-keyhash', metavar='<subject_keyhash>', default=None, \
                            type=str, required=False,
                            help='Css keyhash string in base64 format to specify which specific cert to use. Required only if you have multiple certs for the same purpose (css only)')

    def add_server_argument(self):
        self.parser.add_argument('--server', metavar='<sign_server>', default=None, \
                            type=str, required=False, help='Specify a Signing Server')

    def validate_id(self, id_value, name):
        if id_value >= 0x100000000:
            raise ValueError('id value for ' + name + ' is larger than 32 bits: ' + str(id_value))
        if id_value < 0:
            raise ValueError('id value for ' + name + ' is less than zero: ' + str(id_value))

    def validate_permission(self, perm):
        self.validate_id(perm, "permission")

    def validate_cancel_id(self, cancel_id):
        self.validate_id(cancel_id, "cancel_id")

    def set_sign_keypair_file(self,sign_keypair_file,required=False):
        if self.css.use_code_sign_server:
            self.css.sign_keypair_file = None
            if sign_keypair_file is not None:
                raise ValueError('--signkeyfile arg is only required when using ltsign')
        else:
            if required and sign_keypair_file is None:
                raise ValueError('--signkeyfile arg is required when using ltsign')
            self.css.sign_keypair_file = sign_keypair_file

    def set_css_project(self,css_project):
        if self.css.use_code_sign_server:
            self.css.css_project = css_project
            if self.css.css_project is None and self.css.sign_keychain_file is not None:
                # ToDo: Make this better using Vault()
                self.css.css_project = os.path.basename(self.css.sign_keychain_file)

                if self.css.css_project.endswith('.qky'):
                    self.css.css_project = os.path.splitext(self.css.css_project)[0]
        else:
            self.css.css_project = None
            if css_project is not None:
                raise ValueError('--css-project arg is only required when using css')

    def set_css_keyhash(self,css_keyhash):
        if self.css.use_code_sign_server:
            self.css.css_secure_token_cert_keyhash = css_keyhash
        else:
            self.css.css_secure_token_cert_keyhash = None
            if css_keyhash is not None:
                raise ValueError('--css-keyhash arg is only required when using css')

    def set_sign_keychain_file(self,keychainfile):
        self.css.sign_keychain_file = keychainfile
        if self.css.sign_keychain_file is not None and \
            not os.path.isfile(self.css.sign_keychain_file):

            key_vault = Vault()
            keys_found = key_vault.search_by_filename(self.css.sign_keychain_file)

            if len(keys_found) == 1:
                self.css.sign_keychain_file = keys_found[0]

class SignCommandTest(unittest.TestCase):
    def test_something(self):
        pass

if __name__ == '__main__':
    unittest.main()
