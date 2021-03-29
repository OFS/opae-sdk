#! /usr/bin/env python3
# Copyright(c) 2021, Intel Corporation
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

import re
import os
import glob
import argparse
import sys
import traceback
import fcntl
import stat
import struct
import mmap
import time
from ctypes import c_uint64, Structure, Union, c_uint32
from pyopaeuio import pyopaeuio
from enum import Enum


PATTERN = (r'.*(?P<segment>\w{4}):(?P<bus>\w{2}):'
           r'(?P<dev>\w{2})\.(?P<func>\d).*')

FPGA_ROOT_PATH = '/sys/class/fpga_region'

BDF_PATTERN = re.compile(PATTERN)

DEFAULT_BDF = 'ssss:bb:dd.f'

# 100 milli seconds
HSSI_POLL_SLEEP_TIME = 0.1
# timeout 1 sec
HSSI_POLL_TIMEOUT = 0.5


class HSSI_CSR(Enum):
    """
    HSSI DFH CSR offset
    """
    HSSI_DFH_LOW = 0x0
    HSSI_DFH_HIGH = 0x4
    HSSI_VERSION = 0x8
    HSSI_FEATURE_LIST = 0xC
    HSSI_INTER_ATTRIB_PORT = 0x10
    HSSI_CTL_STS = 0x50
    HSSI_CTL_ADDRESS = 0x54
    HSSI_WR_DATA = 0x58
    HSSI_RD_DATA = 0x5C
    HSSI_GM_TX_LATENCY = 0x60
    HSSI_GM_RX_LATENCY = 0x64
    HSSI_ETH_PORT_STATUS = 0x68
    HSSI_TSE_CONTROL = 0xA8


class dfh_bits(Structure):
    """
    HSSI Device Feature Header Low CSR bits
    """
    _fields_ = [
                    ("id", c_uint64, 12),
                    ("revision", c_uint64, 4),
                    ("next_header_offset", c_uint64, 24),
                    ("eol", c_uint64, 1),
                    ("reserved", c_uint64, 19),
                    ("type", c_uint64, 4)
    ]


class dfh(Union):
    """
    HSSI Device Feature Header Low
    Byte Offset: 0x0
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", dfh_bits),
                ("value", c_uint64)]

    def __init__(self, value):
        self.value = value

    @property
    def id(self):
        return self.bits.id

    @property
    def revision(self):
        return self.bits.revision

    @property
    def next_header_offset(self):
        return self.bits.next_header_offset

    @property
    def eol(self):
        return self.bits.eol

    @property
    def type(self):
        return self.bits.type


class dfh_hssi_lo_bits(Structure):
    """
    HSSI Device Feature Header low CSR bits
    """
    _fields_ = [
                   ("id", c_uint32, 12),
                   ("revision", c_uint32, 4),
                   ("next_header_offset", c_uint32, 24)
    ]


class dfh_hssi_lo(Union):
    """
    HSSI Device Feature Header low
    Byte Offset: 0x0
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", dfh_hssi_lo_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def id(self):
        return self.bits.id

    @property
    def revision(self):
        return self.bits.revision

    @property
    def next_header_offset(self):
        return self.bits.next_header_offset


class dfh_hssi_hi_bits(Structure):
    """
    HSSI Device Feature Header high CSR bits
    """
    _fields_ = [
                   ("next_header_offset", c_uint32, 8),
                   ("eol", c_uint32, 1),
                   ("reserved", c_uint32, 19),
                   ("type", c_uint32, 4)
    ]


class dfh_hssi_hi(Union):
    """
    HSSI Device Feature Header High
    Byte Offset: 0x4
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", dfh_hssi_hi_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def next_header_offset(self):
        return self.bits.next_header_offset

    @property
    def eol(self):
        return self.bits.eol

    @property
    def type(self):
        return self.bits.type


class hssi_ver_bits(Structure):
    """
    HSSI feature version CSR bits
    """
    _fields_ = [
                   ("reserved", c_uint32, 8),
                   ("minor", c_uint32, 1),
                   ("major", c_uint32, 19)
    ]


class hssi_ver(Union):
    """
    HSSI feature version
    Byte Offset: 0x8
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_ver_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    def __str__(self):
        val = '{}.{}'.format(self.bits.major,
                             self.bits.minor)
        return val

    @property
    def minor(self):
        return self.bits.minor

    @property
    def major(self):
        return self.bits.major


class hssi_feature_bits(Structure):
    """
    HSSI Device Feature list CSR bits
    """
    _fields_ = [
                   ("axi4_support", c_uint32, 1),
                   ("num_hssi_ports", c_uint32, 4),
                   ("reserved", c_uint32, 26)
    ]


class hssi_feature(Union):
    """
    HSSI feature list
    Byte Offset: 0xc
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_feature_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def axi4_support(self):
        return self.bits.axi4_support

    @property
    def num_hssi_ports(self):
        return self.bits.num_hssi_ports

    def set_num_hssi_ports(self, value):
        self.bits.num_hssi_ports = value


class HSSI_PORT_BUSWIDTH(Enum):
    """
    HSSI DFH CSR offset
    """
    HSSI_400GAUI_8 = 0x0
    HSSI_400GAUI_4 = 0x1
    HSSI_VERSION = 0x8
    HSSI_FEATURE_LIST = 0xC
    HSSI_INTER_ATTRIB_PORT = 0x10
    HSSI_CTL_STS = 0x50
    HSSI_CTL_ADDRESS = 0x54
    HSSI_WR_DATA = 0x58
    HSSI_RD_DATA = 0x5C
    HSSI_GM_TX_LATENCY = 0x60
    HSSI_GM_RX_LATENCY = 0x64
    HSSI_ETH_PORT_STATUS = 0x68
    HSSI_TSE_CONTROL = 0xA8


class hssi_port_attribute_bits(Structure):
    """
    HSSI Interface Attribute Port X Parameters CSR bits
    """
    _fields_ = [
                   ("port_profiles", c_uint32, 6),
                   ("port_read_latency", c_uint32, 4),
                   ("port_databus_width", c_uint32, 3),
                   ("low_speed_eth", c_uint32, 2),
                   ("dyn_reconf", c_uint32, 1),
                   ("reserved", c_uint32, 16)
    ]


class hssi_port_attribute(Union):
    """
    HSSI Interface Attribute Port X Parameters
    Byte Offset: 0x10 + X * 4 (X = 0 - 15)
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_port_attribute_bits),
                ("value", c_uint32)]

    HSSI_PORT_BUSWIDTH = ((0, 32),
                          (1, 64),
                          (2, 128),
                          (3, 256),
                          (4, 512),
                          (5, 1024))

    HSSI_PORT_PROFILES = ((32, '400GAUI-8'),
                          (31, '400GAUI-4'),
                          (30, '200GAUI-8'),
                          (29, '200GAUI-4'),
                          (28, '200GAUI-2'),
                          (27, '100GCAUI-4'),
                          (26, '100GAUI-2'),
                          (25, '100GAUI-1'),
                          (24, '50GAUI-1'),
                          (23, '50GAUI-2'),
                          (22, '40GCAUI-4'),
                          (21, '5GbE'),
                          (20, '10GbE'),
                          (19, 'Ethernet PMA-Direct'),
                          (18, 'Ethernet FEC-Direct'),
                          (17, 'Ethernet PCS-Direct'),
                          (16, 'MII'),
                          (15, 'General PMA-Direct'),
                          (14, 'General FEC-Direct'),
                          (13, 'General PCS-Direct'),
                          (12, 'OTN'),
                          (11, 'Flex-E'),
                          (10, 'General FEC-Direct'),
                          (9, 'TSE MAC'))

    HSSI_SPEED_ETH_INTER = ((0, 'MII'),
                            (1, 'GMII'),
                            (2, 'XGMII'))

    def __init__(self, value):
        self.value = value

    @property
    def port_read_latency(self):
        return self.bits.port_read_latency

    @property
    def port_databus_width(self):
        return self.bits.port_databus_width

    @property
    def low_speed_eth(self):
        return self.bits.low_speed_eth

    @property
    def dyn_reconf(self):
        return self.bits.dyn_reconf


class hssi_cmd_sts_bits(Structure):
    """
    HSSI Feature Command/Status CSR bits
    """
    _fields_ = [
                   ("read", c_uint32, 1),
                   ("write", c_uint32, 1),
                   ("ack", c_uint32, 1),
                   ("busy", c_uint32, 1),
                   ("error", c_uint32, 1),
                   ("reserved", c_uint32, 27)
    ]


class hssi_cmd_sts(Union):
    """
    HSSI feature Feature Command/Status
    Byte Offset: 0x50
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_cmd_sts_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def ack(self):
        return self.bits.ack

    @property
    def busy(self):
        return self.bits.busy

    @property
    def error(self):
        return self.bits.error

    def set_read(self, value):
        self.bits.read = value

    def set_write(self, value):
        self.bits.write = value

    def set_ack(self, value):
        self.bits.ack = value


class HSSI_SALCMD(Enum):
    """
    HSSI Feature Control/Address Enum
    """
    NOP = 0
    GET_HSSI_PROFILE = 0x1
    SET_HSSI_PROFILE = 0x2
    READ_MAC_STATISTIC = 0x3
    GET_MTU = 0x4
    SET_SCR = 0x5
    GET_SCR = 0x6
    ENABLE_LOOPBACK = 0x7
    DISABLE_LOOPBACK = 0x8
    FIRMWARE_VER = 0xFF


class hssi_ctl_addr_bits(Structure):
    """
    HSSI Feature Control/Address CSR bits
    """
    _fields_ = [
                   ("sal_cmd", c_uint32, 8),
                   ("port_address", c_uint32, 4),
                   ("ch_address", c_uint32, 4),
                   ("addressbit", c_uint32, 16)
    ]


class hssi_ctl_addr(Union):
    """
    HSSI Feature Control/Address CSR
    Byte Offset: 0x54
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_ctl_addr_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def sal_cmd(self):
        return self.bits.sal_cmd

    @property
    def port_address(self):
        return self.bits.port_address

    @property
    def ch_address(self):
        return self.bits.ch_address

    @property
    def addressbit(self):
        return self.bits.addressbit

    def set_sal_cmd(self, value):
        self.bits.sal_cmd = value

    def set_port_address(self, value):
        self.bits.port_address = value

    def set_ch_address(self, value):
        self.bits.ch_address = value

    def set_addressbit(self, value):
        self.bits.addressbit = value


class hssi_wrdata_bits(Structure):
    """
    HSSI Feature Write Data CSR bits
    """
    _fields_ = [
                   ("wrdata", c_uint32, 32)
    ]


class hssi_wrdata(Union):
    """
    HSSI Feature Write Data CSR
    Byte Offset: 0x58
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_wrdata_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def wrdata(self):
        return self.bits.wrdata


class hssi_rddata_bits(Structure):
    """
    HSSI Feature Read Data CSR bits
    """
    _fields_ = [
                   ("rddata", c_uint32, 32)
    ]


class hssi_rddata(Union):
    """
    HSSI Feature Read Data CSR
    Byte Offset: 0x5c
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_rddata_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def rddata(self):
        return self.bits.rddata


class hssi_eth_port_status_bits(Structure):
    """
    HSSI Ethernet Port X Status bits
    """
    _fields_ = [
                   ("o_ehip_ready:", c_uint32, 1),
                   ("o_rx_hi_ber", c_uint32, 1),
                   ("o_cdr_lock", c_uint32, 1),
                   ("rx_am_lock", c_uint32, 1),
                   ("rx_block_lock", c_uint32, 1),
                   ("link_fault_gen_en", c_uint32, 1),
                   ("unidirectional_en", c_uint32, 1),
                   ("local_fault_status", c_uint32, 1),
                   ("remote_fault_status", c_uint32, 1),
                   ("unidirectional_force_remote_faul", c_uint32, 1),
                   ("unidirectional_remote_fault_dis", c_uint32, 1),
                   ("pcs_eccstatus", c_uint32, 2),
                   ("mac_eccstatus", c_uint32, 2),
                   ("set_10", c_uint32, 1),
                   ("set_1000", c_uint32, 1),
                   ("ena_10", c_uint32, 1),
                   ("eth_mode", c_uint32, 1),
                   ("reserved", c_uint32, 12)
    ]


class hssi_eth_port_status(Union):
    """
    10.1.12	HSSI Ethernet Port X Status
    Byte Offset: 0x68 + X (0x00 .. 0x0F)*4
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_eth_port_status_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def o_ehip_ready(self):
        return self.bits.o_ehip_ready

    @property
    def o_rx_hi_ber(self):
        return self.bits.o_rx_hi_ber

    @property
    def o_cdr_lock(self):
        return self.bits.o_cdr_lock

    @property
    def rx_am_lock(self):
        return self.bits.rx_am_lock

    @property
    def rx_block_lock(self):
        return self.bits.rx_block_lock

    @property
    def link_fault_gen_en(self):
        return self.bits.link_fault_gen_en

    @property
    def unidirectional_en(self):
        return self.bits.unidirectional_en

    @property
    def local_fault_status(self):
        return self.bits.local_fault_status

    @property
    def remote_fault_status(self):
        return self.bits.remote_fault_status

    @property
    def unidirectional_force_remote_fault(self):
        return self.bits.unidirectional_force_remote_fault

    @property
    def unidirectional_remote_fault_dis(self):
        return self.bits.unidirectional_remote_fault_dis

    @property
    def pcs_eccstatus(self):
        return self.bits.pcs_eccstatus

    @property
    def mac_eccstatus(self):
        return self.bits.mac_eccstatus

    @property
    def set_10(self):
        return self.bits.set_10

    @property
    def set_1000(self):
        return self.bits.set_1000

    @property
    def ena_10(self):
        return self.bits.ena_10

    @property
    def eth_mode(self):
        return self.bits.eth_mode


class FpgaFinder(object):
    def __init__(self, pcie_address):
        self._pice_address = pcie_address
        self.all_devs = []
        self.match_dev = []
        self.get_fpga_device_list()

    def read_bdf(self, path):
        symlink = os.readlink(path)
        m = BDF_PATTERN.match(symlink)
        data = m.groupdict() if m else {}
        return dict([(k, int(v, 16)) for (k, v) in data.items()])

    def get_fpga_device_list(self):
        if os.path.exists(FPGA_ROOT_PATH):
            paths = glob.glob(os.path.join(FPGA_ROOT_PATH, 'region*'))
            for p in paths:
                sbdf = self.read_bdf(os.path.join(p, 'device'))
                if sbdf:
                    sbdf['path'] = p
                    pcie_address = None
                    sbdf_str = '{0:04x}:{1:02x}:{2:02x}.{3:01x}'
                    pcie_address = sbdf_str.format(sbdf.get('segment'),
                                                   sbdf.get('bus'),
                                                   sbdf.get('dev'),
                                                   sbdf.get('func'))
                    sbdf['pcie_address'] = pcie_address
                    if self._pice_address == pcie_address:
                        self.all_devs.append(sbdf)
                    if self._pice_address is None:
                        self.all_devs.append(sbdf)

    def enum(self):
        if not self.all_devs:
            print('No FPGA device find at {}'.format(FPGA_ROOT_PATH))
        for dev in self.all_devs:
            self.match_dev.append(dev)
        return self.match_dev

    def find_node(self, root, node, depth=5):
        paths = []
        for x in range(depth):
            r = glob.glob(os.path.join(os.path.join(root, *['*'] * x), node))
            paths.extend(r)
        return paths

    def find_hssi_group(self, root):
        hssi_group = {}
        paths = glob.glob("/sys/bus/dfl/drivers/uio_dfl/dfl_*")
        i = 0
        feature_id = 0
        uio_path = 0
        for path in paths:
            with open(os.path.join(path, 'feature_id'), 'r') as fd:
                feature_id = fd.read().strip()

            if feature_id != '0x10':
                continue

            uio_path = glob.glob(os.path.join(path, "uio/uio*"))

            if len(uio_path) == 0:
                continue
            dfl_dev_name = path.split("/sys/bus/dfl/drivers/uio_dfl/")
            hssi_group[i] = [dfl_dev_name[1], uio_path]
            i = i + 1
        return hssi_group


class HSSICOMMON(object):
    def __init__(self):
        self.eth_inst = None
        self.pyopaeuio_inst = pyopaeuio()
        self.num_uio_regions = 0
        self.region_index = 0

    def hssi_info(self, hssi_uio):
        """
        Reads info fron hssi feature uio region
        prints id, verion, number of ports
        and firmware version
        """
        try:
            self.pyopaeuio_inst.open(hssi_uio)
            self.num_uio_regions = self.pyopaeuio_inst.numregions
            hssi_dfh = dfh(self.read32(0, 0))
            hssi_version = hssi_ver(self.read32(0, 0x8))
            hssi_feature_list = hssi_feature(self.read32(0, 0xC))
            ctl_addr = hssi_ctl_addr(0)
            ctl_addr.set_sal_cmd(HSSI_SALCMD.FIRMWARE_VER.value)
            firmware_version = self.read_reg(0, ctl_addr.value)

            print("\n--------HSSI IINFO START-------")
            print("HSSI id: {0: >12}".format(hex(hssi_dfh.id)))
            print("HSSI version: {0: >12}".format(str(hssi_version)))
            print("HSSI num ports: {0: >12}"
                  .format(hssi_feature_list.num_hssi_ports))
            print("Firmware Version: {0: >12}".format(firmware_version))
            print("--------HSSI INFO END------- \n")

            self.close()

        except RuntimeError:
            print("opae uio module exception")
        except ValueError:
            print("Invalid arguemnts")

        return 0

    def open(self, hssi_uio):
        ret = self.pyopaeuio_inst.open(hssi_uio)
        return ret

    def close(self):
        ret = self.pyopaeuio_inst.close()
        return ret

    def write32(self, region_index, offset, value):
        ret = self.pyopaeuio_inst.write32(region_index, offset, value)
        return ret

    def read32(self, region_index, offset):
        value = self.pyopaeuio_inst.read32(region_index, offset)
        return value

    def write64(self, region_index, offset, value):
        ret = self.pyopaeuio_inst.write64(region_index, offset, value)
        return ret

    def read64(self, region_index, offset):
        value = self.pyopaeuio_inst.read64(region_index, offset)
        return value

    def clear_reg(self,
                  region_index,
                  reg_offset):
        """
        Read CTL Address CSR
        Write cmd to CTL Address CSR
        Write cmd to CTL status CSR
        poll for status
        Read Data
        """
        total_time = 0
        while(True):
            reg_data = self.read32(region_index, reg_offset)
            if reg_data == 0:
                return True
            self.write32(region_index, reg_offset, 0x0)

            time.sleep(HSSI_POLL_SLEEP_TIME)
            if total_time > HSSI_POLL_TIMEOUT:
                return False

            total_time = HSSI_POLL_SLEEP_TIME + total_time

        return False

    def clear_ctl_sts_reg(self, region_index):
        """
        Read CTL Address CSR
        Write cmd to CTL Address CSR
        Write cmd to CTL status CSR
        poll for status
        Read Data
        """
        ctl_addr_value = self.read32(region_index,
                                     HSSI_CSR.HSSI_CTL_ADDRESS.value)
        if ctl_addr_value != 0:
            ret = self.clear_reg(region_index,
                                 HSSI_CSR.HSSI_CTL_ADDRESS.value)
            if not ret:
                print("Failed to clear HSSI CTL Address csr")
                return False

        cmd_sts_value = self.read32(region_index, HSSI_CSR.HSSI_CTL_STS.value)
        if cmd_sts_value != 0:
            ret = self.clear_reg(region_index,
                                 HSSI_CSR.HSSI_CTL_STS.value)
            if not ret:
                print("Failed to clear HSSI CTL Address csr")
                return False

        return True

    def read_poll_timeout(self,
                          region_index,
                          reg_offset,
                          bit_index):
        """
        Read CTL Address CSR
        Write cmd to CTL Address CSR
        Write cmd to CTL status CSR
        poll for status
        Read Data
        """
        total_time = 0
        while(True):
            reg_data = self.read32(region_index, reg_offset)
            reg_data &= (1 << bit_index)
            if reg_data == 1:
                return True

            time.sleep(HSSI_POLL_SLEEP_TIME)
            if total_time > HSSI_POLL_TIMEOUT:
                return False
            total_time = HSSI_POLL_SLEEP_TIME + total_time

        return False

    def read_reg(self, region_index, reg_data):
        """
        Read CTL Address CSR
        Write cmd to CTL Address CSR
        Write cmd to CTL status CSR
        poll for status
        Read Data
        """
        ret = self.clear_ctl_sts_reg(region_index)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            return 0

        self.write32(region_index, HSSI_CSR.HSSI_CTL_ADDRESS.value, reg_data)

        cmd_sts = hssi_cmd_sts(0x1)
        self.write32(region_index, HSSI_CSR.HSSI_CTL_STS.value, cmd_sts.value)

        if not self.read_poll_timeout(region_index,
                                      HSSI_CSR.HSSI_CTL_STS.value,
                                      0x3):
            print("HSSI ctl sts csr fails to update ACK")
            return 0

        value = self.read32(region_index, HSSI_CSR.HSSI_RD_DATA.value)

        ret = self.clear_ctl_sts_reg(region_index)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            return 0

        return value

    def write_reg(self, region_index, reg_data, value):
        """
        Read CTL Address CSR
        if not zero clear it
        Write cmd to CTL Address CSR
        Write cmd to CTL status CSR
        poll for status
        Write Data
        """
        ret = self.clear_ctl_sts_reg(region_index)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            return 0

        cmd_sts = hssi_cmd_sts(0x2)
        self.write32(region_index, HSSI_CSR.HSSI_CTL_STS.value, cmd_sts.value)

        if not self.read_poll_timeout(region_index,
                                      HSSI_CSR.HSSI_CTL_STS.value,
                                      0x2):
            print("HSSI ctl sts csr fails to update ACK")
            return 0

        self.write32(region_index, HSSI_CSR.HSSI_WR_DATA.value, value)

        ret = self.clear_ctl_sts_reg(region_index)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            return 0

        return True

    def register_field_set(self, reg_data, idx, width, value):
        mask = 0
        for x in range(width):
            mask |= (1 << x)
        value &= mask
        reg_data &= ~(mask << idx)
        reg_data |= (value << idx)
        return reg_data


def main():
    """
    parse input arguemnts pciaddress
    """
    parser = argparse.ArgumentParser()

    pcieaddress_help = 'bdf of device to program \
                        (e.g. 04:00.0 or 0000:04:00.0).' \
                       ' Optional when one device in system.'
    parser.add_argument('--pcie-address', '-P',
                        default=None, help=pcieaddress_help)

    args, left = parser.parse_known_args()

    print(args)
    print("pcie_address:", args.pcie_address)

    f = FpgaFinder(args.pcie_address)
    devs = f.enum()
    for d in devs:
        print('sbdf: {segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**d))
        print('FPGA dev:', d)
    if len(devs) > 1:
        print('{} FPGAs are found\nplease choose '
              'one FPGA'.format(len(devs)))
        sys.exit(1)
    if not devs:
        print('no FPGA found')
        sys.exit(1)


if __name__ == "__main__":
    main()
