#! /usr/bin/env python3
# Copyright(c) 2017-2023 Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
# may be used to  endorse or promote  products derived  from this  software
# without specific prior written permission.
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

import os
import argparse
import subprocess
import sys
from subprocess import CalledProcessError

cwd = os.path.dirname(sys.argv[0])
cmd_list = ['lpbk1', 'read', 'write', 'trput', 'sw',
            'fvlbypass', 'fpgalpbk', 'mactest', 'fpgastats', 'fpgamac']
cmd_map = {'lpbk1': ['nlb0'],
           'read': ['nlb3', '--mode=read'],
           'write': ['nlb3', '--mode=write'],
           'trput': ['nlb3', '--mode=trput'],
           'sw': ['nlb7'],
           'fvlbypass': ['fvlbypass'],
           'fpgalpbk': ['fpgalpbk'],
           'mactest': ['mactest'],
           'fpgastats': ['fpgastats'],
           'fpgamac': ['fpgamac']}


def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('-t', '--target',
                        default='fpga',
                        choices=['fpga', 'ase'],
                        help='choose target. Default: fpga')
    parser.add_argument('-m', '--mode', choices=cmd_list,
                        help=('choose test mode. Combine this with'
                              '-h or --help to see detail help message'
                              'for each mode.'))

    args, leftover = parser.parse_known_args()

    if args.mode:
        cmdline = cmd_map[args.mode] + leftover
    else:
        parser.print_help()
        exit(1)

    cmdline[0] = os.path.join(cwd, cmdline[0])
    cmdline = cmdline + ['-t', args.target] + leftover

    try:
        subprocess.run(cmdline, check=True)
    except CalledProcessError as e:
        exit(e.returncode)


if __name__ == "__main__":
    main()
