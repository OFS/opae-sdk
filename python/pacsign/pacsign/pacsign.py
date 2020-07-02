# Copyright(c) 2019, Intel Corporation
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
import logging
import importlib
import argparse
import os
import sys

import pacsign.hsm_managers  # noqa
from pacsign.logger import log
from pacsign import reader
from pacsign import common_util


def get_manager_names(append_manager=True):
    managers = []
    suffix = '_manager' if append_manager else ''
    for p in pacsign.hsm_managers.__path__:
        for item in os.listdir(p):
            if not item.startswith('__'):
                try:
                    importlib.import_module('.{}'.format(item),
                                            'pacsign.hsm_managers')
                except ImportError:
                    pass
                else:
                    if item.endswith('_manager'):
                        managers.append(item)
                    else:
                        managers.append('{}{}'.format(item, suffix))
    return managers


def add_common_options(parser):
    parser.add_argument(
        "-t",
        "--cert_type",
        type=str.upper,
        help="Type of certificate to generate",
        required=True,
        choices=["UPDATE", "CANCEL", "RK_256", "RK_384"],
    )
    parser.add_argument(
        "-H",
        "--HSM_manager",
        help="Module name for key / signing manager",
        choices=get_manager_names(),
        required=True,
    )
    parser.add_argument(
        "-C",
        "--HSM_config",
        help="Config file name for key / signing manager (optional)",
    )
    parser.add_argument(
        "-r",
        "--root_key",
        help="Identifier for the root key. Provided as-is to the key manager",
    )
    parser.add_argument(
        "-k",
        "--code_signing_key",
        help="Identifier for the CSK. Provided as-is to the key manager",
    )
    parser.add_argument(
        "-d",
        "--csk_id",
        type=int,
        help="CSK number.  Only required for cancellation certificate",
    )
    parser.add_argument(
        "-R",
        "--root_bitstream",
        help="Root entry hash programming bitstream. "
        "Used to verify update & cancel bitstreams.",
    )
    parser.add_argument(
        "-i", "--input_file", help="File name for the image to be acted upon"
    )
    parser.add_argument(
        "-o", "--output_file", help="File name in which the result is to be stored"
    )
    parser.add_argument(
        "--print", help="Print human-readable header information", action="store_true"
    )
    parser.add_argument(
        "-y", "--yes", help='Answer all questions with "yes"', action="store_true"
    )
    parser.add_argument(
        "-v",
        "--verbose",
        help="Increase verbosity.  Can be specified multiple times",
        action="count",
    )


def answer_y_n(args, question):
    if args.yes:
        return True
    ans = False
    while True:
        overwrite = input("{}? Y = yes, N = no: ".format(question))
        if overwrite.lower() == "y":
            ans = True
            break
        if overwrite.lower() == "n":
            break
    return ans


LOGLEVELS = [logging.WARNING, logging.INFO, logging.DEBUG]


def main():
    parser = argparse.ArgumentParser(prog='pacsign',
                                     description="Sign PAC bitstreams")
    subparsers = parser.add_subparsers(
        title="Commands",
        description="Image types",
        help="Allowable image types",
        dest="main_command",
    )
    parser_sr = subparsers.add_parser(
        "SR", aliases=["FIM", "BBS"], help="Static FPGA image"
    )
    parser_bmc = subparsers.add_parser("BMC", aliases=["BMC_FW"], help="BMC image")
    parser_pr = subparsers.add_parser(
        "PR", aliases=["AFU", "GBS"], help="Reconfigurable FPGA image"
    )
    parser_print = subparsers.add_parser("print", help="Print header information")

    add_common_options(parser_sr)
    add_common_options(parser_bmc)
    add_common_options(parser_pr)
    parser_print.add_argument(
        "files", nargs="+", help="List of files whose contents" " are to be printed"
    )
    parser_print.add_argument(
        "-v",
        "--verbose",
        help="Increase verbosity.  Can be specified multiple times",
        action="count",
    )

    args = parser.parse_args()
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit()

    if not args.verbose:
        args.verbose = 0
    if args.verbose > 2:
        args.verbose = 2

    log.handlers[0].setLevel(LOGLEVELS[args.verbose])

    if args.main_command == "print":
        return reader.print_file_contents(args.files)

    manager = args.HSM_manager.rstrip('_manager')
    # Load HSM handler
    try:
        hsm_manager = importlib.import_module('.{}'.format(manager),
                                              'pacsign.hsm_managers')
    except ImportError as err:
        common_util.assert_in_error(
            False, "Error '{}' importing module {}".format(err, args.HSM_manager)
        )
    try:
        importlib.invalidate_caches()
    except AttributeError:
        pass
    try:
        _method = getattr(hsm_manager, "HSM_MANAGER")
    except AttributeError:
        common_util.assert_in_error(
            False, "Invalid key manager module %s" % args.HSM_manager
        )

    if args.cert_type == "RK_384":
        common_util.assert_in_error(False, "384-bit keys not supported")

    # Validate arguments
    if args.cert_type == "UPDATE":
        common_util.assert_in_error(
            args.input_file is not None and args.output_file is not None,
            "Update requires both an input and output file",
        )
        if os.path.isfile(args.output_file):
            common_util.assert_in_error(
                answer_y_n(
                    args, "Output file {} exists. Overwrite".format(args.output_file)
                ),
                "Aborting.",
            )
        if args.root_key is None:
            common_util.assert_in_error(
                answer_y_n(args, "No root key specified.  Generate unsigned bitstream"),
                "Aborting.",
            )
        if args.code_signing_key is None:
            common_util.assert_in_error(
                answer_y_n(args, "No CSK specified.  Generate unsigned bitstream"),
                "Aborting.",
            )
        if args.root_bitstream is None:
            common_util.assert_in_error(
                answer_y_n(
                    args,
                    "No root entry hash bitstream specified."
                    "  Verification will not be done.  Continue",
                ),
                "Aborting.",
            )
        else:
            common_util.assert_in_error(
                os.path.isfile(args.root_bitstream),
                "File doesn't exist '{}'".format(args.root_bitstream),
            )
        if args.csk_id is not None:
            common_util.assert_in_error(
                False, "CSK ID cannot be specified for update types"
            )

        maker = reader.UPDATE_reader(args, hsm_manager, args.HSM_config)
    elif args.cert_type == "CANCEL":
        common_util.assert_in_error(
            args.root_key is not None, "Cancellation type requires a root key"
        )
        if args.code_signing_key is not None:
            common_util.assert_in_error(
                answer_y_n(args, "CSK specified but not required.  Continue"),
                "Aborting.",
            )
        common_util.assert_in_error(
            args.csk_id is not None, "CSK ID must be specified for cancellation types"
        )
        common_util.assert_in_error(
            args.input_file is None, "Cancellation not allowed with input file."
        )
        common_util.assert_in_error(
            args.output_file is not None, "No output file specified"
        )
        if args.root_bitstream is None:
            common_util.assert_in_error(
                answer_y_n(
                    args,
                    "No root entry hash bitstream specified."
                    "  Verification will not be done.  Continue",
                ),
                "Aborting.",
            )
        else:
            common_util.assert_in_error(
                os.path.isfile(args.root_bitstream),
                "File doesn't exist '{}'".format(args.root_bitstream),
            )
        if os.path.isfile(args.output_file):
            common_util.assert_in_error(
                answer_y_n(
                    args, "Output file {} exists. Overwrite".format(args.output_file)
                ),
                "Aborting.",
            )

        maker = reader.CANCEL_reader(args, hsm_manager, args.HSM_config)
    elif args.cert_type in ["RK_256", "RK_384"]:
        common_util.assert_in_error(
            args.root_key is not None, "Root hash programming requires a root key"
        )
        if args.code_signing_key is not None:
            common_util.assert_in_error(
                answer_y_n(args, "CSK specified and will be ignored.  Continue"),
                "Aborting.",
            )
        if args.csk_id is not None:
            common_util.assert_in_error(
                answer_y_n(args, "CSK ID specified and will be ignored.  Continue"),
                "Aborting.",
            )
        common_util.assert_in_error(
            args.input_file is None,
            "Root hash programming not allowed with input file.",
        )
        common_util.assert_in_error(
            args.output_file is not None, "No output file specified"
        )
        if args.root_bitstream is not None:
            common_util.assert_in_error(
                False,
                "--root_bitstream option not allowed for hash programming generation.",
            )
        if os.path.isfile(args.output_file):
            common_util.assert_in_error(
                answer_y_n(
                    args, "Output file {} exists. Overwrite".format(args.output_file)
                ),
                "Aborting.",
            )

        maker = reader.RHP_reader(args, hsm_manager, args.HSM_config)

    logging.shutdown()
    return maker.run()


if __name__ == "__main__":
    main()
