#!/usr/bin/env python

from abc import ABCMeta, abstractmethod
import unittest

class Command(object):
    '''
    abstract class representing a command to execute
    '''

    SUCCESS = "[Success] "

    __metaclass__ = ABCMeta
    def __init__(self):
        self.name = None
        self.debug = False
        self.verbose_level = 1
        self.parser = None

    def help(self):
        if self.name is None:
            return ""
        else:
            return str(self.name) + ' --help for more info'

    @abstractmethod
    def add_parser(self, parser):
        self.parser = parser
        self.parser.set_defaults(func=self.execute)

    @abstractmethod
    def execute(self, args):
        if args.debug:
            self.debug = True
            self.verbose_level = 3
        else:
            if args.verbose: self.verbose_level = 2

        if args.silent: self.verbose_level = 0

    def log(self, msg, msg_verbose_level=1):
        if msg_verbose_level <= self.verbose_level:
            print msg

    def debug_log(self, msg):
        if self.debug:
            print msg

if __name__ == '__main__':
    unittest.main()