#!/usr/bin/env python3
# Copyright(c) 2019, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and / or other materials provided with the distribution.
# * Neither the name of Intel Corporation nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

##########################
#
# Main entry to the tool
#
##########################
import logging
import argparse
import os
import sys
import json
from opae.admin.utils import database
from opae.admin.utils import common_util
from opae.admin.utils import verifier

METADATA_GUID = "XeonFPGA" + chr(0xB7) + "GBSv001"

# Metadata length field is a unsigned 32 bit int
SIZEOF_LEN_FIELD = 4

# Length of GUID string
GUID_LEN = len(METADATA_GUID)


LOGLEVELS = [logging.WARNING, logging.INFO, logging.DEBUG]

ADD_OPTIONS = database.ADD_OPTIONS

LOG = logging.getLogger()


def add_common_options(parser):
    if ADD_OPTIONS:
        parser.add_argument(
            "-a", "--all", help="Print all headers", action="store_true"
        )
        parser.add_argument(
            "-j",
            "--json",
            help="Print JSON if it exists in the file", action="store_true"
        )
        parser.add_argument(
            "-H",
            "--hashes",
            help="Print hash values from headers",
            action="store_true")
        parser.add_argument(
            "-k", "--keys", help='Print key XY values', action="store_true"
        )

    parser.add_argument(
        "-v",
        "--verbose",
        help="Increase verbosity.  Can be specified multiple times",
        action="count",
    )
    parser.add_argument(
        "file", help='List of files to print', nargs='+'
    )


def is_JSON(contents):
    LOG.debug(
        "GUID: {} vs. file {}".format(
            METADATA_GUID, "".join([chr(i) for i in contents.data[:GUID_LEN]])
        )
    )
    return METADATA_GUID == "".join([chr(i) for i in contents.data[:GUID_LEN]])


def skip_JSON(contents):
    leng = contents.get_dword(GUID_LEN)
    LOG.debug("length of json={}".format(leng))
    return leng + GUID_LEN + SIZEOF_LEN_FIELD


def main():
    parser = argparse.ArgumentParser(
        description="Print PAC Bitstream metadata"
    )

    add_common_options(parser)

    args = parser.parse_args()
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit()

    if not args.verbose:
        args.verbose = 0
    if args.verbose > 2:
        args.verbose = 2

    if not ADD_OPTIONS:
        args.all = True
        args.json = True
        args.hashes = True
        args.keys = True

    if not args.json and not args.hashes and not args.keys:
        args.all = True

    if args.all:
        args.json = True
        args.hashes = True
        args.keys = True

    LOG.setLevel(logging.NOTSET)
    log_fmt = ('[%(asctime)-15s] [%(levelname)-8s] '
               '%(message)s')
    log_hndlr = logging.StreamHandler(sys.stdout)
    log_hndlr.setFormatter(logging.Formatter(log_fmt))

    log_hndlr.setLevel(LOGLEVELS[args.verbose])

    LOG.addHandler(log_hndlr)

    for f in args.file:
        LOG.debug("Trying file {}".format(f))
        print("File {}:".format(f))

        if not os.path.exists(f):
            LOG.error("File {} does not exist".format(f))
            continue

        contents = common_util.BYTE_ARRAY("FILE", f)

        # Get JSON
        has_json = is_JSON(contents)
        LOG.debug("has_json = {}".format(has_json))
        payload_offset = 0 if not has_json else skip_JSON(contents)
        sig_offset = payload_offset

        con_type = contents.get_dword(payload_offset + 8)

        LOG.debug("payload_offset={}".format(payload_offset))
        json_string = common_util.BYTE_ARRAY()
        if has_json:
            json_string.append_data(
                contents.data[GUID_LEN + SIZEOF_LEN_FIELD: sig_offset]
            )

        LOG.debug("".join("{:02x} ".format(x) for x in json_string.data))
        LOG.debug(bytearray(json_string.data).decode(sys.getdefaultencoding()))

        j_data = None
        if has_json:
            a = bytearray(json_string.data).decode(sys.getdefaultencoding())
            j_data = json.loads(a)
            LOG.debug(json.dumps(j_data, sort_keys=True, indent=4))

        b0 = common_util.BYTE_ARRAY()
        b1 = common_util.BYTE_ARRAY()
        payload = common_util.BYTE_ARRAY()

        b0.append_data(contents.data[payload_offset: payload_offset + 128])
        b1.append_data(
            contents.data[payload_offset + 128: payload_offset + 1024])
        payload.append_data(contents.data[payload_offset + 1024:])

        # is_OK = b0.get_dword(0) == database.DESCRIPTOR_BLOCK_MAGIC_NUM

        # LOG.debug("b0 size={}, b1 size={}, payload size={}".format(
        #    b0.size(), b1.size(), payload.size()))

        # if not is_OK:
        #    LOG.error("File '{}' unrecognized".format(f))
        #    continue

        # Check for DC PR bitstream
        if _VERIFIER_BASE.is_Darby_PR(contents, sig_offset):
            block0 = verifier.Block_0_dc(b0.data, payload.data)
            block1 = verifier.Block_1_dc(b1.data, block0)
        else:
            block0 = verifier.Block_0(b0.data, payload.data)
            block1 = verifier.Block_1(b1.data, block0)

        if not block0.is_good and not block1.is_good:
            LOG.error("File '{}' unrecognized".format(f))
            continue

        args.main_command = block0.content_type
        args.cert_type = block0.cert_type

        verifier.print_bitstream(args, b0, b1, payload,
                                 bytearray(json_string.data))

    return


if __name__ == "__main__":
    main()
