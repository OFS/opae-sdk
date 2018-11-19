#!/usr/bin/env python
# Copyright(c) 2017, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#  this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#  this list of conditions and the following disclaimer in the documentation
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

import os
import sys
import subprocess
import shutil
import argparse
import json
import utils
from afu import AFU
from gbs import GBS
from metadata import metadata


PACKAGER_EXEC = "packager"
DESCRIPTION = 'Intel OPAE FPGA Packager'
VERSION = "@INTEL_FPGA_API_VERSION@"

try:
    assert sys.version_info >= (2, 7) and sys.version_info < (3, 0, 0)
except AssertionError:
    print(DESCRIPTION + " requires Python 2 version 2.7+")
    sys.exit(1)

USAGE = """
{0}

{1} <cmd> [options]

The following values for <cmd> are currently supported:
\t help - displays this message
\t create-gbs - creates GBS file from RBF and Accelerator Description File
\t modify-gbs - modify metadata of existing GBS file using --set-value
\t gbs-info - prints information about GBS file
\t get-rbf - creates RBF file by extracting RBF from GBS file

{1} <cmd> --h will give command specific help
""".format(DESCRIPTION, PACKAGER_EXEC)


def run_packager():
    parser = argparse.ArgumentParser(usage=USAGE, add_help=False)
    parser.add_argument("cmd", nargs="?")
    parser.add_argument("remain_args", nargs=argparse.REMAINDER)
    args = parser.parse_args(sys.argv[1:])
    cmd_description = "{0} {1}".format(PACKAGER_EXEC, args.cmd)
    subparser = argparse.ArgumentParser(description=cmd_description)
    subparser._optionals.title = 'Options'

    if args.cmd == "help" or not args.cmd:
        print(USAGE)

    elif args.cmd == "version":
        if VERSION.startswith("@"):
            try:
                devnull = open(os.devnull, 'w')
                repo = subprocess.check_output('git remote -v',
                                               shell=True,
                                               stderr=devnull)
                version = (subprocess.check_output('git describe --tags',
                                                   shell=True,
                                                   stderr=devnull).split()[0]
                           if "opae-sdk" in repo else "UNKNOWN REPO")
            except subprocess.CalledProcessError:
                version = "UNKNOWN"
        else:
            version = VERSION
        print("{0}: version {1}".format(DESCRIPTION, version))
    elif args.cmd == "create-gbs":
        subparser.usage = "\n" + cmd_description + \
            " --rbf=<RBF_PATH> --afu-json=<AFU_JSON_PATH>"\
            " --gbs=<NAME_FOR_GBS> --set-value <key>:<value>\n"
        subparser.add_argument('--rbf', required=True,
                               help='RBF file (REQUIRED)')
        subparser.add_argument('--afu-json', required=False,
                               help='AFU JSON file that contains metadata')
        subparser.add_argument('--no-metadata', default=False,
                               action='store_true',
                               help='Empty metadata section will be appended')
        subparser.add_argument('--gbs', required=False,
                               help='Output location for gbs file. '
                               'Default is <rbf_name>.gbs')
        subparser.add_argument('--set-value', required=False, nargs='*',
                               help='set values for keys in JSON metadata as '
                               '<key>:<value>. Can be followed by more than '
                               'one <key>:<value> pairs.')
        subargs = subparser.parse_args(args.remain_args)
        afu = AFU(subargs.afu_json)
        gbs_file = afu.create_gbs(subargs.rbf, subargs.gbs, subargs.set_value)
        print("Wrote {0}".format(gbs_file))

    elif args.cmd == "modify-gbs":
        subparser.usage = "\n" + cmd_description + \
            " --input-gbs=<PATH_TO_GBS_TO_BE_MODIFIED>"\
            " --output-gbs=<NAME_FOR_NEW_GBS> --set-value <key>:<value>\n"
        subparser.add_argument('--input-gbs', required=True,
                               help='Path to input gbs file')
        subparser.add_argument('--output-gbs', required=False,
                               help='Path to output gbs file. Will replace '
                               'original file if not provided')
        subparser.add_argument('--set-value', required=True, nargs='*',
                               help='set values for keys in JSON metadata as '
                               '<key>:<value>. Can be followed by more than '
                               'one <key>:<value> pairs.')
        subargs = subparser.parse_args(args.remain_args)
        gbs = GBS(subargs.input_gbs)
        afu = AFU.create_afu_from_gbs(gbs)
        afu.update_afu_json(subargs.set_value)
        afu.validate(packaging=True)
        gbs.update_gbs_info(afu.afu_json)
        gbs_file = gbs.write_gbs(subargs.output_gbs)
        print("Wrote {0}".format(gbs_file))

    elif args.cmd == "package":
        subparser.usage = "\n" + cmd_description + \
            " --afu-json=<AFU_JSON_PATH> --rbf=<RBF_PATH>"\
            " --out=<NAME_OF_PACKAGE>\n"
        subparser.usage += cmd_description + \
            " --afu-json=<AFU_JSON_PATH> --rbf=<RBF_PATH> --sw-dir=<SW_DIR>"\
            " --doc-dir=<DOC_DIR>"
        subparser.add_argument('--afu-json', required=True,
                               help='AFU JSON file that contains metadata '
                               '(REQUIRED)')
        subparser.add_argument('--rbf', required=True,
                               help='RBF file (REQUIRED)')
        subparser.add_argument('--sw-dir', required=False,
                               help='Location of software files to include')
        subparser.add_argument('--doc-dir', required=False,
                               help='Location of documentation files to '
                               'include')
        subparser.add_argument('--out', required=False, default="afu",
                               help='Used to specify name of package')
        subargs = subparser.parse_args(args.remain_args)
        afu = AFU(subargs.afu_json)
        afu.package(subargs.rbf, subargs.sw_dir, subargs.doc_dir, subargs.out)
        print("Wrote {0}.zip".format(subargs.out))

    elif args.cmd == "gbs-info":
        subparser.usage = "\n" + cmd_description + " --gbs=<GBS_PATH>"
        subparser.add_argument('--gbs', required=True,
                               help='Path to GBS file')
        subargs = subparser.parse_args(args.remain_args)
        gbs = GBS(subargs.gbs)
        gbs.print_gbs_info()

    elif args.cmd == "get-rbf":
        subparser.usage = "\n" + cmd_description + \
            "--gbs=<GBS_PATH> --rbf=<NAME_FOR_RBF>"
        subparser.add_argument('--gbs', required=True,
                               help='Path to GBS file from which rbf is to be '
                               'extracted')
        subparser.add_argument('--rbf', required=False,
                               help='Output location for rbf file. Default is '
                               '<gbs_name>.rbf')
        subargs = subparser.parse_args(args.remain_args)
        gbs = GBS(subargs.gbs)
        rbf_file = gbs.write_rbf(subargs.rbf)
        print("Wrote {0}".format(rbf_file))

    else:
        raise Exception("{0} is not a command for {1}!".format(
            args.cmd, DESCRIPTION))


def main():
    try:
        sys.exit(run_packager())
    except Exception as e:
        print("ERROR: {0}".format(e.__str__()))
        sys.exit(1)


if __name__ == '__main__':
    main()
