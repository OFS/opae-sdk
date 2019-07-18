#!/usr/bin/env python
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

import argparse
import sys
import glob
import os
import re
import time
import fcntl
import subprocess


VCP_ID = 0x0b30
VCP_PF1_ID = (0x0b32, 0x5052)

RSU_LOCK_FILE = "/tmp/rsu_lock"
f_lock = None

FPGA_RSU = 0
BMC_RSU = 1
USER_BOOT_PAGE = 0
FACTORY_BOOT_PAGE = 1

RSU_SP_ID = 0        # slot port index in rsu ports list
RSU_USP_ID = 1       # upstream switch port index in rsu ports list
RSU_DSP_PF0_ID = 2   # downstream switch port (to pf0) index in rsu ports list
RSU_FPGA_PF0_ID = 3  # fpga pf0 port index in rsu ports list
RSU_DSP_PF1_ID = 4   # downstream switch port (to pf1) index in rsu ports list
RSU_FPGA_PF1_ID = 5  # fpga pf1 port index in rsu ports list
RSU_PORTS_LIST_LEN = 6

debug_on = False


def parse_args():
    descr = 'A tool to test RSU.'

    epi = 'example usage:\n\n'
    epi += '    vcrsu.py a10 25:00.0\n\n'

    fc_ = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(description=descr, epilog=epi,
                                     formatter_class=fc_)
    parser.add_argument('type', help='type of operation',
                        choices=['a10', 'max10',
                                 'disable-card', 'enable-card'])
    bdf_help = "bdf of device to do rsu (e.g. 04:00.0 or 0000:04:00.0)"
    parser.add_argument('bdf', nargs='?', help=bdf_help)
    opt_help = "reload from factory image"
    parser.add_argument('-f', '--factory', action='store_true', help=opt_help)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='debug mode')
    return parser.parse_args()


def normalize_bdf(bdf):
    pat = r'[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$'
    if re.match(pat, bdf):
        return bdf

    if re.match(r'[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$', bdf):
        return "0000:{}".format(bdf)


def get_devid(bdf):
    filepath = os.path.join('/sys/bus/pci/devices', bdf, 'device')
    if not os.path.exists(filepath):
        print("WARNING: {} not found".format(filepath))
        return None
    with open(filepath, 'r') as fd:
        return fd.read().strip()


def get_fpga_sysfs_path(bdf):
    for fpga in glob.glob('/sys/class/fpga/*'):
        fpga_bdf = os.path.basename(os.readlink(os.path.join(fpga, "device")))
        if not fpga_bdf:
            continue
        if bdf == fpga_bdf:
            return fpga
    print("WARNING: fpga not found @ {}".format(bdf))
    return None


def get_spi_sysfs_path(bdf):
    fpga_path = get_fpga_sysfs_path(bdf)
    if not fpga_path:
        return None
    else:
        paths = glob.glob(os.path.join(fpga_path, 'intel-fpga-fme.*',
                                       'spi-altera.*', 'spi_master',
                                       'spi*', 'spi*.*'))
        if not paths:
            return None
        if len(paths) > 1:
            print("multiple spi found @ {}".format(bdf))
        return paths[0]


def get_rsu_ports(path, rsu_type):
    p = re.compile(r'(?P<s>\d{4}):(?P<b>\w{2}):(?P<d>\d{2})\.(?P<f>\d)')
    try:
        symlink = os.readlink(path)
    except Exception as ex:
        return []
    m = p.findall(symlink)
    ports = []
    for n in m:
        ports.append('{}:{}:{}.{}'.format(*n))

    # ports pattern:
    # [..., slot_port, upstream_switch_port, downstream_switch_port, fpga_pf]
    if len(ports) < 4:
        print("pci path to fpga is short than expected")
        return []
    sp = ports[-4]        # bdf of slot port
    usp = ports[-3]       # bdf of upstream switch port
    dsp_pf0 = ports[-2]   # bdf of downstream switch port connected to fpga pf0
    fpga_pf0 = ports[-1]  # bdf of fpga pf0

    # to be found in the following search
    dsp_pf1 = None      # bdf of downstream switch port connected to fpga pf1
    fpga_pf1 = None     # bdf of fpga pf1

    # search other devices' root path under the same switch
    paths = glob.glob(os.path.join('/sys/bus/pci/devices', usp,
                                   '*:*:*.*', '*:*:*.*', 'device'))
    for devpath in paths:
        with open(devpath, 'r') as fd:
            devid = fd.read().strip()
            is_fpga_pf1 = (int(devid, 16) in VCP_PF1_ID)
            if rsu_type == BMC_RSU or is_fpga_pf1:
                m = p.findall(devpath)
                for n in m:
                    port = '{}:{}:{}.{}'.format(*n)
                    if port not in ports:
                        ports.append(port)
                if is_fpga_pf1:
                    dsp_pf1 = ports[-2]
                    fpga_pf1 = ports[-1]

    if dsp_pf1 is None or fpga_pf1 is None:
        print("second physical function of fpga is not found")

    # each port index in below list is fixed
    return [sp, usp, dsp_pf0, fpga_pf0, dsp_pf1, fpga_pf1]


def set_aer(port, mask):
    global debug_on
    cmd = "setpci -s {} ECAP_AER+0x08.L={:#x}".format(port, mask)
    if debug_on:
        print(cmd)
    try:
        output = subprocess.check_output(cmd, shell=True)
    except Exception as e:
        if e.returncode != 256:
            return 1

    cmd = "setpci -s {} ECAP_AER+0x14.L={:#x}".format(port, mask)
    if debug_on:
        print(cmd)
    try:
        output = subprocess.check_output(cmd, shell=True)
    except Exception as e:
        if e.returncode != 256:
            return 1
    return 0


def disable_aer(port):
    return set_aer(port, 0xFFFFFFFF)


def enable_aer(port):
    return set_aer(port, 0x00000000)


def write_sysfs_node(path, value):
    global debug_on
    if not os.path.exists(path):
        print("WARNING: {} not found".format(path))
        return 1
    try:
        with open(path, 'w') as fd:
            if debug_on:
                print("echo {} > {}".format(value, path))
            fd.write(str(value))
            return 0
    except Exception as e:
        print(e)
        return 1


def trigger_rsu(bdf, rsu_type, boot_page):
    spi_path = get_spi_sysfs_path(bdf)
    if not spi_path:
        return 1
    else:
        if rsu_type == FPGA_RSU:
            path = os.path.join(spi_path, 'fpga_flash_ctrl', 'fpga_image_load')
            value = 1 if boot_page == USER_BOOT_PAGE else 0
        elif rsu_type == BMC_RSU:
            path = os.path.join(spi_path, 'bmcimg_flash_ctrl',
                                'bmcimg_image_load')
            value = 0 if boot_page == USER_BOOT_PAGE else 1
        else:
            print("WARNING: invalid type {} for rsu".format(rsu_type))
            return 1
        return write_sysfs_node(path, value)


def remove_device(bdf):
    path = os.path.join('/sys/bus/pci/devices', bdf, 'remove')
    return write_sysfs_node(path, 1)


def rescan_device(bdf, bus=None):
    if bus is None:
        rescan_path = '/sys/bus/pci/devices/{}/rescan'.format(bdf)
        ret = write_sysfs_node(rescan_path, 1)
    else:
        power_path = '/sys/bus/pci/devices/{}/power/control'.format(bdf)
        if not os.path.exists(power_path):
            print("WARNING: {} not found".format(power_path))
            return 1
        with open(power_path, 'r') as fd:
            ctrl = fd.read().strip()
        ret = 0
        if ctrl != 'on':
            ret = write_sysfs_node(power_path, 'on')
        if ret == 0:
            rescan_path = '/sys/bus/pci/devices/{}/pci_bus/{}' \
                          '/rescan'.format(bdf, bus)
            ret = write_sysfs_node(rescan_path, 1)
            if ctrl != 'on':
                write_sysfs_node(power_path, ctrl)
    return ret


def acquire_rsu_lock():
    global f_lock
    if not os.path.exists(RSU_LOCK_FILE):
        with open(RSU_LOCK_FILE, "w") as f:
            f.write("0")
    try:
        f_lock = open(RSU_LOCK_FILE, "r+")
    except Exception as ex:
        print(ex)
        return 1
    fcntl.flock(f_lock.fileno(), fcntl.LOCK_EX)
    return 0


def release_rsu_lock():
    global f_lock
    if f_lock:
        f_lock.close()


def do_rsu(rsu_type, bdf, boot_page):
    ret = acquire_rsu_lock()
    if ret == 1:
        print("failed to get rsu lock, please reboot board to complete rsu")
        return ret
    devid = get_devid(bdf)
    if devid is None:
        return 1
    if int(devid, 16) == VCP_ID:
        print("performing remote system update")
        ports = get_rsu_ports(os.path.join('/sys/bus/pci/drivers',
                                           'intel-fpga-pci', bdf),
                              rsu_type)
        if len(ports) == RSU_PORTS_LIST_LEN:
            if rsu_type == FPGA_RSU:
                pf1_bdf = ports[RSU_FPGA_PF1_ID]
                ret = disable_aer(ports[RSU_DSP_PF0_ID])
                if pf1_bdf is not None:
                    ret += disable_aer(ports[RSU_DSP_PF1_ID])
                if ret != 0:
                    sys.exit(ret)
                ret = trigger_rsu(bdf, rsu_type, boot_page)
                if ret == 0:
                    ret = remove_device(bdf)
                    if pf1_bdf is not None:
                        if ret == 0 and pf1_bdf != bdf:
                            ret = remove_device(pf1_bdf)
                time.sleep(10)
                ret += rescan_device(ports[RSU_DSP_PF0_ID],
                                     ports[RSU_FPGA_PF0_ID][:7])
                if pf1_bdf is not None:
                    ret += rescan_device(ports[RSU_DSP_PF1_ID], pf1_bdf[:7])
                ret += enable_aer(ports[RSU_DSP_PF0_ID])
                if pf1_bdf is not None:
                    ret += enable_aer(ports[RSU_DSP_PF1_ID])
            elif rsu_type == BMC_RSU:
                ret = disable_aer(ports[RSU_SP_ID])
                if ret != 0:
                    sys.exit(ret)
                ret = trigger_rsu(bdf, rsu_type, boot_page)
                if ret == 0:
                    ret = remove_device(ports[RSU_USP_ID])
                time.sleep(10)
                ret += rescan_device(ports[RSU_SP_ID],
                                     ports[RSU_USP_ID][:7])
                ret += enable_aer(ports[RSU_SP_ID])
        else:
            print("FPGA is not found @ {}".format(bdf))
            ret = 1
    else:
        print('FPGA with id {:04x} is not found @ {}'.format(VCP_ID, bdf))
        ret = 1
    release_rsu_lock()
    return ret


def main():
    global debug_on
    args = parse_args()
    if args.bdf is None:
        if args.type == 'enable-card':
            paths = glob.glob(os.path.join('/tmp/*:*:*.*'))
            if len(paths) == 1:
                args.bdf = os.path.basename(paths[0])
        if args.bdf is None:
            print("Please specify bdf as [bus]:[device].[function]")
            sys.exit(1)
    bdf = normalize_bdf(args.bdf)
    debug_on = args.debug
    boot_page = FACTORY_BOOT_PAGE if args.factory else USER_BOOT_PAGE
    rsu_type = FPGA_RSU if args.type == 'a10' else BMC_RSU

    if args.type in ['a10', 'max10']:
        if bdf:
            ret = do_rsu(rsu_type, bdf, boot_page)
        else:
            print("Must specify bdf of FPGA in the board")
            ret = 1
    elif args.type == 'disable-card':
        ports = get_rsu_ports(os.path.join('/sys/bus/pci/drivers',
                                           'intel-fpga-pci', bdf),
                              args.type)
        if len(ports) == 0:
            print('{} not found'.format(args.bdf))
            sys.exit(0)

        p = ports[RSU_SP_ID]
        print(p)
        with open("/tmp/"+bdf, 'w') as f:
            f.write(p)
        ret = disable_aer(p)
        if ret == 0:
            ret = remove_device(ports[RSU_USP_ID])
    elif args.type == 'enable-card':
        with open("/tmp/"+bdf, 'r') as f:
            bdf = f.read(16)
        bp = re.compile(r'(?P<s>\d{4}):(?P<b>\w{2})')
        paths = glob.glob(os.path.join('/sys/bus/pci/devices', bdf,
                                       'pci_bus', '*:*', 'rescan'))
        ret = 0
        for path in paths:
            m = bp.findall(path)
            for n in m:
                bus = '{}:{}'.format(*n)
                if bus != bdf[:7]:
                    ret += rescan_device(bdf, bus)
        if ret == 0:
            ret = enable_aer(bdf)

    sys.exit(ret)


if __name__ == "__main__":
    main()
