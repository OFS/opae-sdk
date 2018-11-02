#! /usr/bin/env python
# Copyright(c) 2018, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
import argparse
import logging
import sys
from collections import OrderedDict
from os.path import basename, EX_OK, EX_USAGE, EX_SOFTWARE, EX_UNAVAILABLE
from nlb0 import nlb0
from nlb3 import nlb3
from nlb7 import nlb7
# pylint: disable=E0611
from opae import fpga


class fpgadiag(object):
    mode_class = OrderedDict({'lpbk1': nlb0,
                              'read': nlb3,
                              'write': nlb3,
                              'trput': nlb3,
                              'sw': nlb7})

    @classmethod
    def create(cls):
        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument(
            '-m',
            '--mode',
            choices=cls.mode_class.keys(),
            default='lpbk1')
        parser.add_argument(
            '--loglevel',
            choices=[
                'exception',
                'error',
                'warning',
                'info',
                'debug'],
            default='warning',
            help='error level to set')
        parser.add_argument(
            '--version',
            help='print version information then quit',
            action='store_true',
            default=False)
        parser.add_argument('-h', '--help',
                            action='store_true',
                            default=False,
                            help='print help message and exit')

        args, _ = parser.parse_known_args()
        if args.version:
            print(
                "{} OPAE {}, build {}".format(
                    basename(sys.argv[0]),
                    fpga.version(),
                    fpga.build()))
            sys.exit(EX_OK)
        test = cls.mode_class[args.mode](args.mode, parser)
        test.logger.setLevel(getattr(logging, args.loglevel.upper()))
        return test


def main():
    test = fpgadiag.create()
    if not test.setup():
        sys.exit(EX_USAGE)
    tokens = test.enumerate()
    if not tokens:
        test.logger.error("Could not find suitable accelerator")
        sys.exit(EX_UNAVAILABLE)
    with fpga.open(tokens[0]) as handle:
        parent = fpga.properties(handle).parent
        with fpga.open(parent, fpga.OPEN_SHARED) as device:
            try:
                test.logger.info(
                    "{} OPAE {}, build {}".format(
                        basename(sys.argv[0]),
                        fpga.version(),
                        fpga.build()))
                test.run(handle, device)
            except KeyboardInterrupt:
                test.logger.info(
                    "User requested interrupt - ending execution")
            except RuntimeError as e:
                test.logger.error("Error running test: %s", e)
            else:
                return EX_OK
    return EX_SOFTWARE


if __name__ == '__main__':
    main()
