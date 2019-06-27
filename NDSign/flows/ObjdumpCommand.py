#!/usr/bin/env python

import unittest

from flows.Command import Command
from bitstream.factory import BitstreamFactory


class ObjdumpCommand(Command):
    '''
    objdump any valid data structure
    '''

    def __init__(self):
        super(ObjdumpCommand,self).__init__()
        self.name = 'objdump'

    def add_parser(self, parser):
        super(ObjdumpCommand,self).add_parser(parser=parser)
        self.parser.add_argument('file', metavar='file')
        self.parser.add_argument('--raw', metavar='raw')

    def execute(self, args):
        super(ObjdumpCommand, self).execute(args=args)
        self.execute_cmd(filename=args.file, raw=args.raw)

    def execute_cmd(self, filename, raw=None):
        no_format = False
        if raw:
            no_format = True
        frag = BitstreamFactory().generate_from_file(filename=filename, no_format=no_format)
        print str(frag)

class ObjdumpCommandTest(unittest.TestCase):

    def test_constructor(self):
        cmd = ObjdumpCommand()
        self.assertNotEqual(cmd, None)

if __name__ == '__main__':
    unittest.main()