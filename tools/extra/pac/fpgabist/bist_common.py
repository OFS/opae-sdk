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

import glob
import json
import os
import re
import subprocess
import sys

# TODO: Use AFU IDs vs. names of AFUs
BIST_MODES = ['bist_afu', 'dma_afu', 'nlb_mode_3']
REQ_CMDS = ['lspci', 'fpgainfo', 'fpgaconf', 'fpgadiag', 'fpga_dma_test',
            'fpga_dma_vc_test', 'bist_app']
BDF_PATTERN = r'{:02x}:{:02x}.{:x}'
VCP_ID = 0x0b30


def find_exec(cmd, paths):
    for p in paths:
        f = os.path.join(p, cmd)
        if os.path.isfile(f):
            return True
    return False


def check_required_cmds():
    path = os.environ['PATH'].split(os.pathsep)
    if not all([find_exec(cmd, path) for cmd in REQ_CMDS]):
        sys.exit("Failed to find required BIST commands\nTerminating BIST")


def get_afu_id(gbs_path="", bdf=None):
    if os.path.isfile(gbs_path):
        cmd = ["packager", "gbs-info", "--gbs={}".format(gbs_path)]
        output = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        json_data = json.loads(output.communicate()[0])
        accel_data = json_data["afu-image"]["accelerator-clusters"]
        uuid = accel_data[0]["accelerator-type-uuid"].encode("ascii")
        return uuid.lower().replace("-", "")
    elif bdf:
        pattern = BDF_PATTERN.format(bdf['bus'], bdf['device'],
                                     bdf['function'])
        fpgas = glob.glob('/sys/class/fpga/*')
        for fpga in fpgas:
            slink = os.path.basename(os.readlink(os.path.join(fpga, "device")))
            m = re.findall(pattern, slink)
            if m:
                id_path = os.path.join(fpga,
                                       'intel-fpga-port.{}'
                                       .format(fpgas.index(fpga)),
                                       'afu_id')
                with open(id_path) as f:
                    uuid = f.read().rstrip("\n")
                    return uuid.lower()
    return None


# Return a list of all available bus numbers
def get_all_fpga_bdfs(args):
    pattern = re.compile(r'\d+:(?P<bus>[a-fA-F0-9]{2}):'
                         r'(?P<device>[a-fA-F0-9]{2})\.'
                         r'(?P<function>[a-fA-F0-9])')
    bdf_list = []
    for fpga in glob.glob('/sys/class/fpga/*'):
        devpath = os.path.join(fpga, "device")
        symlink = os.path.basename(os.readlink(devpath))
        m = pattern.match(symlink)
        data = m.groupdict() if m else {}
        if data:
            with open(os.path.join(devpath, "device"), 'r') as fd:
                device_id = fd.read().strip()

            # Add device ID to the data dictionary
            data['device_id'] = device_id

            # Add this BDF/device to the list
            bdf_list.append(dict([(k, int(v, 16))
                                  for (k, v) in data.iteritems()]))

    return bdf_list


# Given a list of all available FPGAs, return a list filtered by
# the command line arguments.
def get_bdf_from_args(all_bdfs, args):
    bdf_list = []

    for tgt in all_bdfs:
        if (not args.bus or (args.bus == tgt['bus'])) and \
           (not args.device or (args.device == tgt['device'])) and \
           (not args.function or (args.function == tgt['function'])) and \
           (not args.device_id or (args.device_id == tgt['device_id'])):
            bdf_list.append(tgt)

    return bdf_list


def get_mode_from_path(gbs_path):
    if os.path.isfile(gbs_path):
        base = os.path.basename(gbs_path)
        return os.path.splitext(base)[0]
    return None


def load_gbs(gbs_file, bdf):
    print "Attempting Partial Reconfiguration:"
    cmd = ['fpgaconf', '-B', hex(bdf['bus']), '-D',
           hex(bdf['device']), '-F', hex(bdf['function']),
           gbs_file]
    try:
        subprocess.check_call(cmd)
    except subprocess.CalledProcessError as e:
        print "Failed to load gbs file: {}".format(gbs_file)
        print "Please try a different gbs"
        sys.exit(-1)


class BistMode(object):
    name = ""
    afu_id = ""
    executables = {}
    dir_path = ""
    dev_id = 0

    def run(self, gbs_path, bus_num):
        raise NotImplementedError


def global_arguments(parser):
    parser.add_argument('-i', '--device-id',
                        type=str,
                        help='Device Id for Intel FPGA')

    parser.add_argument('-B', '--bus', type=str,
                        help='Bus number for specific FPGA')

    parser.add_argument('-D', '--device', type=str,
                        help='Device number for specific FPGA')

    parser.add_argument('-F', '--function', type=str,
                        help='Function number for specific FPGA')

    parser.add_argument('gbs_paths', nargs='*', type=str,
                        help='Paths for the gbs files for BIST')


# Parse and then canonicalize the arguments
def parse_args(parser):
    args = parser.parse_args()

    # Bist defaulted to hex values for BDF. Interpret the strings and
    # replace them with integers.
    if args.device_id:
        args.device_id = int(args.device_id, 16)
    if args.bus:
        args.bus = int(args.bus, 16)
    if args.device:
        args.device = int(args.device, 16)
    if args.function:
        args.function = int(args.function, 16)

    return args
