#!/usr/bin/env python

import unittest


from flows.Command import Command
from flows.SignCommand import SignCommand


class CreatePublicKeyModuleCommand(SignCommand):
    '''
    Create a Module File given a public keypair file.
    '''

    def __init__(self):
        super(CreatePublicKeyModuleCommand,self).__init__()
        self.name = 'create_pubkey_module'

    def add_parser(self, parser):
        super(CreatePublicKeyModuleCommand,self).add_parser(parser=parser)
        self.add_permissions_argument()
        self.add_cancel_id_argument()
        self.add_css_project_argument()
        self.parser.add_argument('--multi', metavar='multi-root-key-format')
        self.parser.add_argument('--skip-css', action='store_true', default=False,
                                 help='Create a *.qky file without sending a request through CSS')
        self.parser.add_argument('keyfile', help='raw ECDSA public key file to be signed in sexp binary format (*.kp:ltsign,*_pubkey.bin:css)')
        self.parser.add_argument('modulefile', help='The Module File that will be created')

    def execute(self, args):
        super(CreatePublicKeyModuleCommand, self).execute(args=args)
        self.execute_cmd(keypair_file=args.keyfile, \
                         perm=args.permissions, \
                         cancel_id=args.cancel_id, \
                         css_project=args.css_project, \
                         enable_multi=args.multi, \
                         skip_css=args.skip_css, \
                         signtool=args.signtool, \
                         module_file=args.modulefile)

    def execute_cmd(self, keypair_file, perm, cancel_id, css_project, enable_multi, skip_css, signtool, module_file):
        super(CreatePublicKeyModuleCommand, self).execute_cmd(signtool=signtool)

        self.validate_permission(perm)
        self.css.permission = perm
        self.validate_cancel_id(cancel_id)
        self.css.cancel_id = cancel_id
        self.set_css_project(css_project)

        if self.css.use_code_sign_server and not skip_css:
            self.css.css_project = css_project
        else:
            self.css.css_project = None

        enable_multi = True if enable_multi else False
        module = self.css.create_pubkey_module(keypair_file=keypair_file, enable_multi=enable_multi)

        module.save(filename=module_file)
        self.log(Command.SUCCESS + "Module file has been created and saved to: " + module_file)


class CreatePublicKeyModuleCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = CreatePublicKeyModuleCommand()
        self.assertNotEqual(cmd, None)


    def test_create_pubkey_module(self):
        '''
        This unittest ensures that the .module file created by CreatePublicKeyModuleCommand is the same as
        the .module file extracted from public keychain file.
        '''
        from flows.ExtractModuleCommand import ExtractModuleCommand
        from sign.module import PublicKeyCssModule

        extract_module_cmd = ExtractModuleCommand()

        css_project = "STRATIX10LimitedSigningKEY"
        src_keypair_file = "../keys/css/STRATIX10LimitedSigningKEY_Debug_pubkey.bin"
        src_keychain_file = "../keys/css/STRATIX10LimitedSigningKEY.qky"
        extracted_module = "../work_dir/extracted_unsigned.module"
        extract_module_cmd.execute_cmd(file_in=src_keychain_file, module_file_out=extracted_module, enable_multi=False)

        created_module = "../work_dir/created_unsigned.module"
        create_pubkey_module_cmd = CreatePublicKeyModuleCommand()
        create_pubkey_module_cmd.execute_cmd(keypair_file=src_keypair_file, signtool="ltsign",\
         perm=0x1, cancel_id=0x1, enable_multi=False, skip_css=False, css_project=None, module_file=created_module)

        self.assertTrue(str(PublicKeyCssModule().load(extracted_module)) == str(PublicKeyCssModule().load(created_module)))

    def test_create_pubkey_module_with_multi_root_format(self):
        '''
        This unittest ensures that the .module file created by CreatePublicKeyModuleCommand is the same as
        the .module file extracted from multi-root formatted public keychain file.
        '''
        from flows.ExtractModuleCommand import ExtractModuleCommand
        from sign.module import PublicKeyCssModule

        extract_module_cmd = ExtractModuleCommand()

        css_project = "FalconMesaRootKey"
        src_keypair_file = "../keys/css/FalconMesaRootKey_Release_pubkey.bin"
        src_keychain_file = "../keys/css/FalconMesaRootKey.qky"
        extracted_module_file = "../work_dir/extracted_unsigned.module"
        extract_module_cmd.execute_cmd(file_in=src_keychain_file, module_file_out=extracted_module_file, enable_multi=True)

        created_module_file = "../work_dir/created_unsigned.module"
        create_pubkey_module_cmd = CreatePublicKeyModuleCommand()
        create_pubkey_module_cmd.execute_cmd(keypair_file=src_keypair_file, signtool="ltsign",\
         perm=0xffffffff, cancel_id=0xffffffff, enable_multi=True, skip_css=False, css_project=None, module_file=created_module_file)

        # Public key format can be different in a Signature chain Multi Root Entry and a Signature chain Public Key Entry
        # If we want to define the extracted module from a Signature chain Multi Root Entry
        #   as PublicKeyCSSModule, which is a wrapper around Signature chain Public Key Entry,
        #   then we need to change 0x20 to be 0 (reserved) manually to match the format in section 7.6.1.3 of Nadder_Config_Data doc.
        extracted_module = PublicKeyCssModule(enable_multi=True).load(extracted_module_file)
        extracted_module.key().set_contrib(contrib=0x0)

        created_module = PublicKeyCssModule(enable_multi=True).load(created_module_file)
        self.assertTrue(str(extracted_module) == str(created_module))

if __name__ == '__main__':
    unittest.main()