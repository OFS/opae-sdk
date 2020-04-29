#! /usr/bin/env python3
# Copyright(c) 2018, Intel Corporation
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
import mmap
import os
import re
import sys
import time

DEVICES_PATH = "/sys/bus/pci/devices"


def chars_to_vals(chars):
    vals = 0
    for char in chars:
        vals = (vals << 8) | ord(char)
    return vals


def vals_to_chars(vals, num):
    chars = ""
    for i in range(num):
        chars = chr((vals >> (8 * i)) & 0xFF) + chars
    return chars


class PhyMemAccess(object):

    def __init__(self):
        self.is_opened = False
        self.fdesc = None
        self.base_addr = None
        self.rdptr = None
        self.wrptr = None

    def open(self, base_addr, psize=mmap.PAGESIZE):
        self.fdesc = os.open("/dev/mem", os.O_RDWR)
        self.base_addr = base_addr
        gpio_addr = base_addr
        page_size = psize
        page_addr = (gpio_addr & (~(page_size - 1)))

        self.rdptr = mmap.mmap(fileno=self.fdesc, length=page_size,
                               flags=mmap.MAP_SHARED,
                               prot=mmap.PROT_READ, offset=page_addr)
        self.wrptr = mmap.mmap(fileno=self.fdesc, length=page_size,
                               flags=mmap.MAP_SHARED,
                               prot=mmap.PROT_WRITE, offset=page_addr)
        self.is_opened = True

    def close(self):
        if not self.is_opened:
            return
        self.is_opened = False
        self.rdptr.close()
        self.wrptr.close()
        os.close(self.fdesc)

    def __getitem__(self, index):
        return self.rdptr[index]

    def __setitem__(self, index, value):
        self.wrptr[index] = value

    def __getslice__(self, first, last):
        return self.rdptr[first:last][::-1]

    def __setslice__(self, first, last, value):
        if len(value) != (last - first):
            raise ValueError('Slice length does not equal actual length')
        self.wrptr[first:last] = value[::-1]

    def write32(self, addr, val):
        self[addr:addr + 4] = vals_to_chars(val, 4)

    def read32(self, addr):
        return chars_to_vals(self[addr:addr + 4])

    def write64(self, addr, val):
        self[addr:addr + 8] = vals_to_chars(val, 8)

    def read64(self, addr):
        return chars_to_vals(self[addr:addr + 8])


def normalize_bdf(bdf):

    pat = r'[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$'
    if re.match(pat, bdf):
        return bdf

    if re.match(r'[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]$', bdf):
        return "0000:{}".format(bdf)

    raise Exception("{} is an invalide bdf".format(bdf))


def get_pcie_bdfs(vendor_id, device_id):
    bdfs = []
    pcie_device_list = [x[1] for x in os.walk(DEVICES_PATH)][0]
    for dev in pcie_device_list:
        devpath = os.path.join(DEVICES_PATH, dev)
        with open(os.path.join(devpath, "vendor")) as fdesc:
            vendor = fdesc.read()
            fdesc.close()

        with open(os.path.join(devpath, "device")) as fdesc:
            device = fdesc.read()
            fdesc.close()

        if (
                (vendor_id == int(vendor.strip(), 16)) and
                (device_id == int(device.strip(), 16))
        ):
            bdfs.append(dev)
    return bdfs


def get_bdf_base_addr(bdf, pci_bar):
    """
        The file, /sys/bus/pci/devices/{bdf}/resource,
        contains info about the BAR mappings in the
        following format:

        0x00000000eab00000 0x00000000eab7ffff 0x000000000014220c
        0x0000000000000000 0x0000000000000000 0x0000000000000000
        0x00000000eaa00000 0x00000000eaafffff 0x000000000014220c
        0x0000000000000000 0x0000000000000000 0x0000000000000000
        0x0000000000000000 0x0000000000000000 0x0000000000000000
        0x0000000000000000 0x0000000000000000 0x0000000000000000
        0x0000000000000000 0x0000000000000000 0x0000000000000000
        0x00000000ea000000 0x00000000ea0fffff 0x000000000014220c
        0x0000000000000000 0x0000000000000000 0x0000000000000000
        0x0000000000000000 0x0000000000000000 0x0000000000000000
        0x0000000000000000 0x0000000000000000 0x0000000000000000
        0x00000000eab80000 0x00000000eab83fff 0x000000000014220c
        0x0000000000000000 0x0000000000000000 0x0000000000000000

        Return the physical address of the requested bar.
    """
    if (pci_bar < 0) or (pci_bar > 5):
        raise Exception("invalid pci_bar {}".format(pci_bar))

    devpath = os.path.join(DEVICES_PATH, bdf, "resource")
    with open(devpath, 'r') as fdesc:
        desc = fdesc.read()
        fdesc.close()

    desc = desc.split('\n')
    return int(desc[pci_bar].split()[0], 16)


class SklHssi(object):
    # DFH Base
    DFH_BASE = 0x6000

    # Nios control and status register offsets from DFH base
    # 64 bit CPU to NIOS control reg with the layout
    # cmd[16] addr[16] data[32]
    HSSI_CTRL_REG_OFFSET = 0x8
    # 64 bit NIOS reply to CPU status reg
    # extra_status [31] ack[1] data[32]
    HSSI_STAT_REG_OFFSET = 0x10

    # Control commands
    HSSI_CTRL_CMD_NO_REQ = 0    # no request
    HSSI_CTRL_CMD_SW_RD = 0x8   # software register read request
    HSSI_CTRL_CMD_SW_WR = 0x10  # software register write request
    HSSI_CTRL_CMD_AUX_RD = 0x40  # aux bus read request
    HSSI_CTRL_CMD_AUX_WR = 0x80  # aux bus write request

    # Nios Aux bus CSR addresses (refer spec)
    HSSI_AUX_LED_R = 0
    HSSI_AUX_PRMGMT_CMD = 4
    HSSI_AUX_PRMGMT_DIN = 5
    HSSI_AUX_PRMGMT_DOUT = 6
    HSSI_AUX_LOCAL_CMD = 7
    HSSI_AUX_LOCAL_DIN = 8
    HSSI_AUX_LOCAL_DOUT = 9

    # Nios soft commands
    NIOS_NOP = 0x0
    NIOS_I2C_READ = 0x1
    NIOS_I2C_WRITE = 0x2
    NIOS_I2C_WRITE2 = 0x3
    NIOS_I2C_WRITE_VAL = 0x4
    NIOS_PLLT_RECO = 0x5
    NIOS_PLLR_RECO = 0x6
    NIOS_SERDES_RECO = 0x7
    NIOS_CHANGE_HSSI_MODE = 0x8
    NIOS_TX_EQ_WRITE = 0x9
    NIOS_TX_EQ_READ = 0xa
    NIOS_HSSI_INIT = 0xb
    NIOS_HSSI_INIT_DONE = 0xc
    NIOS_HSSI_FATL_ERR = 0xd
    NIOS_SET_HSSI_EN = 0xe
    NIOS_GET_HSSI_EN = 0xf
    NIOS_GET_HSSI_MODE = 0x10
    NIOS_GET_GBS_SERVICE_ENA = 0x11
    NIOS_FW_VERSION = 0xFF

    # Nios local bus CSRs
    NIOS_LBUS_TEMP_SENSE = 0x0
    NIOS_LBUS_PLL_RST_CTRL = 0x1
    NIOS_LBUS_PLL_LOCKED_STATUS = 0x2
    NIOS_LBUS_PRMGMT_RAME_ENA = 0x3
    NIOS_LBUS_CLK_MON_SEL = 0x4
    NIOS_LBUS_CLK_MON_OUT = 0x5
    NIOS_LBUS_RSVD_1 = 0x6
    NIOS_LBUS_RSVD_2 = 0x7
    NIOS_LBUS_RECONFIG_CMD_ADDR = 0x8
    NIOS_LBUS_RECONFIG_WR_DATA = 0x9
    NIOS_LBUS_RECONFIG_RD_DATA = 0xa
    NIOS_LBUS_PLL_CMD_ADDR = 0xb
    NIOS_LBUS_PLL_WR = 0xc
    NIOS_LBUS_PLL_T_RD = 0xd
    NIOS_LBUS_PLL_R_RD = 0xe
    NIOS_LBUS_RSVD_3 = 0xf

    # PR management interface commands
    PR_MGMT_SCRATCH = 0x0
    PR_MGMT_RST = 0x1  # csr_rst, rx_rst, tx_rst
    # status_read (1bit), status_write (1bit), status_addr (16 bit)
    PR_MGMT_STATUS = 0x2
    PR_MGMT_STATUS_WR_DATA = 0x3
    PR_MGMT_STATUS_RD_DATA = 0x4
    PR_MGMT_PORT_SEL = 0x5
    PR_MGMT_SLOOP = 0x6  # serial loopback
    # hssi.f2a_rx_enh_blk_lock, hssi.f2a_rx_is_lockedtodata
    PR_MGMT_LOCK_STATUS = 0x7
    PR_MGMT_I2C_SEL_WDATA = 0x8  # i2c_inst_sel_r,i2c_ctrl_wdata_r
    PR_MGMT_I2C_SEL_RDATA = 0x9  # i2c_stat_rdata
    PR_MGMT_ETH_CTRL = 0xa  # applies only to E40
    PR_MGMT_ETH_WR_DATA = 0xb  # applies only to E40
    PR_MGMT_ETH_RD_DATA = 0xc  # applies only to E40
    # hssi.a2f_prmgmt_fatal_err, hssi.f2a_init_done
    PR_MGMT_ERR_INIT_DONE = 0xd

    # Low-latency 40G IP Core registers
    # refer https://www.altera.com/content/dam/altera-www/
    # global/en_US/pdfs/literature/ug/ug_ll_40_100gbe.pdf
    PHY_REVID = 0x300
    PHY_SCRATCH = 0x301
    PHY_NAME_0 = 0x302
    PHY_NAME_1 = 0x303
    PHY_NAME_2 = 0x304
    PHY_CONFIG = 0x310
    PHY_PMA_SLOOP = 0x313
    PHY_PCS_INDIRECT_ADDR = 0x314
    PHY_PCS_INDIRECT_DATA = 0x315
    PHY_TX_PLL_LOCKED = 0x320
    PHY_EIOFREQ_LOCKED = 0x321
    PHY_TX_COREPLL_LOCKED = 0x322
    PHY_FRAME_ERROR = 0x323
    PHY_SCLR_FRAME_ERROR = 0x324
    PHY_EIO_SFTRESET = 0x325
    PHY_RXPCS_STATUS = 0x326
    PHY_REFCLK_KHZ = 0x340
    PHY_RXCLK_KHZ = 0x341
    PHY_TXCLK_KHZ = 0x342
    PHY_RECCLK_KHZ = 0x343
    PHY_TXIOCLK_KHZ = 0x344

    # TX MAC Configuration registers
    TXMAC_REVID = 0x400
    TXMAC_SCRATCH = 0x401
    TXMAC_NAME_0 = 0x402
    TXMAC_NAME_1 = 0x403
    TXMAC_NAME_2 = 0x404
    LINK_FAULT_CONFIG = 0x405
    IPG_COL_REM = 0x406
    MAX_TX_SIZE_CONFIG = 0x407
    CRC_CONFIG = 0x408
    ADDR_CONFIG = 0x409
    PLE_CONFIG = 0x40a
    ERR_CONFIG = 0x4f0

    # Transmit side statistics registers
    CNTR_TX_64B_LO = 0x816
    CNTR_TX_64B_HI = 0x817
    CNTR_TX_ST_LO = 0x836
    CNTR_TX_ST_HI = 0x837
    TXSTAT_NAME_0 = 0x842
    TXSTAT_NAME_1 = 0x843
    TXSTAT_NAME_2 = 0x844
    CNTR_TX_CONFIG = 0x845

    # RX MAC Configuration registers
    RXMAC_REVID = 0x500
    RXMAC_SCRATCH = 0x501
    RXMAC_NAME_0 = 0x502
    RXMAC_NAME_1 = 0x503
    RXMAC_NAME_2 = 0x504
    RXMAC_TYPE = 0x505
    MAX_RX_SIZE_CONFIG = 0x506
    MAC_CRC_CONFIG = 0x507
    RX_FAULT_CONFIG = 0x508
    RX_AFL = 0x509
    CFG_PLEN_CHECK = 0x50a
    RX_ERR = 0x5f0

    # Receive side statistics registers
    CNTR_RX_CRCERR_LO = 0x906
    CNTR_RX_CRCERR_HI = 0x907
    CNTR_RX_RUNT_LO = 0x934
    CNTR_RX_RUNT_HI = 0x935
    CNTR_RX_ST_LO = 0x936
    CNTR_RX_ST_HI = 0x937
    RXSTAT_NAME_0 = 0x942
    RXSTAT_NAME_1 = 0x943
    RXSTAT_NAME_2 = 0x944
    CNTR_RX_CONFIG = 0x945

    # AFU IDs
    AFU_OFFSET_HI = 0x40010
    AFU_OFFSET_LO = 0x40008

    # AFU Registers
    ETH_STAT_ADDR = 0x40028
    ETH_CTRL_ADDR = 0x40030
    ETH_WR_DATA = 0x40038
    ETH_RD_DATA = 0x40040

    AFU_ID_E10 = 0x05189FE40676DD24B74F291AF34E1783
    AFU_ID_E40 = 0x26B40788034B4389B3C151A1B62ED6C2

    # Miscallaenous
    CLK_RD_DELAY = 0.25
    NUM_E10_CHANNELS = 4
    NUM_SEND_PACKETS = 1000000

    def __init__(self, mem, dfhaddr=DFH_BASE):
        self._mem0 = mem
        self._mem2 = None
        self._dfhaddr = dfhaddr
        self._hssictrladdr = self._dfhaddr + self.HSSI_CTRL_REG_OFFSET
        self._hssistataddr = self._dfhaddr + self.HSSI_STAT_REG_OFFSET

    def _wait_for_ack(self):
        val = self._mem0.read32(self._hssistataddr + 0x4) & 1
        while val == 0:
            val = self._mem0.read32(self._hssistataddr + 0x4) & 1

    def _wait_for_clr(self):
        val = self._mem0.read32(self._hssistataddr + 0x4) & 1
        while val:
            val = self._mem0.read32(self._hssistataddr + 0x4) & 1

    def skl_set_mem2(self, mem2):
        self._mem2 = mem2

    def skl_read(self, addr):
        self._mem0.write64(self._hssictrladdr,
                           ((self.HSSI_CTRL_CMD_SW_RD << 16) + addr) << 32)
        self._wait_for_ack()
        ret = self._mem0.read32(self._hssistataddr)
        self._mem0.write64(self._hssictrladdr, self.HSSI_CTRL_CMD_NO_REQ)
        self._wait_for_clr()
        return ret

    def skl_write(self, addr, data):
        self._mem0.write64(self._hssictrladdr,
                           (((self.HSSI_CTRL_CMD_SW_WR << 16) + addr) << 32) |
                           data)
        self._wait_for_ack()
        self._mem0.write64(self._hssictrladdr, self.HSSI_CTRL_CMD_NO_REQ)
        self._wait_for_clr()

    def skl_aux_read(self, addr):
        self._mem0.write64(self._hssictrladdr,
                           ((self.HSSI_CTRL_CMD_AUX_RD << 16) + addr) << 32)
        self._wait_for_ack()
        ret = self._mem0.read32(self._hssistataddr)
        self._mem0.write64(self._hssictrladdr, self.HSSI_CTRL_CMD_NO_REQ)
        self._wait_for_clr()
        return ret

    def skl_aux_write(self, addr, data):
        self._mem0.write64(self._hssictrladdr,
                           (((self.HSSI_CTRL_CMD_AUX_WR << 16) + addr) << 32) |
                           data)
        self._wait_for_ack()
        self._mem0.write64(self._hssictrladdr, self.HSSI_CTRL_CMD_NO_REQ)
        self._wait_for_clr()

    def skl_maci2c_start(self):
        beat = 0x2 | 0x1
        self.skl_aux_write(0x11, beat)
        beat = 0x2 | 0x0
        self.skl_aux_write(0x11, beat)
        beat = 0x0 | 0x0
        self.skl_aux_write(0x11, beat)

    def skl_maci2c_stop(self):
        beat = 0x2 | 0x0
        self.skl_aux_write(0x11, beat)
        beat = 0x2 | 0x1
        self.skl_aux_write(0x11, beat)
        beat = 0x0 | 0x1
        self.skl_aux_write(0x11, beat)

    def skl_maci2c_wrbeat(self, SDA):
        beat = (0x0 << 1) | SDA
        self.skl_aux_write(0x11, beat)
        beat = (0x1 << 1) | SDA
        self.skl_aux_write(0x11, beat)
        beat = (0x0 << 1) | SDA
        self.skl_aux_write(0x11, beat)

    def skl_maci2c_rdbeat(self):
        beat = (0x0 << 1) | 0x1
        self.skl_aux_write(0x11, beat)
        beat = (0x1 << 1) | 0x1
        self.skl_aux_write(0x11, beat)
        ret = self.skl_aux_read(0x11) & 0x1
        beat = (0x0 << 1) | 0x1
        self.skl_aux_write(0x11, beat)
        return ret

    def skl_maci2c_wrbyte(self, byte):
        for i in range(7, -1, -1):
            self.skl_maci2c_wrbeat((byte >> i) & 0x1)

    def skl_maci2c_rdbyte(self):
        ret = 0
        for i in range(7, -1, -1):
            ret = (ret << 1) | self.skl_maci2c_rdbeat()
        return ret

    def skl_maci2c_reset(self):
        self.skl_maci2c_start()
        for i in range(9):
            self.skl_maci2c_rdbeat()
        self.skl_maci2c_start()
        self.skl_maci2c_stop()

    def skl_maci2c_read_serial(self, addr, num=1):
        self.skl_maci2c_start()
        self.skl_maci2c_wrbyte(int('10110000', 2))
        if self.skl_maci2c_rdbeat() != 0:
            raise Exception("Error: I2C ACK bit not clear! (0)")

        self.skl_maci2c_wrbyte(0x80 | addr)
        if self.skl_maci2c_rdbeat() != 0:
            raise Exception("Error: I2C ACK bit not clear! (1)")

        self.skl_maci2c_stop()
        self.skl_maci2c_start()
        self.skl_maci2c_wrbyte(int('10110001', 2))
        if self.skl_maci2c_rdbeat() != 0:
            raise Exception("Error: I2C ACK bit not clear! (2)")

        ret = 0
        for i in range(num):
            ret = (ret << 8) | self.skl_maci2c_rdbyte()
            if i == (num - 1):
                self.skl_maci2c_stop()
            else:
                self.skl_maci2c_wrbeat(0)  # ACK
        return ret

    def skl_maci2c_read_eeprom(self, addr, num=1):
        self.skl_maci2c_start()
        self.skl_maci2c_wrbyte(int('10100000', 2))
        if self.skl_maci2c_rdbeat() != 0:
            raise Exception("Error: I2C ACK bit not clear! (0)")

        self.skl_maci2c_wrbyte(addr)
        if self.skl_maci2c_rdbeat() != 0:
            raise Exception("Error: I2C ACK bit not clear! (1)")

        self.skl_maci2c_stop()
        self.skl_maci2c_start()
        self.skl_maci2c_wrbyte(int('10100001', 2))
        if self.skl_maci2c_rdbeat() != 0:
            raise Exception("Error: I2C ACK bit not clear! (2)")

        ret = 0
        for i in range(num):
            ret = (ret << 8) | self.skl_maci2c_rdbyte()
            if i == (num - 1):
                self.skl_maci2c_stop()
            else:
                self.skl_maci2c_wrbeat(0)  # ACK
        return ret

    def skl_local_read(self, addr):
        self.skl_aux_write(self.HSSI_AUX_LOCAL_CMD, addr)
        return self.skl_aux_read(self.HSSI_AUX_LOCAL_DOUT)

    def skl_local_write(self, addr, din):
        self.skl_aux_write(self.HSSI_AUX_LOCAL_DIN, din)
        self.skl_aux_write(self.HSSI_AUX_LOCAL_CMD, 0x10000 | addr)
        self.skl_aux_write(self.HSSI_AUX_LOCAL_CMD, 0x0)

    def skl_prmgmt_read(self, addr):
        if self._mem2:
            self._mem2.write64(self.ETH_CTRL_ADDR, 0x20000 | addr)
            rval = self._mem2.read64(self.ETH_RD_DATA)
            self._mem2.write64(self.ETH_CTRL_ADDR, 0x0)
            return rval
        else:
            self.skl_aux_write(self.HSSI_AUX_PRMGMT_CMD, addr)
            rval = self.skl_aux_read(self.HSSI_AUX_PRMGMT_DOUT)
            self.skl_aux_write(self.HSSI_AUX_PRMGMT_CMD, 0)
            return rval

    def skl_prmgmt_write(self, addr, din):
        if self._mem2:
            self._mem2.write64(self.ETH_WR_DATA, din)
            self._mem2.write64(self.ETH_CTRL_ADDR, 0x10000 | addr)
            self._mem2.write64(self.ETH_CTRL_ADDR, 0x0)
        else:
            self.skl_aux_write(self.HSSI_AUX_PRMGMT_DIN, din)
            self.skl_aux_write(self.HSSI_AUX_PRMGMT_CMD, 0x10000 | addr)
            self.skl_aux_write(self.HSSI_AUX_PRMGMT_CMD, 0x0)

    def skl_e10_read(self, addr):
        self.skl_prmgmt_write(self.PR_MGMT_STATUS, 0x20000 | addr)
        return self.skl_prmgmt_read(self.PR_MGMT_STATUS_RD_DATA)

    def skl_e10_write(self, addr, din):
        self.skl_prmgmt_write(self.PR_MGMT_STATUS_WR_DATA, din)
        self.skl_prmgmt_write(self.PR_MGMT_STATUS, 0x10000 | addr)

    def skl_e10_check(self):
        afu_guid = (self._mem2.read64(self.AFU_OFFSET_HI) <<
                    64) | self._mem2.read64(self.AFU_OFFSET_LO)
        if afu_guid != self.AFU_ID_E10:
            raise Exception("Unexpected AFU_ID, please load the E2E_E10 AFU")

    def nios_soft_fn(self, cmd, arg0=0, arg1=0, arg2=0, arg3=0):
        self.skl_write(2, arg0)
        self.skl_write(3, arg1)
        self.skl_write(4, arg2)
        self.skl_write(5, arg3)
        self.skl_write(1, cmd)
        return self.skl_read(6)

    def skl_stat(self):
        clock_name = ["spare   ", "ref     ", "TX out  ",
                      "RX rec  ", "TX com  ", "RX com  ",
                      "TX com2 ", "RX com2 "]
        print("Firmware version %08x"
              % self.nios_soft_fn(self.NIOS_FW_VERSION))
        val = self.skl_prmgmt_read(self.PR_MGMT_SCRATCH)
        print("GBS %c%c%c version %02x" % (
            (val >> 24) & 0xff, (val >> 16)
            & 0xff, (val >> 8) & 0xff, val & 0xff))
        val = self.skl_local_read(self.NIOS_LBUS_TEMP_SENSE)
        print("Temp sense : %d (C) %d (F)" % (val >> 8, val & 0xff))
        print("")
        val = self.nios_soft_fn(self.NIOS_GET_HSSI_MODE)
        modes = {
            0: "LSFR Mode",
            1: "10GbE Mode",
            2: "40GbE Mode",
        }
        mode = modes.get(val, 'Unknown')

        print("HSSI Mode: %d (%s)" % (val, mode))
        print("HSSI Enabled: %d" % self.nios_soft_fn(self.NIOS_GET_HSSI_EN))
        print("init_done: %d" % self.nios_soft_fn(self.NIOS_HSSI_INIT_DONE))
        print("")
        print("NIOS error: %08x" % self.nios_soft_fn(self.NIOS_HSSI_FATL_ERR))
        print("")
        val = self.skl_local_read(self.NIOS_LBUS_PLL_LOCKED_STATUS)
        print("PLL lock Rpre      : %x" % ((val >> 2) & 1))
        print("PLL lock core TX   : %x" % ((val >> 3) & 1))
        print("PLL lock core RX   : %x" % ((val >> 1) & 1))
        print("PLL lock TX   ATX  : %x" % ((val >> 0) & 1))
        print("")
        val = self.skl_local_read(self.NIOS_LBUS_PRMGMT_RAME_ENA)
        print("RAM ena : %x" % val)
        print("")
        i = 0
        for name in clock_name:
            self.skl_local_write(self.NIOS_LBUS_CLK_MON_SEL, 7 - i)
            self.skl_local_read(self.NIOS_LBUS_CLK_MON_SEL)
            time.sleep(self.CLK_RD_DELAY)
            val = self.skl_local_read(self.NIOS_LBUS_CLK_MON_OUT)
            val *= 10
            print("%s %6d KHz" % (name, val))
            i = i + 1

    def set_mode(self, mode):
        if mode < 0 or mode > 2:
            return

        self.skl_local_write(self.NIOS_LBUS_PRMGMT_RAME_ENA, 0x2)

        modes = {
            0: "Setting the SKL nios into mode 0 (LFSR Mode)",
            1: "Setting the SKL nios into mode 1 (10GbE Mode)",
            2: "Setting the SKL nios into mode 2 (40GbE Mode)",
        }
        print(modes.get(mode))
        self.nios_soft_fn(self.NIOS_CHANGE_HSSI_MODE, mode)
        self.nios_soft_fn(self.NIOS_HSSI_INIT)
        self.skl_local_write(self.NIOS_LBUS_PRMGMT_RAME_ENA, 0x0)
        self.skl_local_write(self.NIOS_LBUS_PRMGMT_RAME_ENA, 0x1)

    def e10_tx_stat(self):
        base = 0x1c00
        val = 0
        val2 = 0
        print("TX side statistics -- ")
        val = self.skl_e10_read(base + 2)
        val2 = self.skl_e10_read(base + 3)
        print("Frames OK: %d" % ((val2 << 32) | val))
        val = self.skl_e10_read(base + 4)
        val2 = self.skl_e10_read(base + 5)
        print("Frames Err: %d" % ((val2 << 32) | val))
        val = self.skl_e10_read(base + 6)
        val2 = self.skl_e10_read(base + 7)
        print("Frames CRC: %d" % ((val2 << 32) | val))
        val = self.skl_e10_read(base + 8)
        val2 = self.skl_e10_read(base + 9)
        print("Bytes OK: %d" % ((val2 << 32) | val))
        print("")

    def e10_rx_stat(self):
        base = 0xc00
        val = 0
        val2 = 0
        print("RX side statistics -- ")
        val = self.skl_e10_read(base + 2)
        val2 = self.skl_e10_read(base + 3)
        print("Frames OK: %d" % ((val2 << 32) | val))
        val = self.skl_e10_read(base + 4)
        val2 = self.skl_e10_read(base + 5)
        print("Frames Err: %d" % ((val2 << 32) | val))
        val = self.skl_e10_read(base + 6)
        val2 = self.skl_e10_read(base + 7)
        print("Frames CRC: %d" % ((val2 << 32) | val))
        val = self.skl_e10_read(base + 8)
        val2 = self.skl_e10_read(base + 9)
        print("Bytes OK: %d" % ((val2 << 32) | val))
        print("")

    def e10_stat(self):
        val = self.skl_prmgmt_read(self.PR_MGMT_SLOOP)
        print("TxRx loop : %x" % val)
        val = self.skl_prmgmt_read(self.PR_MGMT_LOCK_STATUS)
        print("Freq lock : %x" % ((int(val) >> 4) & 0xf))
        print("Word lock : %x" % (int(val) & 0xf))
        print("")
        for i in range(self.NUM_E10_CHANNELS):
            print("*** 10GE port %d\n" % i)
            self.skl_prmgmt_write(self.PR_MGMT_PORT_SEL, i)
            self.e10_tx_stat()
            self.e10_rx_stat()
        print("")

    def skl_e40_check(self):
        afu_guid = (self._mem2.read64(self.AFU_OFFSET_HI) <<
                    64) | self._mem2.read64(self.AFU_OFFSET_LO)
        if afu_guid != self.AFU_ID_E40:
            raise Exception("Unexpected AFU_ID, please load the E2E_E40 AFU")

    def skl_e40_read(self, addr):
        self.skl_prmgmt_write(self.PR_MGMT_STATUS, 0x20000 | addr)
        return self.skl_prmgmt_read(self.PR_MGMT_STATUS_RD_DATA)

    def skl_e40_write(self, addr, din):
        self.skl_prmgmt_write(self.PR_MGMT_STATUS_WR_DATA, din)
        self.skl_prmgmt_write(self.PR_MGMT_STATUS, 0x10000 | addr)

    def skl_e40_traf_read(self, addr):
        self.skl_prmgmt_write(self.PR_MGMT_ETH_CTRL, 0x20000 | addr)
        return self.skl_prmgmt_read(self.PR_MGMT_ETH_RD_DATA)

    def skl_e40_traf_write(self, addr, din):
        self.skl_prmgmt_write(self.PR_MGMT_ETH_WR_DATA, din)
        self.skl_prmgmt_write(self.PR_MGMT_ETH_CTRL, 0x10000 | addr)

    def skl_e40_stat_clr(self):
        print("clearing e40 stats...")
        self.skl_e40_write(self.CNTR_RX_CONFIG, 1)
        self.skl_e40_write(self.CNTR_TX_CONFIG, 1)

    def skl_e40_stat(self):
        print("************************")
        print("** A10 40GbE port 0")
        print("************************")
        print("")
        print("PCS status section")
        print("  rev    : %08x" % self.skl_e40_read(self.PHY_REVID))
        print("  scratch: %08x" % self.skl_e40_read(self.PHY_SCRATCH))
        print("  name   : %s%s%s" % (
            vals_to_chars(self.skl_e40_read(self.PHY_NAME_0), 4),
            vals_to_chars(self.skl_e40_read(self.PHY_NAME_1), 4),
            vals_to_chars(self.skl_e40_read(self.PHY_NAME_2), 4)))
        print("  ctrl ovr: %x" % self.skl_e40_read(self.PHY_CONFIG))
        print("  FIFO flags ")
        for k in range(8):
            self.skl_e40_write(self.PHY_PCS_INDIRECT_ADDR, k)
            val = self.skl_e40_read(self.PHY_PCS_INDIRECT_DATA)
            val = self.skl_e40_read(self.PHY_PCS_INDIRECT_DATA)
            sys.stdout.write("    %d : %x " % (k, val))
            if k & 4:
                sys.stdout.write("rx")
            else:
                sys.stdout.write("tx")
            if k & 2:
                sys.stdout.write("p")
            if k & 1:
                sys.stdout.write("empty")
            else:
                sys.stdout.write("full")
            print("")
        print("  Loop      : %x" % self.skl_e40_read(self.PHY_PMA_SLOOP))
        print("  TX lock   : %x" % self.skl_e40_read(self.PHY_TX_PLL_LOCKED))
        val = self.skl_e40_read(self.PHY_EIOFREQ_LOCKED)
        if val == 0xf:
            lstat = "fully locked"
        else:
            lstat = "incomplete"
        print("  Freqlock  : %x (%s)" % (val, lstat))
        print("  mac PLL flg: %x" % self.skl_e40_read(
            self.PHY_TX_COREPLL_LOCKED))

        val = self.skl_e40_read(self.PHY_FRAME_ERROR)
        output = ""
        if val != 0:
            output = "  (clearing)"
            self.skl_e40_write(self.PHY_SCLR_FRAME_ERROR, 1)
            self.skl_e40_write(self.PHY_SCLR_FRAME_ERROR, 0)

        print("  Frame Err : %x%s" % (val, output))
        print("  rx purge: %x " % (self.skl_e40_read(self.PHY_EIO_SFTRESET)))
        print("")
        val = self.skl_e40_read(self.PHY_RXPCS_STATUS)
        if val & 1:
            print("RX status : fully aligned")
        else:
            print("RX status : not aligned")
        print("")
        print("Clk ref   : %6d KHz" % (self.skl_e40_read(self.PHY_REFCLK_KHZ)))
        print("Clk RX    : %6d KHz" % (self.skl_e40_read(self.PHY_RXCLK_KHZ)))
        print("Clk TX    : %6d KHz" % (self.skl_e40_read(self.PHY_TXCLK_KHZ)))
        print("Clk recov : %6d KHz" % (self.skl_e40_read(self.PHY_RECCLK_KHZ)))
        print("Clk TX IO : %6d KHz"
             % (self.skl_e40_read(self.PHY_TXIOCLK_KHZ)))
        print("")
        print("TXCSR section")
        print("  rev    : %08x" % (self.skl_e40_read(self.TXMAC_REVID)))
        print("  scratch: %08x" % (self.skl_e40_read(self.TXMAC_SCRATCH)))
        print("  name   : %s%s%s" % (
            vals_to_chars(self.skl_e40_read(self.TXMAC_NAME_0), 4),
            vals_to_chars(self.skl_e40_read(self.TXMAC_NAME_1), 4),
            vals_to_chars(self.skl_e40_read(self.TXMAC_NAME_2), 4)))

        print("  fault  : %08x" % (self.skl_e40_read(self.LINK_FAULT_CONFIG)))
        print("  ipgcol : %08x" % (self.skl_e40_read(self.IPG_COL_REM)))
        print("  frmsize: %08x"
            % (self.skl_e40_read(self.MAX_TX_SIZE_CONFIG)))
        print("  crc cfg: %08x" % (self.skl_e40_read(self.CRC_CONFIG)))
        print(" addr cfg: %08x" % (self.skl_e40_read(self.ADDR_CONFIG)))
        print(" ple  cfg: %08x" % (self.skl_e40_read(self.PLE_CONFIG)))
        print(" err  cfg: %08x" % (self.skl_e40_read(self.ERR_CONFIG)))
        print("")
        print("TXstat section")

        print("%s%s%s" % (
            vals_to_chars(self.skl_e40_read(self.TXSTAT_NAME_0), 4),
            vals_to_chars(self.skl_e40_read(self.TXSTAT_NAME_1), 4),
            vals_to_chars(self.skl_e40_read(self.TXSTAT_NAME_2), 4)))
        self.skl_e40_write(self.CNTR_TX_CONFIG, 4)  # shadow on
        sys.stdout.write("TX packets : ")
        val = self.skl_e40_read(self.CNTR_TX_ST_HI)
        val = self.skl_e40_read(self.CNTR_TX_ST_HI)
        i = val
        val = self.skl_e40_read(self.CNTR_TX_ST_LO)
        val = self.skl_e40_read(self.CNTR_TX_ST_LO)
        i = (i << 32) | val
        print("%d" % i)
        sys.stdout.write("TX 64b     : ")
        val = self.skl_e40_read(self.CNTR_TX_64B_HI)
        val = self.skl_e40_read(self.CNTR_TX_64B_HI)
        i = val
        val = self.skl_e40_read(self.CNTR_TX_64B_LO)
        val = self.skl_e40_read(self.CNTR_TX_64B_LO)
        i = (i << 32) | val
        print("%d" % i)
        self.skl_e40_write(self.CNTR_TX_CONFIG, 0)  # shadow off
        print("")
        print("RXCSR section")
        print("  rev    : %08x" % (self.skl_e40_read(self.RXMAC_REVID)))
        print("  scratch: %08x" % (self.skl_e40_read(self.RXMAC_SCRATCH)))
        print("  name   : %s%s%s" % (
            vals_to_chars(self.skl_e40_read(self.RXMAC_NAME_0), 4),
            vals_to_chars(self.skl_e40_read(self.RXMAC_NAME_1), 4),
            vals_to_chars(self.skl_e40_read(self.RXMAC_NAME_2), 4)))
        print("  type   : %08x" % (self.skl_e40_read(self.RXMAC_TYPE)))
        print("  frm sz : %08x" % (self.skl_e40_read(self.MAX_RX_SIZE_CONFIG)))
        print("  crc    : %08x" % (self.skl_e40_read(self.MAC_CRC_CONFIG)))
        print("  fault  : %08x" % (self.skl_e40_read(self.RX_FAULT_CONFIG)))
        print("  afl    : %08x" % (self.skl_e40_read(self.RX_AFL)))
        print("  pfe    : %08x" % (self.skl_e40_read(self.CFG_PLEN_CHECK)))
        print("  err    : %08x" % (self.skl_e40_read(self.RX_ERR)))
        print("\nRXstat section")
        print("%s%s%s" % (
            vals_to_chars(self.skl_e40_read(self.RXSTAT_NAME_0), 4),
            vals_to_chars(self.skl_e40_read(self.RXSTAT_NAME_1), 4),
            vals_to_chars(self.skl_e40_read(self.RXSTAT_NAME_2), 4)))
        self.skl_e40_write(self.CNTR_RX_CONFIG, 4)  # shadow on
        sys.stdout.write("RX packets : ")
        val = self.skl_e40_read(self.CNTR_RX_ST_HI)
        val = self.skl_e40_read(self.CNTR_RX_ST_HI)
        i = val
        val = self.skl_e40_read(self.CNTR_RX_ST_LO)
        val = self.skl_e40_read(self.CNTR_RX_ST_LO)
        i = (i << 32) | val
        print("%d" % i)
        sys.stdout.write("RX runts   : ")
        val = self.skl_e40_read(self.CNTR_RX_RUNT_HI)
        val = self.skl_e40_read(self.CNTR_RX_RUNT_HI)
        i = val
        val = self.skl_e40_read(self.CNTR_RX_RUNT_LO)
        val = self.skl_e40_read(self.CNTR_RX_RUNT_LO)
        i = (i << 32) | val
        print("%d" % i)
        sys.stdout.write("RX CRC err : ")
        val = self.skl_e40_read(self.CNTR_RX_CRCERR_HI)
        val = self.skl_e40_read(self.CNTR_RX_CRCERR_HI)
        i = val
        val = self.skl_e40_read(self.CNTR_RX_CRCERR_LO)
        val = self.skl_e40_read(self.CNTR_RX_CRCERR_LO)
        i = (i << 32) | val
        print("%d" % i)
        self.skl_e40_write(self.CNTR_RX_CONFIG, 0)  # shadow off
        print("")


def stat_fxn(args, skl):
    skl.skl_stat()


def eeprom_fxn(args, skl):
    skl.skl_maci2c_reset()
    val = 0
    val = skl.skl_maci2c_read_serial(0, 16)
    print("")
    print("128-bit Unique Board ID: 0x%032x" % (val))
    print("")
    print("EEPROM:")
    numbytes = 512
    val = skl.skl_maci2c_read_eeprom(0, numbytes)
    tmp = [0x0] * 16
    for i in range(numbytes):
        dat0 = int(((val >> ((numbytes - 1 - i) * 8))) & 0xFF)
        tmp[i % 16] = dat0
        if (i % 16) == 0:
            sys.stdout.write("(0x%04X) " % i)
        sys.stdout.write("%02X " % dat0)
        if (i % 16) == 15:
            for j in range(16):
                if (tmp[j] >= 32) and (tmp[j] <= 126):
                    sys.stdout.write(chr(tmp[j]))
                else:
                    sys.stdout.write('.')
            sys.stdout.write("\n")
    print("")


PARAMS_DICT = {
    0: "CTLE",
    1: "VGA",
    2: "DCGAIN",
    3: "1st POST",
    4: "2nd POST",
    5: "1st PRE",
    6: "2nd PRE",
    7: "VOD"
}


def eqwrite_fxn(args, skl):
    chan = int(args.channel, 16)
    param = int(args.parameter, 16)
    value = int(args.value, 16)
    val = skl.nios_soft_fn(skl.NIOS_TX_EQ_WRITE, chan, param, value)

    msg = PARAMS_DICT.get(param, "Unknown Paramter")
    msg = "WRITE Channel %i - %s" % (chan, msg)
    print("eqwrite(C=0x%01X,P=0x%02X) <- 0x%02X (%s)" % (
        chan, param, value, msg))


def eqread_fxn(args, skl):
    chan = int(args.channel, 16)
    param = int(args.parameter, 16)
    val = skl.nios_soft_fn(skl.NIOS_TX_EQ_READ, chan, param)

    msg = PARAMS_DICT.get(param, "Unknown Paramter")
    msg = "READ Channel %i - %s" % (chan, msg)
    print("eqread(C=0x%01X,P=0x%02X) -> 0x%02X (%s)" % (chan, param, val, msg))


def e10_loop_fxn(args, skl):
    skl.skl_e10_check()
    if args.flag == "off":
        skl.skl_prmgmt_write(skl.PR_MGMT_SLOOP, 0x0)
    else:
        skl.skl_prmgmt_write(skl.PR_MGMT_SLOOP, 0xf)


def e10_reset_fxn(args, skl):
    skl.skl_e10_check()
    if args.flag == "off":
        skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x6)
        skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x4)
        skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x0)
    else:
        skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x7)


def e10_pkt_send_fxn(args, skl):
    skl.skl_e10_check()
    for i in range(skl.NUM_E10_CHANNELS):
        print("*** 10GE port %d sending" % i)
        skl.skl_prmgmt_write(skl.PR_MGMT_PORT_SEL, i)
        skl.skl_e10_write(0x3c00, skl.NUM_SEND_PACKETS)  # num pkts
        skl.skl_e10_write(0x3c03, 1)          # go


def e10_init_fxn(args, skl):
    skl.skl_e10_check()
    mem2 = skl._mem2
    skl.skl_set_mem2(None)
    print("Changing SKL mode to 10g, \
    initializing E2E_10G AFU and placing it in loopback...")
    skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x7)
    skl.set_mode(1)
    skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x6)
    skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x4)
    skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x0)
    skl.skl_prmgmt_write(skl.PR_MGMT_SLOOP, 0xF)
    skl.skl_set_mem2(mem2)
    e10_stat_clr_fxn(args, skl)
    print("Done!")


def e10_stat_fxn(args, skl):
    skl.skl_e10_check()
    skl.e10_stat()


def e10_stat_clr_fxn(args, skl):
    skl.skl_e10_check()
    for i in range(skl.NUM_E10_CHANNELS):
        print("*** 10GE port %d clearing" % i)
        skl.skl_prmgmt_write(skl.PR_MGMT_PORT_SEL, i)
        skl.skl_e10_write(0xc00, 1)
        skl.skl_e10_write(0x1c00, 1)


def e40_loop_fxn(args, skl):
    skl.skl_e40_check()
    if args.flag == "off":
        skl.skl_e40_write(skl.PHY_PMA_SLOOP, 0x0)
    else:
        skl.skl_e40_write(skl.PHY_PMA_SLOOP, 0x3ff)


def e40_reset_fxn(args, skl):
    skl.skl_e40_check()
    if args.flag == "off":
        skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x0)
    else:
        skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x3)


def e40_pkt_send_fxn(args, skl):
    skl.skl_e40_check()
    print("Sending {} 1500-byte packets...".format(skl.NUM_SEND_PACKETS))
    skl.skl_e40_traf_write(0x4, skl.NUM_SEND_PACKETS)  # number of packets
    skl.skl_e40_traf_write(0x5, 1500)  # packet length
    skl.skl_e40_traf_write(0x6, 0)  # packet delay
    skl.skl_e40_traf_write(0x7, 0x1)  # start
    skl.skl_e40_traf_write(0x7, 0x0)  # start


def e40_init_fxn(args, skl):
    skl.skl_e40_check()
    mem2 = skl._mem2
    skl.skl_set_mem2(None)
    print("Changing SKL mode to 40g, \
    initializing E2E_40G AFU and placing it in loopback...")
    skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x3)
    skl.set_mode(2)
    skl.skl_prmgmt_write(skl.PR_MGMT_RST, 0x0)
    skl.skl_e40_write(0x313, 0x3ff)
    skl.skl_set_mem2(mem2)
    skl.skl_e40_stat_clr()
    print("Done!")


def e40_stat_fxn(args, skl):
    skl.skl_e40_check()
    skl.skl_e40_stat()


def e40_stat_clr_fxn(args, skl):
    skl.skl_e40_check()
    skl.skl_e40_stat_clr()


def parse_args():
    eqw = 'Write value to an equalization parameter to a transceiver channel '
    eqw += 'Usage: eqwrite <channel> <parameter> <value>'
    eqw += ' - List of parameters: 0 = CTLE, 1 = VGA, 2 = DCGAIN, '
    eqw += '3 = 1st POST, 4 = 2nd POST, 5 = 1st PRE, 6 = 2nd PRE, 7 = VOD'

    eqr = 'Read out an equalization parameter from a transceiver channel '
    eqr += 'Usage: eqread <channel> <parameter> '
    eqr += ' - List of parameters: 0 = CTLE, 1 = VGA, 2 = DCGAIN, '
    eqr += '3 = 1st POST, 4 = 2nd POST, 5 = 1st PRE, 6 = 2nd PRE, 7 = VOD'
    subcmds = [
        {'subcmd': 'stat',
         'help': 'Print the SKL nios statistics',
         'function': stat_fxn},
        {'subcmd': 'eeprom',
         'help': ('Read out 128-bit unique id, '
                  'MAC address and board specific ids from EEPROM'),
         'function': eeprom_fxn},
        {'subcmd': 'eqwrite',
         'help': eqw,
         'arg0': 1,
         'arg1': 1,
         'arg2': 1,
         'function': eqwrite_fxn},
        {'subcmd': 'eqread',
         'help': eqr,
         'arg0': 1,
         'arg1': 1,
         'function': eqread_fxn},
        {'subcmd': 'e10init',
         'help': 'Initialize and turn on loopback E10 AFU',
         'function': e10_init_fxn},
        {'subcmd': 'e10loop',
         'help': 'Turn on or off internal loopback in the E10 AFU',
         'flag': 1,
         'function': e10_loop_fxn},
        {'subcmd': 'e10reset',
         'help': 'Turn on or off the ACLRs in the E10 AFU',
         'flag': 1,
         'function': e10_reset_fxn},
        {'subcmd': 'e10send',
         'help': 'Send packets out all E10 ports',
         'function': e10_pkt_send_fxn},
        {'subcmd': 'e10stat',
         'help': 'Print out packet statistics for all E10 ports',
         'function': e10_stat_fxn},
        {'subcmd': 'e10statclr',
         'help': 'Clears packet statistics for all E10 ports',
         'function': e10_stat_clr_fxn},
        {'subcmd': 'e40init',
         'help': 'Initialize and turn on loopback E40 AFU',
         'function': e40_init_fxn},
        {'subcmd': 'e40loop',
         'help': 'Turn on or off internal loopback in the E40 AFU',
         'flag': 1,
         'function': e40_loop_fxn},
        {'subcmd': 'e40reset',
         'help': 'Turn on or off the ACLRs in the E40 AFU',
         'flag': 1,
         'function': e40_reset_fxn},
        {'subcmd': 'e40send',
         'help': 'Send packets out E40 port',
         'function': e40_pkt_send_fxn},
        {'subcmd': 'e40stat',
         'help': 'Print out packet statistics for E40 port',
         'function': e40_stat_fxn},
        {'subcmd': 'e40statclr',
         'help': 'Clears packet statistics for E40 port',
         'function': e40_stat_clr_fxn},
    ]
    usage = "{} [-h] subcommand [subarg] [bdf]".format(sys.argv[0])
    formatter = argparse.RawDescriptionHelpFormatter

    epi = "[subarg] - subcommand specific argument\n"
    epi += "[bdf] - bdf of device to configure "
    epi += "(e.g. 04:00.0 or 0000:04:00.0)\n"
    epi += "        optional when one device in system"

    top_parser = argparse.ArgumentParser(usage=usage,
                                         epilog=epi,
                                         formatter_class=formatter)

    subparsers = top_parser.add_subparsers(title='subcommands',
                                           metavar="",
                                           help=None)

    for subcmd in subcmds:
        subparser = subparsers.add_parser(
            subcmd['subcmd'], help=subcmd['help'])

        if subcmd.get('flag', 0):
            subparser.add_argument('flag', type=str, choices=['on', 'off'])
        if subcmd.get('arg0', 0):
            subparser.add_argument('channel', type=str)
        if subcmd.get('arg1', 0):
            subparser.add_argument('parameter', type=str)
        if subcmd.get('arg2', 0):
            subparser.add_argument('value', type=str)
        subparser.add_argument('bdf', nargs='?')
        subparser.set_defaults(func=subcmd['function'])

    return top_parser.parse_args()


def validate_bdf(args):
    devs = get_pcie_bdfs(0x8086, 0x09c4)
    if not devs:
        sys.exit("No valid cards found")

    if not args.bdf:
        if len(devs) > 1:
            for dev in devs:
                print("    {}".format(dev))
            sys.exit("Must specify a bdf. More than one device found.")
        else:
            args.bdf = devs[0]
    else:
        norm_bdf = normalize_bdf(args.bdf)

        args.bdf = norm_bdf
        if args.bdf not in devs:
            text = "Invalid bdf, {}. Not one of the following".format(args.bdf)
            for dev in devs:
                print("    {}".format(dev))
            sys.exit(text)


def main():
    args = parse_args()

    validate_bdf(args)

    mem = PhyMemAccess()
    mem.open(get_bdf_base_addr(args.bdf, 0), 0x80000)
    skl = SklHssi(mem)

    mem2 = PhyMemAccess()

    mem2.open(get_bdf_base_addr(args.bdf, 2), 0x80000)

    skl.skl_set_mem2(mem2)

    args.func(args, skl)

    mem.close()
    mem2.close()


if __name__ == '__main__':
    main()
