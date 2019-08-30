#! /usr/bin/env python
# Copyright(c) 2019, Intel Corporation
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

import argparse
import sys
import glob
import os
import fcntl
import re
import time
import subprocess
from common import exception_quit, FpgaFinder, COMMON
from common import convert_argument_str2hex


VCP_ID = 0x0b30

CONF_FILE = '/etc/modprobe.d/intel-fpga-fme.conf'
OPTION_LINE = 'options intel-fpga-fme fec_mode='
DRV_MODE = '/sys/module/intel_fpga_fme/parameters/fec_mode'
REMOVE_MOD = 'rmmod intel_fpga_fme'
PROBE_MOD = 'modprobe intel-fpga-fme'


def get_fpga_sysfs_path(sbdf):
    for fpga in glob.glob('/sys/class/fpga/*'):
        fpga_bdf = os.path.basename(os.readlink(os.path.join(fpga, "device")))
        if not fpga_bdf:
            continue
        if sbdf == fpga_bdf:
            return fpga
    print("WARNING: fpga not found @ {}".format(sbdf))
    return None


def get_upstream_switch_port(sbdf):
    p = re.compile(r'(?P<s>\d{4}):(?P<b>\w{2}):(?P<d>\d{2})\.(?P<f>\d)')
    try:
        symlink = os.readlink(os.path.join('/sys/bus/pci/devices', sbdf))
    except Exception as ex:
        return None
    m = p.findall(symlink)
    ports = []
    for n in m:
        ports.append('{}:{}:{}.{}'.format(*n))

    # ports pattern:
    # [..., slot_port, upstream_switch_port, downstream_switch_port, fpga]
    if len(ports) < 4:
        print("pci path to fpga is short than expected")
        return None

    return ports[-3]    # sbdf of upstream switch port


def do_rsu(sbdf, debug):
    print("performing remote system update")
    usp = get_upstream_switch_port(sbdf)
    if usp is None:
        print("{} is not valid".format(sbdf))
        return None

    try:
        cmd = "rsu bmcimg {}".format(sbdf)
        if debug:
            print(cmd)
            cmd += ' -d'
        rc = subprocess.call(cmd, shell=True)
        if rc != 0:
            print("failed to '{}'".format(cmd))
            return None
    except subprocess.CalledProcessError as e:
        print('failed call')
        return None

    # sbdf may changed after rsu, find the new one
    paths = glob.glob(os.path.join('/sys/bus/pci/devices', usp,
                                   '*:*:*.*', '*:*:*.?'))
    for devpath in paths:
        devid_path = os.path.join(devpath, 'device')
        if os.path.exists(devid_path):
            with open(devid_path, 'r') as fd:
                devid = fd.read().strip()
                if int(devid, 16) == VCP_ID:
                    sbdf = os.path.basename(devpath)

    return sbdf


def get_driver_mode(path):
    try:
        mode = None
        with open(path, 'r') as fd:
            mode = fd.read().strip()
        return mode
    except Exception as e:
        return None


def get_fec_mode(sbdf, debug):
    fpga_path = get_fpga_sysfs_path(sbdf)
    if not fpga_path:
        return None
    else:
        paths = glob.glob(os.path.join(fpga_path, 'intel-fpga-fme.*',
                                       'nios_spi', 'fec_mode',))
        if not paths:
            return None

        mode = None
        if len(paths) >= 1:
            with open(paths[0], 'r') as fd:
                mode = fd.read().strip()
            if debug:
                print("{} -> {}".format(paths[0], mode))
        return mode


def reload_driver(fec_mode, debug):
    print("reloading driver with new parameter '{}'".format(fec_mode))
    try:
        if debug:
            print(REMOVE_MOD)
        rc = subprocess.call(REMOVE_MOD, shell=True)
        if rc != 0:
            print("failed to '{}'".format(REMOVE_MOD))
            return rc
    except subprocess.CalledProcessError as e:
        print('failed call')
        return 2

    time.sleep(1)

    try:
        if debug:
            print(PROBE_MOD)
        rc = subprocess.call(PROBE_MOD, shell=True)
        if rc != 0:
            print("failed to '{}'".format(PROBE_MOD))
            return rc
    except subprocess.CalledProcessError as e:
        print(e)
        return 2

    time.sleep(1)

    mode = get_driver_mode(DRV_MODE)
    if debug:
        print("{} -> {}".format(DRV_MODE, mode))
    if mode != fec_mode:
        return 1

    return 0


def check_fec_mode(sbdf, fec_mode, debug):
    mode = get_fec_mode(sbdf, debug)
    if mode != fec_mode:
        return 1
    return 0


def show_fec_mode(sbdf):
    try:
        with open(CONF_FILE, 'r') as f:
            mode = f.read().strip()
    except Exception as e:
        mode = 'rs'
    p = r'"(\w+)"'
    m = re.findall(p, mode)
    if len(m) > 0:
        print("FEC mode in configuration: {}".format(m[0]))
    mode = get_driver_mode(DRV_MODE)
    if mode is None:
        print("FEC mode is not configurable by current driver")
    else:
        print("FEC mode in current driver: {}".format(mode))
    mode = get_fec_mode(sbdf, False)
    if mode is None:
        print("FEC mode is not configurable by current hardware")
    else:
        print("FEC mode in current hardware: {}".format(mode))
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--segment', '-S',
                        help='Segment number of PCIe device')
    parser.add_argument('--bus', '-B',
                        help='Bus number of PCIe device')
    parser.add_argument('--device', '-D',
                        help='Device number of PCIe device')
    parser.add_argument('--function', '-F',
                        help='Function number of PCIe device')
    parser.add_argument('mode', choices=['no', 'kr', 'rs'], nargs='?',
                        default=None, help='Choose fec mode to configure')
    parser.add_argument('--rsu', '-r', action='store_true',
                        help='Reboot card')
    parser.add_argument('--debug', '-d', action='store_true',
                        help='Output debug information')
    args, left = parser.parse_known_args()

    args = convert_argument_str2hex(
        args, ['segment', 'bus', 'device', 'function'])

    f = FpgaFinder(args.segment, args.bus, args.device, args.function)
    devs = f.find()
    for d in devs:
        sbdf = '{segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**d)
    if len(devs) > 1:
        exception_quit('{} FPGAs are found\n'
                       'please choose one FPGA'.format(len(devs)), 1)
    if not devs:
        exception_quit('no FPGA found', 2)

    if args.mode is None:
        if args.rsu:
            bdf = do_rsu(sbdf, args.debug)
            if bdf is None:
                ret = 1
            else:
                ret = 0
        else:
            ret = show_fec_mode(sbdf)
        sys.exit(ret)

    with open(CONF_FILE, 'w') as f:
        option_line = OPTION_LINE + '"{}"'.format(args.mode)
        if args.debug:
            print("write '{}' to file {}".format(option_line, CONF_FILE))
        f.write(option_line+'\n')

    ret = reload_driver(args.mode, args.debug)
    if ret == 0:
        bdf = do_rsu(sbdf, args.debug)
        if bdf is not None:
            if args.debug and bdf != sbdf:
                print("BDF of FPGA changed to {}".format(bdf))
            ret = check_fec_mode(bdf, args.mode, args.debug)
        else:
            ret = 1

    if ret == 0:
        print("done")
    else:
        print("please power cycle system or reboot card to make the fec mode "
              "effective")
    sys.exit(ret)


if __name__ == "__main__":
    main()
