#!/usr/bin/env python

import unittest

from bitstream.signature_chain_root_entry import SignatureChainRootEntry
from bitstream.signature_chain_multi_root_entry import SignatureChainMultiRootEntry
from flows.Command import Command
from flows.SignCommand import SignCommand
from sign.css import Css


class CreateRootKeyCommand(SignCommand):
    '''
    Create a Root Key File root.qky
    '''

    def __init__(self):
        super(CreateRootKeyCommand,self).__init__()
        self.name = 'create_root_key'

    def add_parser(self, parser):
        super(CreateRootKeyCommand,self).add_parser(parser=parser)
        self.add_keyfile_argument()
        self.parser.add_argument('--multi', metavar='multi-root-key-format')
        self.parser.add_argument('--hash-sel', metavar='hash-select')
        self.parser.add_argument('keychainfile', metavar='key-chain-file')

    def execute(self, args):
        super(CreateRootKeyCommand, self).execute(args=args)
        self.execute_cmd(keypair_file=args.keyfile, keychainfile=args.keychainfile, enable_multi=args.multi, hash_sel=args.hash_sel, signtool=args.signtool)

    def execute_cmd(self, keypair_file, keychainfile, signtool, enable_multi=None, hash_sel=None):
        assert(keychainfile is not None)

        css = Css()
        if self.debug: css.debug = True
        if self.verbose_level > 1: css.verbose = True
        if signtool == "css": css.use_code_sign_server = True

        if keypair_file is None:
            keypair_file = css.keypair_file_from_sigchain_file(sigchain_file=keychainfile)

        if enable_multi is None:
            # Create Single-Root Format Key
            root_key = css.create_root_key(keypair_file=keypair_file, hash_sel=hash_sel)
            log_string = "Root Key File Saved to: " + keychainfile
        else:
            # Create Multi-Root Format Key
            root_key = css.create_multi_root_key(keypair_file0=keypair_file, hash_sel=hash_sel)
            log_string = "Multi Root Key File Saved to: " + keychainfile
        root_key.save(filename=keychainfile)
        self.log(Command.SUCCESS + log_string)

        if self.debug:
            if enable_multi is None:
                readback_root_key = SignatureChainRootEntry().load(filename=keychainfile)
            else:
                readback_root_key = SignatureChainMultiRootEntry().load(filename=keychainfile)
            readback_root_key.validate()

class CreateRootKeyCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = CreateRootKeyCommand()
        self.assertNotEqual(cmd, None)
        cmd.execute_cmd(keypair_file=None, keychainfile="../work_dir/root_foo.qky", signtool="ltsign")
        cmd.execute_cmd(keypair_file=None, keychainfile="../work_dir/multi_root_foo.qky", signtool="ltsign", enable_multi=True)

if __name__ == '__main__':
    unittest.main()