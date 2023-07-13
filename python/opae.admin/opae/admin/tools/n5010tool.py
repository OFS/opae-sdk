#!/usr/bin/env python3
# Copyright(c) 2023, Silicom Denmark A/S
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
# POSSIBILITY OF SUCH DAMAGE.import opae.admin.tools.regmap_debugfs as regm

import argparse
import math
import sys
import re
import os

import opae.admin.tools.regmap_debugfs as regm
from opae.admin.utils import max10_or_nios_version
from opae.admin.fpga import fpga

regmap_path = '/sys/kernel/debug/regmap/'

PCI_VENDOR_ID_SILICOM_DENMARK = 0x1c2c
N5010_BMC_VERSION_QSFP_RXPWR = 0x10c00  # version 1.12.0

N5010_BMC_OFFSET = 0x300800
N5010_QSFP0_RXPWR = 0x1c8
N5010_PHY_CSR_0 = 0x40c
N5010_PHY_CSR_1 = 0x410
N5010_PHY_ABSENT_0 = 1 << 7
N5010_PHY_ABSENT_1 = 1 << 23


def normalize_bdf(bdf):
    pat = r'[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$'
    if re.match(pat, bdf):
        return bdf

    if re.match(r'[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$', bdf):
        return "0000:{}".format(bdf)
    sys.stderr.write('invalid bdf: {}\n'.format(bdf))
    raise SystemExit(os.EX_USAGE)


def init_regmap(regmap_dir):
    """Returns an initialized regmap object"""
    try:
        regm.is_regmap_dir(regmap_dir)
    except argparse.ArgumentTypeError:
        sys.stderr.write(
            'Regmap not found, path name: {}. Root privileges required.\n'.
            format(regmap_dir))
        raise SystemExit(os.EX_UNAVAILABLE)

    regmap = regm.RegmapDebugfs(regmap_dir)
    regmap.get_range_info()
    regmap.get_register_info()
    return regmap


def read_power(regmap):
    """Returns list for each QSFP of list of Rx pwr"""
    qsfps = []
    for i in range(0, 16, 4):
        rx_pwrs = []
        addr = N5010_BMC_OFFSET + N5010_QSFP0_RXPWR + i
        val = regmap.reg_read(addr)

        for f in range(0, 4):
            pwr = val & 0xff
            rx_pwrs.append(pwr)
            val >>= 8
        qsfps.append(rx_pwrs)
    return qsfps


def read_qsfp_insert(regmap):
    """Returns list of QSFP present bools."""
    qsfps = []
    for p in range(0, 4):
        if p == 0:
            offset = N5010_PHY_CSR_1
            bit = N5010_PHY_ABSENT_0
        elif p == 1:
            offset = N5010_PHY_CSR_1
            bit = N5010_PHY_ABSENT_1
        elif p == 2:
            offset = N5010_PHY_CSR_0
            bit = N5010_PHY_ABSENT_0
        elif p == 3:
            offset = N5010_PHY_CSR_0
            bit = N5010_PHY_ABSENT_1

        val = regmap.reg_read(N5010_BMC_OFFSET + offset)
        val &= bit
        present = (val == 0)
        qsfps.append(present)
    return qsfps


def pwr2str(qsfps, unit):
    out = []
    for rx_pwrs in qsfps:
        out2 = []
        for rx_pwr in rx_pwrs:
            if rx_pwr == 0:
                out3 = "   NA"
            else:
                # val read is in steps of 25.6 uW
                val = 25.6 * rx_pwr
                if unit == 'uW':
                    out3 = f"{val:>5.0f}"
                else:
                    # conversion from uW to dBm
                    val = 10.0 * math.log10(val * 0.001)
                    out3 = f"{val:>5.2f}"
            out2.append(out3)
        out.append(out2)
    return out


def print_power(qsfps, uW, qspfs_insert):
    """Print QSFP present and Rx pwr in chosen unit for each QSFP"""
    if uW:
        unit = 'uW'
    else:
        unit = 'dbm'
    val = pwr2str(qsfps, unit)
    port = 0
    print("RX pwr: present\t lane1 \t lane2 \t lane3 \t lane4 \t unit")
    for qsfp, present in zip(val, qspfs_insert):
        if present:
            qsfp_present = f"yes"
        else:
            qsfp_present = f"no"
        p_str = f"Port {port}: {qsfp_present:>7s}\t {qsfp[0]:s}\t " +\
                f"{qsfp[1]:s}\t {qsfp[2]:s}\t {qsfp[3]:s}\t {unit}"
        print(p_str)
        port += 1


def main():
    descr = 'Show QSFP status and optical Rx power levels for N501x card(s)'
    parser = argparse.ArgumentParser(description=descr)
    parser.add_argument('bdf', nargs='?',
                        help=('PCIe address '
                              '(eg 04:00.0 or 0000:04:00.0)'))
    parser.add_argument('--uW', action='store_true',
                        help='output in mW instead of dBm')

    args = parser.parse_args()
    if args.bdf:
        args.bdf = normalize_bdf(args.bdf)
        compatible = fpga.enum([
            {'pci_node.pci_address': str(args.bdf),
             'pci_node.vendor_id': PCI_VENDOR_ID_SILICOM_DENMARK}
             ])
    else:
        compatible = fpga.enum([
            {'pci_node.vendor_id': PCI_VENDOR_ID_SILICOM_DENMARK}
            ])

    if not compatible:
        sys.stderr.write('No compatible devices found\n')
        raise SystemExit(os.EX_USAGE)

    for c in compatible:
        print("\nQSFP info for card at: {}".format(c.pci_node.pci_address))
        spi_bus = str(c.fme.spi_bus)

        if not (c.fme.bmcfw_version >=
                max10_or_nios_version(N5010_BMC_VERSION_QSFP_RXPWR)):
            sys.stderr.write(
                f'Device with BMC {c.fme.bmcfw_version} not supported\n')
        else:
            regmap = init_regmap(regmap_path+spi_bus)
            rx_pwrs = read_power(regmap)
            qsfps_insert = read_qsfp_insert(regmap)
            print_power(rx_pwrs, args.uW, qsfps_insert)


if __name__ == "__main__":
    main()
