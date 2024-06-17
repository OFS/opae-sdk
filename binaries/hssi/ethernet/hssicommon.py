#! /usr/bin/env python3
# Copyright(c) 2021-2022, Intel Corporation
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

BDF_PATTERN = re.compile(PATTERN, re.IGNORECASE)

DEFAULT_BDF = 'ssss:bb:dd.f'

# mailbox register poll interval 1 microseconds
HSSI_POLL_SLEEP_TIME_SEC = 1/1000000

# Mailbox register poll timeout 1 second. Responses are typically much
# faster than one second, but in rare cases can take a while. No reason
# to make this short since timeout is generally a failure.
HSSI_POLL_TIMEOUT_SEC = 1

HSSI_FEATURE_ID = 0x15


class HSSI_DFHV0_CSR(object):
    """
    HSSI DFHV0 csr offsets
    """
    def __init__(self):
        self.HSSI_DFH = 0x0
        self.HSSI_VERSION = 0x8
        self.HSSI_FEATURE_LIST = 0xC
        self.HSSI_INTER_ATTRIB_PORT = 0x10
        self.HSSI_CTL_STS = 0x50
        self.HSSI_CTL_ADDRESS = 0x54
        self.HSSI_RD_DATA = 0x58
        self.HSSI_WR_DATA = 0x5C
        self.HSSI_GM_TX_LATENCY = 0x60
        self.HSSI_GM_RX_LATENCY = 0x64
        self.HSSI_ETH_PORT_STATUS = 0x68
        self.HSSI_TSE_CONTROL = 0xA8
        self.HSSI_PORT_COUNT = 16


class HSSI_DFHV05_CSR(object):
    """
    HSSI DFHV 0.5 csr offset
    """
    def __init__(self):
        self.csr_offset = 0
        self.HSSI_DFH = 0x0
        self.HSSI_GUID_L = 0x8
        self.HSSI_GUID_H = 0x10
        self.FEATUR_ADDR_CSR = 0x18
        self.FEATURE_SIZE_GROUP = 0x20
        self.HSSI_VERSION = 0
        self.HSSI_FEATURE_LIST = 0
        self.HSSI_INTER_ATTRIB_PORT = 0
        self.HSSI_CTL_STS = 0
        self.HSSI_CTL_ADDRESS = 0
        self.HSSI_RD_DATA = 0
        self.HSSI_WR_DATA = 0
        self.HSSI_GM_TX_LATENCY = 0
        self.HSSI_GM_RX_LATENCY = 0
        self.HSSI_PORT_STATUS = 0
        self.HSSI_TSE_CONTROL = 0
        self.HSSI_DBG_CONTROL = 0
        self.HSSI_PORT_COUNT = 20
        self.HSSI_DFHV05_GUILDL = 0x99a078ad18418b9d
        self.HSSI_DFHV05_GUILDH = 0x4118a7cbd9db4a9b
        self.HSSI_NCSI_CH_SEL = 0x830

    def set_csr_dfhv05_offset(self, offset):
        self.csr_offset = offset
        self.HSSI_VERSION = self.csr_offset
        self.HSSI_FEATURE_LIST = 0x4 + self.csr_offset
        self.HSSI_INTER_ATTRIB_PORT = 0x8 + self.csr_offset
        self.HSSI_CTL_STS = 0x48 + self.csr_offset
        self.HSSI_CTL_ADDRESS = 0x4C + self.csr_offset
        self.HSSI_RD_DATA = 0x50 + self.csr_offset
        self.HSSI_WR_DATA = 0x54 + self.csr_offset
        self.HSSI_GM_TX_LATENCY = 0x58 + self.csr_offset
        self.HSSI_GM_RX_LATENCY = 0x5C + self.csr_offset
        self.HSSI_PORT_STATUS = 0xC0 + self.csr_offset
        self.HSSI_TSE_CONTROL = 0xA0 + self.csr_offset
        self.HSSI_DBG_CONTROL = 0xB8 + self.csr_offset


class dfh_bits(Structure):
    """
    HSSI Device Feature Header CSR bits
    """
    _fields_ = [
                    ("id", c_uint64, 12),
                    ("feature_rev", c_uint64, 4),
                    ("next_header_offset", c_uint64, 24),
                    ("eol", c_uint64, 1),
                    ("reserved", c_uint64, 7),
                    ("feature_minor_rev", c_uint64, 4),
                    ("dfh_verion", c_uint64, 8),
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
    def feature_rev(self):
        return self.bits.feature_rev

    @property
    def next_header_offset(self):
        return self.bits.next_header_offset

    @property
    def eol(self):
        return self.bits.eol

    @property
    def feature_minor_rev(self):
        return self.bits.feature_minor_rev

    @property
    def dfh_verion(self):
        return self.bits.dfh_verion

    @property
    def type(self):
        return self.bits.type


class dfh_hssi_lo_bits(Structure):
    """
    HSSI Device Feature Header low CSR bits
    """
    _fields_ = [
                   ("id", c_uint32, 12),
                   ("feature_rev", c_uint32, 4),
                   ("next_header_offset", c_uint32, 16)
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
    def feature_rev(self):
        return self.bits.feature_rev

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
                   ("reserved", c_uint32, 7),
                   ("feature_minor_rev", c_uint64, 4),
                   ("dfh_verion", c_uint64, 8),
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
    def feature_minor_rev(self):
        return self.bits.feature_minor_rev

    @property
    def type(self):
        return self.bits.type

    @property
    def dfh_verion(self):
        return self.bits.dfh_verion

    @property
    def type(self):
        return self.bits.type


class csr_addr_bits(Structure):
    """
    HSSI Feature CSR address CSR bits
    """
    _fields_ = [
                    ("rel", c_uint64, 1),
                    ("addr", c_uint64, 63)
    ]


class csr_addr(Union):
    """
    HSSI Feature CSR address
    Byte Offset: 0x18
    Addressing Mode: 64 bits
    """
    _fields_ = [("bits", csr_addr_bits),
                ("value", c_uint64)]

    def __init__(self, value):
        self.value = value

    @property
    def addr(self):
        return self.bits.addr

    @property
    def rel(self):
        return self.bits.rel


class csr_group_bits(Structure):
    """
    HSSI Feature CSR size group bits
    """
    _fields_ = [
                    ("instance_id", c_uint64, 16),
                    ("grouping_id", c_uint64, 15),
                    ("has_params", c_uint64, 1),
                    ("size", c_uint64, 32)
    ]


class csr_size(Union):
    """
    HSSI Feature CSR size group
    Byte Offset: 0x20
    Addressing Mode: 64 bits
    """
    _fields_ = [("bits", csr_group_bits),
                ("value", c_uint64)]

    def __init__(self, value):
        self.value = value

    @property
    def size(self):
        return self.bits.size

    @property
    def has_params(self):
        return self.bits.has_params

    @property
    def instance_id(self):
        return self.bits.instance_id

    @property
    def grouping_id(self):
        return self.bits.grouping_id


class hssi_ver_bits(Structure):
    """
    HSSI feature version CSR bits
    """
    _fields_ = [
                   ("reserved", c_uint32, 8),
                   ("minor", c_uint32, 8),
                   ("major", c_uint32, 16)
    ]


class hssi_ver(Union):
    """
    HSSI feature version
    Byte Offset dfv0: 0x8
    Byte Offset dfv0.5: CSR_ADDROFF
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
                   ("num_hssi_ports", c_uint32, 5),
                   ("port_enable", c_uint32, 20),
                   ("reserved", c_uint32, 6)
    ]


class hssi_feature(Union):
    """
    HSSI feature list
    Byte Offset dfv0: 0xc
    Byte Offset dfv0.5: 0x4 + CSR_ADDROFF
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

    @property
    def port_enable(self):
        return self.bits.port_enable

    @num_hssi_ports.setter
    def num_hssi_ports(self, value):
        self.bits.num_hssi_ports = value


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
                   ("port_sub_profiles", c_uint32, 5),
                   ("rsfec_enabled", c_uint32, 1),
                   ("anlt_enabled", c_uint32, 1),
                   ("ptp_enabled", c_uint32, 1),
                   ("reserved", c_uint32, 8)
    ]


class hssi_port_attribute(Union):
    """
    HSSI Interface Attribute Port X Parameters
    Byte Offset dfv0: 0x10 + X * 4 (X = 0 - num ports)
    Byte Offset dfv0.5: 0x8 + X * 4 (X = 0 - num ports) + CSR_ADDROFF
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

    HSSI_PORT_PROFILES = ((33, 'CRI'),
                          (32, '400GAUI-8'),
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
                          (21, '25GbE'),
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
                          (10, 'TSE MAC'),
                          (9, 'TSE PCS'),
                          (8, 'LL10G'),
                          (7, 'MRPHY'),
                          (6, '10_25G'),
                          (5, '25_50G'),
                          (4, 'Ultra40G'),
                          (3, 'LL40G'),
                          (2, 'LL50G'),
                          (1, 'Ultra100G'),
                          (0, 'LL100G'))

    HSSI_SPEED_ETH_INTER = ((0, 'MII'),
                            (1, 'GMII'),
                            (2, 'XGMII'))

    HSSI_PORT_SUBPROFILES = ((15, '24G PCS'),
                             (14, '12G PCS'),
                             (13, '10G PCS'),
                             (12, '9.8G PMA'),
                             (11, '8.1G PMA'),
                             (10, '6.1G PMA'),
                             (9, '4.9G PMA'),
                             (8, '3.0G PMA'),
                             (7, '2.4G PMA'),
                             (6, '1.2G PMA'),
                             (5, '0.6G PMA'),
                             (4, 'MAC + PCS'),
                             (3, 'PCS'),
                             (2, 'Flex-E'),
                             (1, 'OTN'),
                             (0, 'None'))

    def __init__(self, value):
        self.value = value

    @property
    def port_profiles(self):
        return self.bits.port_profiles

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

    @property
    def port_sub_profiles(self):
        return self.bits.port_sub_profiles

    @property
    def rsfec_enabled(self):
        return self.bits.rsfec_enabled

    @property
    def anlt_enabled(self):
        return self.bits.anlt_enabled

    @property
    def ptp_enabled(self):
        return self.bits.ptp_enabled


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
                   ("regoffset", c_uint32, 2),
                   ("reserved", c_uint32, 25)
    ]


class hssi_cmd_sts(Union):
    """
    HSSI feature Feature Command/Status
    Byte Offset dfv0: 0x50
    Byte Offset dfv0.5: 0x48 + CSR_ADDROFF
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

    @property
    def read(self):
        return self.bits.read

    @property
    def write(self):
        return self.bits.write

    @property
    def regoffset(self):
        return self.bits.regoffset

    @read.setter
    def read(self, value):
        self.bits.read = value

    @write.setter
    def write(self, value):
        self.bits.write = value

    @ack.setter
    def ack(self, value):
        self.bits.ack = value

    @regoffset.setter
    def regoffset(self, value):
        self.bits.regoffset = value


class HSSI_SAL_CMD(Enum):
    """
    HSSI Feature Control/Address Enum
    """
    NOP = 0
    GET_HSSI_PROFILE = 0x1
    SET_HSSI_PROFILE = 0x2
    READ_MAC_STATISTIC = 0x3
    GET_MTU = 0x4
    SET_CSR = 0x5
    GET_CSR = 0x6
    ENABLE_LOOPBACK = 0x7
    DISABLE_LOOPBACK = 0x8
    RESET_MAC_STATISTIC = 0x9
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
    Byte Offset dfv0: 0x54
    Byte Offset dfv0.5: 0x4C + CSR_ADDROFF
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

    @sal_cmd.setter
    def sal_cmd(self, value):
        self.bits.sal_cmd = value

    @port_address.setter
    def port_address(self, value):
        self.bits.port_address = value

    @ch_address.setter
    def ch_address(self, value):
        self.bits.ch_address = value

    @addressbit.setter
    def addressbit(self, value):
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
    Byte Offset dfv0: 0x58
    Byte Offset dfv0.5: 0x54 + CSR_ADDROFF
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
    Byte Offset dfv0: 0x5c
    Byte Offset dfv0.5: 0x50 + CSR_ADDROFF
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
                   ("load_recipe_error", c_uint32, 1),
                   ("ical_pcal_errors", c_uint32, 1),
                   ("tx_lanes_stable", c_uint32, 1),
                   ("rx_pcs_ready", c_uint32, 1),
                   ("tx_pll_locked", c_uint32, 1),
                   ("tx_pll_locked", c_uint32, 2),
                   ("reserved", c_uint32, 5)
    ]


class hssi_eth_port_status(Union):
    """
    HSSI Ethernet Port X Status
    Byte Offset dfv0:  0x68 + X (0x00 .. 0x0F)*4
    Byte Offset dfv0.5: 0xC0 + X (0x00 .. 0x0F) * 4 + CSR_ADDROFF
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


class hssi_dbg_ctl_bits(Structure):
    """
    HSSI Debug Control CSR bits
    """
    _fields_ = [
                   ("portn_led_speed", c_uint32, 3),
                   ("portn_led_status", c_uint32, 3),
                   ("portn_led_status_override", c_uint32, 4),
                   ("led_status_override_enable", c_uint32, 1),
                   ("led_blink_rate", c_uint32, 8),
                   ("reserved", c_uint32, 13)
    ]


class hssi_dbg_ctl(Union):
    """
    HSSI Debug Control CSR
    Byte Offset dfv0.5: 0xB8 + CSR_ADDROFF
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_dbg_ctl_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def portn_led_speed(self):
        return self.bits.portn_led_speed

    @property
    def portn_led_status_override(self):
        return self.bits.portn_led_status_override

    @property
    def led_status_override_enable(self):
        return self.bits.led_status_override_enable

    @property
    def led_blink_rate(self):
        return self.bits.led_blink_rate

    @property
    def led_blink_rate(self):
        return self.bits.led_blink_rate

    @portn_led_speed.setter
    def portn_led_speed(self, value):
        self.bits.portn_led_speed = value

    @portn_led_status_override.setter
    def portn_led_status_override(self, value):
        self.bits.portn_led_status_override = value

    @portn_led_status_override.setter
    def portn_led_status_override(self, value):
        self.bits.portn_led_status_override = value

    @led_status_override_enable.setter
    def led_status_override_enable(self, value):
        self.bits.led_status_override_enable = value

    @led_blink_rate.setter
    def led_blink_rate(self, value):
        self.bits.led_blink_rate = value


class hssi_tse_ctl_bits(Structure):
    """
    HSSI TSE Control CSR bits
    """
    _fields_ = [
                   ("magic_wakeup", c_uint32, 1),
                   ("magic_sleep_n", c_uint32, 3),
                   ("reserved", c_uint32, 30)
    ]


class hssi_tse_ctl(Union):
    """
    HSSI tse Control CSR
    Byte Offset dfv0.5: 0x2C + CSR_ADDROFF
    Addressing Mode: 32 bits
    """
    _fields_ = [("bits", hssi_tse_ctl_bits),
                ("value", c_uint32)]

    def __init__(self, value):
        self.value = value

    @property
    def magic_wakeup(self):
        return self.bits.magic_wakeup

    @property
    def magic_sleep_n(self):
        return self.bits.magic_sleep_n


def verify_pcie_address(pcie_address):
    m = BDF_PATTERN.match(pcie_address)
    if m is None:
        print("Invalid pcie address format{}".format(pcie_address))
        return False
    return True


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

    def find_hssi_group(self, pci_address):
        hssi_group = []
        paths = glob.glob(os.path.join("/sys/bus/pci/devices/",
                                       pci_address,
                                       "fpga_region/region*/dfl-fme*/dfl_dev*"))
        feature_id = 0
        uio_path = 0
        for path in paths:
            with open(os.path.join(path, 'feature_id'), 'r') as fd:
                feature_id = fd.read().strip()
                feature_id = int(feature_id, 16)

            if feature_id != HSSI_FEATURE_ID:
                continue

            uio_path = glob.glob(os.path.join(path, "uio/uio*"))
            if len(uio_path) == 0:
                continue
            m = re.search('dfl_dev(.*)', path)
            if m:
                hssi_group.append((m.group(0), uio_path, pci_address, feature_id))
        return hssi_group


class HSSICOMMON(object):
    def __init__(self):
        self.eth_inst = None
        self.pyopaeuio_inst = pyopaeuio()
        self.num_uio_regions = 0
        self.region_index = 0
        self.hssi_csr = 0

    def verify_hssi_dfh_ver(self):
        hssi_dfh = dfh(self.read64(0, 0))
        print("{0: <24}:{1}".format("DFH", hex(self.read64(0, 0))))
        print("{0: <24}:{1}".format("HSSI ID", hex(hssi_dfh.id)))

        if (hssi_dfh.feature_rev == 0x1) or (hssi_dfh.feature_rev == 0):
            print("{0: <24}:{1}".format("DFHv", 0))
            self.hssi_csr = HSSI_DFHV0_CSR()
        elif (hssi_dfh.feature_rev >= 0x2) and (hssi_dfh.feature_rev < 0xf):
            self.hssi_csr = HSSI_DFHV05_CSR()
            guidl = self.read64(0, self.hssi_csr.HSSI_GUID_L)
            guidh = self.read64(0, self.hssi_csr.HSSI_GUID_H)
            print("{0: <24}:{1}".format("DFHv", 0.5))
            print("{0: <24}:{1}".format("guidl", hex(guidl)))
            print("{0: <24}:{1}".format("guidh", hex(guidh)))

            if guidl != self.hssi_csr.HSSI_DFHV05_GUILDL:
                print("bad guidl {0}!={1}".format(hex(guidl),
                                                  hex(self.hssi_csr.HSSI_DFHV05_GUILDL)))
                return False

            if guidh != self.hssi_csr.HSSI_DFHV05_GUILDH:
                print("bad guidh {0}!={1}".format(hex(guidh),
                                                  hex(self.hssi_csr.HSSI_DFHV05_GUILDH)))
                return False

            hssi_csr_addr = csr_addr(self.read64(0, self.hssi_csr.FEATUR_ADDR_CSR))
            self.hssi_csr.set_csr_dfhv05_offset(hssi_csr_addr.addr)
        else:
            print("dfh feature revision not supported:", hex(hssi_dfh.feature_rev))
            return False

        return True

    def hssi_ver(self):
        hssi_version = hssi_ver(self.read32(0, self.hssi_csr.HSSI_VERSION))
        print("{0: <24}:{1}".format("HSSI version", str(hssi_version)))
        ctl_addr = hssi_ctl_addr(0)
        ctl_addr.sal_cmd = HSSI_SAL_CMD.FIRMWARE_VER.value
        res, firmware_version = self.read_reg(0, ctl_addr.value)
        if not res:
            print("Failed to read  HSSI firmware version")
            return False

        print("{0: <24}:{1}".format("Firmware Version", firmware_version))
        return True

    def hssi_port_info(self):
        hssi_feature_list = hssi_feature(self.read32(0,
                                         self.hssi_csr.HSSI_FEATURE_LIST))
        print("{0: <24}:{1}".format("HSSI num ports",
                                    hssi_feature_list.num_hssi_ports))
        for port in range(0, self.hssi_csr.HSSI_PORT_COUNT):
            enable = self.register_field_get(hssi_feature_list.port_enable,
                                             port)
            if enable == 0:
                continue
            port_attribute = hssi_port_attribute(self.read32(0,
                                                 self.hssi_csr.HSSI_INTER_ATTRIB_PORT + port * 4))
            for profile, pro_str in hssi_port_attribute.HSSI_PORT_PROFILES:
                if port_attribute.port_profiles == profile:
                    print("Port{0:<20}:{1:<25}".format(port, pro_str))
        return True

    def hssi_info(self, hssi_uio):
        """
        Reads info fron hssi feature uio region
        prints id, verion, number of ports
        and firmware version
        """
        try:
            self.open(hssi_uio)
            self.num_uio_regions = self.pyopaeuio_inst.numregions
            print("\n--------HSSI INFO START-------")
            if not self.verify_hssi_dfh_ver():
                self.close()
                return False

            if not self.hssi_ver():
                self.close()
                return False

            self.hssi_port_info()
            print("--------HSSI INFO END------- \n")
            self.close()

        except RuntimeError:
            print("opae uio module exception")
            return False
        except ValueError:
            print("Invalid arguemnts")
            return False

        return True

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

    def clear_reg_bits(self,
                       region_index,
                       reg_offset,
                       idx, width):
        """
        Read reg offset
        Write 0 to bits
        poll for status
        """
        total_time = 0
        while (True):
            reg_data = self.read32(region_index, reg_offset)
            value = self.register_get_bits(reg_data, idx, width)
            if value == 0:
                return True
            value = self.register_field_set(value,
                                            idx, width, 0)
            self.write32(region_index, reg_offset, value)

            time.sleep(HSSI_POLL_SLEEP_TIME_SEC)
            if total_time > HSSI_POLL_TIMEOUT_SEC:
                return False

            total_time = HSSI_POLL_SLEEP_TIME_SEC + total_time
        return False

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
        while (True):
            reg_data = self.read32(region_index, reg_offset)
            if reg_data == 0:
                return True
            self.write32(region_index, reg_offset, 0x0)

            time.sleep(HSSI_POLL_SLEEP_TIME_SEC)
            if total_time > HSSI_POLL_TIMEOUT_SEC:
                return False

            total_time = HSSI_POLL_SLEEP_TIME_SEC + total_time

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
                                     self.hssi_csr.HSSI_CTL_ADDRESS)
        if ctl_addr_value != 0:
            ret = self.clear_reg(region_index,
                                 self.hssi_csr.HSSI_CTL_ADDRESS)
            if not ret:
                print("Failed to clear HSSI CTL Address csr")
                return False

        cmd_sts_value = self.read32(region_index, self.hssi_csr.HSSI_CTL_STS)
        if cmd_sts_value != 0:
            ret = self.clear_reg_bits(region_index,
                                      self.hssi_csr.HSSI_CTL_STS,
                                      0, 3)
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
        while (True):
            reg_data = self.read32(region_index, reg_offset)
            if ((reg_data >> bit_index) & 1) == 1:
                return True

            time.sleep(HSSI_POLL_SLEEP_TIME_SEC)
            if total_time > HSSI_POLL_TIMEOUT_SEC:
                return False
            total_time = HSSI_POLL_SLEEP_TIME_SEC + total_time

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
            return False, -1

        self.write32(region_index, self.hssi_csr.HSSI_CTL_ADDRESS, reg_data)

        cmd_sts = hssi_cmd_sts(0x1)
        self.write32(region_index, self.hssi_csr.HSSI_CTL_STS, cmd_sts.value)

        if not self.read_poll_timeout(region_index,
                                      self.hssi_csr.HSSI_CTL_STS,
                                      2):
            print("HSSI ctl sts csr fails to update ACK")
            return False, -1

        value = self.read32(region_index, self.hssi_csr.HSSI_RD_DATA)

        ret = self.clear_ctl_sts_reg(region_index)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            return False, -1

        return True, value

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
            return False

        self.write32(region_index, self.hssi_csr.HSSI_CTL_ADDRESS, reg_data)
        self.write32(region_index, self.hssi_csr.HSSI_WR_DATA, value)

        cmd_sts = hssi_cmd_sts(0x2)
        self.write32(region_index, self.hssi_csr.HSSI_CTL_STS, cmd_sts.value)

        if not self.read_poll_timeout(region_index,
                                      self.hssi_csr.HSSI_CTL_STS,
                                      2):
            print("HSSI ctl sts csr fails to update ACK")
            return False

        ret = self.clear_ctl_sts_reg(region_index)
        if not ret:
            print("Failed to clear HSSI CTL STS csr")
            return False

        return True

    def register_field_set(self, reg_data, idx, width, value):
        mask = 0
        for x in range(width):
            mask |= (1 << x)
        value &= mask
        reg_data &= ~(mask << idx)
        reg_data |= (value << idx)
        return reg_data

    def register_field_get(self, reg_data, idx):
        value = ((reg_data >> idx) & (1))
        return value

    def register_get_bits(self, reg_data, idx, width):
        value = 0
        for x in range(width):
            value |= (reg_data & (1 << (idx + x)))
        return value
