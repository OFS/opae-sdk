#!/usr/bin/env python

import unittest

from bitstream.factory import BitstreamFactory
from bitstream.signature_chain import SignatureChain
from flows.Command import Command
from sign.css import Css


class ExtractModuleCommand(Command):
    '''
    Extract the binary blob that needs to be signed from a Given Input (qky or cmf)
    '''

    def __init__(self):
        super(ExtractModuleCommand,self).__init__()
        self.name = 'extractmodule'
        self.css = None

    def add_parser(self, parser):
        super(ExtractModuleCommand,self).add_parser(parser=parser)
        self.parser.add_argument('keychainfile', metavar='qky-file-in', help="input keychain file (*.qky) to extract from")
        self.parser.add_argument('modulefile', metavar='module-out-file', nargs='?', help="output module file (*.module) that will be sent to CSS sign server")
        self.parser.add_argument('--multi', metavar='multi-root-key-format')

    def execute(self, args):
        super(ExtractModuleCommand, self).execute(args=args)
        self.execute_cmd(file_in=args.keychainfile, module_file_out=args.modulefile, enable_multi=args.multi)

    def execute_cmd(self, file_in, module_file_out, enable_multi):
        if self.css is None: self.css = Css()
        obj_in = BitstreamFactory().generate_from_file(filename=file_in)
        if not isinstance(obj_in, SignatureChain):
            raise ValueError(self.name + " does not support this file type")

        module = self.css.extract_module_from_qky(qky_signature_chain=obj_in, enable_multi=enable_multi)
        module.validate()

        if module_file_out is None or module_file_out == '-':
            print str(module)
        else:
            self.log(module.css_hash_properties_str())
            module.save(filename=module_file_out)
            self.log(Command.SUCCESS + "Module file has been extracted and saved to: " + module_file_out)

class ExtractModuleCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = ExtractModuleCommand()
        self.assertNotEqual(cmd, None)

if __name__ == '__main__':
    unittest.main()

