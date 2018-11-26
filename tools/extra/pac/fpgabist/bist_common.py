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
import os
import re
import subprocess
import sys

# TODO: Use AFU IDs vs. names of AFUs
BIST_MODES = ['bist_afu', 'dma_afu', 'nlb_mode_3']
REQ_CMDS = ['lspci', 'fpgainfo', 'fpgaconf', 'fpgadiag', 'fpga_dma_test',
            'bist_app']
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


# Return a list of all available bus numbers
def get_all_fpga_bdfs(args):
    pattern = (r'\d+:(?P<bus>[a-fA-F0-9]{2}):'
               r'(?P<device>[a-fA-F0-9]{2})\.(?P<function>[a-fA-F0-9])')
    bdf_pattern = re.compile(pattern)
    bdf_list = []
    for fpga in glob.glob('/sys/class/fpga/*'):
        devpath = os.path.join(fpga, "device")
        symlink = os.path.basename(os.readlink(devpath))
        m = bdf_pattern.match(symlink)
        data = m.groupdict() if m else {}
        if data:
            with open(os.path.join(devpath, "device"), 'r') as fd:
                device_id = fd.read().strip()
            if int(device_id, 16) == int(vars(args)['device_id'], 16):
                bdf_list.append(dict([(k, hex(int(v, 16)).lstrip("0x"))
                                for (k, v) in data.iteritems()]))
    return bdf_list


def get_bdf_from_args(args):
    pattern = (r'(?P<bus>[a-fA-F0-9]{2}):'
               r'(?P<device>[a-fA-F0-9]{2})\.(?P<function>[a-fA-F0-9]).*?.')
    dev_id = r'{:04x}'.format(int(vars(args)['device_id'], 16))
    bdf_pattern = re.compile(pattern+dev_id)
    bdf_list = []
    param = ':{}:{}.{}'.format(
            hex(int(vars(args)['bus'], 16))
            if vars(args)['bus'] else '',
            hex(int(vars(args)['device'], 16))
            if vars(args)['device'] else '',
            hex(int(vars(args)['function'], 16))
            if vars(args)['function'] else '')
    host = subprocess.check_output(['lspci', '-s', param])
    matches = re.findall(bdf_pattern, host)
    for bus, device, function in matches:
        bdf_list.append({'bus': bus, 'device': device, 'function': function})
    return bdf_list


def get_mode_from_path(gbs_path):
    if os.path.isfile(gbs_path):
        base = os.path.basename(gbs_path)
        return os.path.splitext(base)[0]
    return None


def load_gbs(gbs_file, bus_num):
    print "Attempting Partial Reconfiguration:"
    cmd = "{} -B 0x{} -v {}".format('fpgaconf', bus_num, gbs_file)
    try:
        subprocess.check_call(cmd, shell=True)
    except subprocess.CalledProcessError as e:
        print "Failed to load gbs file: {}".format(gbs_file)
        print "Please try a different gbs"
        sys.exit(-1)


class BistMode(object):
    name = ""
    executables = {}
    dir_path = ""

    def run(self, gbs_path, bus_num):
        raise NotImplementedError


def global_arguments(parser):
    parser.add_argument('-i', '--device-id', default='09c4',
                        type=str,
                        help='Device Id for Intel FPGA default: 09c4')

    parser.add_argument('-B', '--bus', type=str,
                        help='Bus number for specific FPGA')

    parser.add_argument('-D', '--device', type=str,
                        help='Device number for specific FPGA')

    parser.add_argument('-F', '--function', type=str,
                        help='Function number for specific FPGA')

    parser.add_argument('gbs_paths', nargs='*', type=str,
                        help='Paths for the gbs files for BIST')
