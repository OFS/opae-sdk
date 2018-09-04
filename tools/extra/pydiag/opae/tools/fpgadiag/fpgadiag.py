#! /usr/bin/env python
# Copyright(c) 2017, Intel Corporation
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
from collections import OrderedDict
from opae import fpga
from nlb0 import nlb0
from nlb3 import nlb3
from nlb7 import nlb7


class fpgadiag(object):
    modes = OrderedDict({'lpbk1': nlb0,
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
            choices=cls.modes.keys(),
            default='lpbk1')
        parser.add_argument('-h', '--help',
                            action='store_true',
                            default=False,
                            help='print help message and exit')

        args, _ = parser.parse_known_args()

        test = cls.modes[args.mode](args.mode, parser)
        return test


if __name__ == '__main__':
    logging.info("starting up")
    test = fpgadiag.create()
    if test.setup():
        tokens = test.enumerate()
        with fpga.open(tokens[0]) as handle:
            dsm = test.run(handle)
