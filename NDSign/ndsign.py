#!/usr/bin/env python

import argparse
import sys

from flows.ServerModeCommand import ServerModeCommand
from flows.CreateRootKeyCommand import CreateRootKeyCommand
from flows.ExtractModuleCommand import ExtractModuleCommand
from flows.ObjdumpCommand import ObjdumpCommand
from flows.SignCmfApproveCommand import SignCmfApproveCommand
from flows.SignCmfCommand import SignCmfCommand
from flows.SignEphCommand import SignEphCommand
from flows.SignCmfFinalizeCommand import SignCmfFinalizeCommand
from flows.SignCmfRequestCommand import SignCmfRequestCommand
from flows.SignKeyApproveCommand import SignKeyApproveCommand
from flows.SignKeyCommand import SignKeyCommand
from flows.SignKeyFinalizeCommand import SignKeyFinalizeCommand
from flows.SignKeyRequestCommand import SignKeyRequestCommand
from flows.SignCmfCancelCommand import SignCmfCancelCommand
#from flows.SignKeyCancelCommand import SignKeyCancelCommand
#from flows.SignEngCertCancelCommand import SignEngCertCancelCommand
from flows.CreateEngCertCommand import CreateEngCertCommand
from flows.SignEngCertCommand import SignEngCertCommand
from flows.SignEngCertRequestCommand import SignEngCertRequestCommand
from flows.SignEngCertApproveCommand import SignEngCertApproveCommand
from flows.SignEngCertFinalizeCommand import SignEngCertFinalizeCommand
from flows.CreatePublicKeyModuleCommand import CreatePublicKeyModuleCommand
from flows.SignModuleRequestCommand import SignModuleRequestCommand
from flows.SignModuleApproveCommand import SignModuleApproveCommand
from flows.SignModuleFinalizeCommand import SignModuleFinalizeCommand

class NadderSign(object):
    SubCommands = [CreateRootKeyCommand(), CreatePublicKeyModuleCommand(), \
                   ExtractModuleCommand(), ObjdumpCommand(), \
                   SignCmfCommand(), SignCmfRequestCommand(), SignCmfApproveCommand(), \
                   SignCmfFinalizeCommand(), SignKeyCommand(), SignKeyApproveCommand(), \
                   SignKeyFinalizeCommand(),SignCmfCancelCommand(), \
				   SignKeyRequestCommand(), ServerModeCommand(), \
                   CreateEngCertCommand(), SignEngCertCommand(), SignEngCertRequestCommand(), \
                   SignEngCertApproveCommand(), SignEngCertFinalizeCommand(), SignEphCommand(), \
                   SignModuleRequestCommand(), SignModuleApproveCommand(), SignModuleFinalizeCommand()]

    Version = '2.9'

    def __init__(self):
        self.commands = NadderSign.SubCommands

    def argparse(self):
        parser = argparse.ArgumentParser()
        parser.prog = 'ndsign'
        parser.description = 'Sign Stratix 10 Data Structures'
        parser.add_argument('-v', '--version', action='version', version='%(prog)s [Version '+ NadderSign.Version + ']')
        parser.add_argument('--debug', action='store_true', help='work in debug mode to help debug issues with this utility')
        parser.add_argument('-s', '--silent', action='store_true', help='keep quiet')
        parser.add_argument('--verbose', action='store_true', help='be verbose')

        if len(self.commands) > 0:
            subparsers = parser.add_subparsers(title='commands', metavar='{command}', help='command help', \
                                               description='Run "' + parser.prog + \
                            ' {command} --help" to learn how to use each of the following subcommands. ' + \
                            'Here is a list of the supported subcommands:')
            for cmd in self.commands:
                subparser = subparsers.add_parser(cmd.name, help=cmd.help())
                cmd.add_parser(parser=subparser)

        self.args = parser.parse_args()

    def run(self):
        self.argparse()

        if not self.args.silent and self.args.verbose:
            print "Nadder Signing Tool " + "[Version " + NadderSign.Version +"]"

        if self.args.debug: print " [DEBUG] Args: " + str(self.args)

        try:
            self.compatibility_check()

            self.args.func(self.args)

        except ValueError as error_msg:
            print "Oops! Something bad happened. Run with --debug if you want a stack trace of this error."
            print "*** [ERROR] *** " + str(error_msg)
            if self.args.debug: raise
            exit(1)

    def compatibility_check(self):

        if sys.version_info < (2,7):
            raise ValueError('A Python version >= 2.7 is required. Your Python Version is too old: ' + str(sys.version_info))

if __name__ == '__main__':
    ndSign = NadderSign()
    ndSign.run()
