#! /usr/bin/env python
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

"""Program non-ROT to ROT
"""

import argparse
import struct
import fnmatch
import glob
import sys
import tempfile
import os
import fcntl
import binascii
import filecmp
import stat
import subprocess
import re
import time
import datetime
import json
import shutil
import logging
import logging.handlers
from array import array
from opae.admin.fpga import fpga
from opae.admin.tools import rsu
from opae.admin.tools.rsu import do_rsu


LOG = logging.getLogger()


# MTD ioctl
IOCTL_MTD_MEMGETINFO = 0x80204d01
IOCTL_MTD_MEMERASE = 0x40084d02
IOCTL_MTD_MEMUNLOCK = 0x40084d06
# MTD block size
FPGA_FLASH_BLOCK_SIZE = 4096

# FPGA device ID
D5005_DEV_ID = 0x0b2b
N3000_DEV_ID = 0x0b30

#D5005
FPGA_D5005_ERASE_START = 0x7800000
FPGA_D5005_ERASE_END = 0xfffffff

FPGA_D5005_NIOS_USER_ERASE_START = 0x0
FPGA_D5005_NIOS_USER_ERASE_END = 0x800000

FPGA_D5005_FPGA_FACTORY_START =0x20000 
FPGA_D5005_FPGA_FACTORY_END = 0x381FFFF

FPGA_D5005_CFM1_ERASE_START = 0x70000
FPGA_D5005_CFM1_ERASE_END = 0xB7FFF

FPGA_D5005_CFM2_ERASE_START = 0x10000 
FPGA_D5005_CFM2_ERASE_END = 0x6FFFF

# n3000
FPGA_N3000_ERASE_FLASH1_START = 0x7800000 
FPGA_N3000_ERASE_FLASH1_END = 0x7FFFFFF

FPGA_N3000_ERASE_FLASH2_START = 0x0 
FPGA_N3000_ERASE_FLASH2_END = 0x8000000

FPGA_N3000_FPGA_FACTORY_START =0x20000 
FPGA_N3000_FPGA_FACTORY_END = 0x381FFFF

FPGA_N3000_CFM1_ERASE_START = 0x70000
FPGA_N3000_CFM1_ERASE_END = 0xB7FFF

FPGA_N3000_CFM2_ERASE_START = 0x10000 
FPGA_N3000_CFM2_ERASE_END = 0x6FFFF

FPGA_N3000_NIOS_USER_ERASE_START = 0x0
FPGA_N3000_NIOS_USER_ERASE_END = 0x800000

# Json config file & Binary path
OPAE_SHARE = '/usr/share/opae'


# class MTD
class mtd(object):
    @staticmethod
    def size(dev):
        # meminfo ioctl
        ioctl_data = struct.pack('BIIIIIQ', 0, 0, 0, 0, 0, 0, 0)
        with os.fdopen(os.open(dev, os.O_SYNC | os.O_RDONLY), 'r') as file_:
            try:
                ret = fcntl.ioctl(
                                file_.fileno(),
                                IOCTL_MTD_MEMGETINFO,
                                ioctl_data)
            except IOError as exc:
                LOG.exception('Flash meminfo fails error no: %d,strerror:%s',exc.errno,exc.strerror)
                return -1

        ioctl_odata = struct.unpack_from('BIIIIIQ', ret)
        return ioctl_odata[2]

    @staticmethod
    def erase(dev, start, nbytes):
        LOG.debug('erasing 0x%08x bytes starting at 0x%08x',nbytes, start)

        ioctl_data = struct.pack('II', start, nbytes)
        with os.fdopen(os.open(dev, os.O_SYNC | os.O_RDWR), 'w') as file_:
            fcntl.ioctl(file_.fileno(), IOCTL_MTD_MEMERASE, ioctl_data)

    @staticmethod
    def read(dev, start, nbytes, ofile):
        LOG.debug('reading 0x%08x bytes to 0x%08x',nbytes, start)

        with os.fdopen(os.open(dev, os.O_RDONLY), 'r') as file_:
            offset = os.lseek(file_.fileno(), start, os.SEEK_SET)
            if offset != start:
                raise Exception("Seek to start failed")

            while nbytes > 0:
                if nbytes > FPGA_FLASH_BLOCK_SIZE:
                    rbytes = FPGA_FLASH_BLOCK_SIZE
                else:
                    rbytes = nbytes

                ichunk = os.read(file_.fileno(), rbytes)

                if not ichunk:
                    raise Exception("read of flash failed")

                ofile.write(ichunk)
                nbytes -= rbytes

    @staticmethod
    def write(dev, start, nbytes, ifile):
        LOG.debug('writing 0x%08x bytes to 0x%08x',nbytes, start)

        last_write_position = start
        with os.fdopen(os.open(dev, os.O_SYNC | os.O_RDWR), 'a') as file_:
            offset = os.lseek(file_.fileno(), start, os.SEEK_SET)
            if offset != start:
                raise Exception("Seek to start failed")

            while nbytes > 0:
                if nbytes > FPGA_FLASH_BLOCK_SIZE:
                    rbytes = FPGA_FLASH_BLOCK_SIZE
                else:
                    rbytes = nbytes

                ichunk = ifile.read(rbytes)

                if not ichunk:
                    raise Exception("read of flash failed")

                if all([c == '\xFF' for c in ichunk]):
                      os.lseek(file_.fileno(), rbytes, os.SEEK_CUR)
                else:
                    os.write(file_.fileno(), ichunk)
                    last_write_position = file_.tell()

                nbytes -= rbytes

        bytes_written = last_write_position - start
        LOG.debug('actual bytes written 0x%x - 0x%x = 0x%x',last_write_position, start, bytes_written)

        return bytes_written
# end of MTD


def write_file(fname, data):
    with open(fname, 'wb') as wfile:
        wfile.write(data)

def update_flash_verify(dev, filename, intput_offset, len):
    with open(filename, 'rb') as fd:
        file_size = os.path.getsize(fd.name)
        if file_size > len:
            LOG.error('Invalid file size:%s',filename)
            return -1

        fd.seek(0)
        bytes_written  = mtd.write(dev,intput_offset,file_size,fd)
        LOG.debug('bytes written to flash 0x%x',bytes_written)

        with open(fd.name, 'rb+') as rfile:
            rfile.truncate(bytes_written)

        temp_file = tempfile.NamedTemporaryFile(mode='wb', delete=False)
        mtd.read(dev, intput_offset, bytes_written, temp_file)
        temp_file.close()

        LOG.debug('verifying flash')
        retval = filecmp.cmp(fd.name, temp_file.name)
        fd.close()
        os.remove(temp_file.name)
        if retval:
            LOG.debug('Flash successfully verified')
        else:
            LOG.error('Failed to verify flash')
            return -1
    return 0

class flash_data(object):
    def __init__(self,data,path):
        self.filename = data.get('filename')
        self.file = os.path.join(path, data.get('filename'))
        self.type = data.get('type')
        self.start = int(data.get('start'),16)
        self.end = int(data.get('end'),16)

    def __str__(self):
        return 'type: {:25s}  filename: {:60s} start: {:0x}  end: {:0x} '.format(
            self.type, self.filename, self.start, self.end)

    @classmethod
    def print_data(self):
        LOG.debug('file :%s', self.filename)
        LOG.debug('start: 0x%x ', self.start)
        LOG.debug('end: 0x%x ', self.start)


class fpga_cfg_data(object):
    def __init__(self):
        self.product = None
        self.vendor = None
        self.device = None
        self.bmc_root_key_hash = flash_data()
        self.bmc_root_key_program = flash_data()
        self.sr_root_key_hash = flash_data()
        self.sr_root_key_program = flash_data()
        self.bmc_dts = flash_data()
        self.nios_factory_header = flash_data()
        self.nios_factory = flash_data()
        self.fpga_factory = flash_data()
        self.nios_bootloader = flash_data()
        self.max10_factory = flash_data()
        self.max10_user = flash_data()
        self.nios_user_header = flash_data()
        self.nios_user = flash_data()

    @classmethod
    def print_json_cfg(self):
        LOG.debug('\n\n')
        LOG.debug('FPGA JSON CONFIG DATA')
        LOG.debug('product:%s',self.product)
        LOG.debug('vendor:%s',self.vendor)
        LOG.debug('device:%s',self.device)
        LOG.debug(self.bmc_root_key_hash)
        LOG.debug(self.bmc_root_key_program)
        LOG.debug(self.sr_root_key_hash)
        LOG.debug(self.sr_root_key_program)
        LOG.debug(self.bmc_dts)
        LOG.debug(self.nios_factory_header)
        LOG.debug(self.nios_factory)
        LOG.debug(self.fpga_factory)
        LOG.debug(self.nios_bootloader)
        LOG.debug(self.max10_factory)
        LOG.debug(self.max10_user)
        LOG.debug(self.nios_user_header)
        LOG.debug(self.nios_user)
        LOG.debug('\n \n ')

    @classmethod
    def check_fpga_images(self):
        if(not os.path.isfile(self.bmc_root_key_hash.file)):
            LOG.error('file does not exist:%s',self.bmc_root_key_hash.file)
            return False

        if(not os.path.isfile(self.bmc_root_key_program.file)):
            LOG.error('file does not exist:%s',self.bmc_root_key_program.file)
            return False

        if(not os.path.isfile(self.sr_root_key_hash.file)):
            LOG.error('file does not exist:%s',self.sr_root_key_hash.file)
            return False

        if(not os.path.isfile(self.sr_root_key_program.file)):
            LOG.error('file does not exist:%s',self.sr_root_key_program.file)
            return False

        if(not os.path.isfile(self.bmc_dts.file)):
            LOG.error('file does not exist:%s',self.bmc_dts.file)
            return False

        if(not os.path.isfile(self.nios_factory_header.file)):
            LOG.error('file does not exist:%s',self.nios_factory_header.file)
            return False

        if(not os.path.isfile(self.fpga_factory.file)):
            LOG.error('file does not exist:%s',self.fpga_factory.file)
            return False

        if(not os.path.isfile(self.nios_bootloader.file)):
            LOG.error('file does not exist:%s',self.nios_bootloader.file)
            return False

        if(not os.path.isfile(self.max10_factory.file)):
            LOG.error('file does not exist:%s',self.max10_factory.file)
            return False

        if(not os.path.isfile(self.max10_user.file)):
            LOG.error('file does not exist:%s',self.max10_user.file)
            return False

        if(not os.path.isfile(self.nios_user_header.file)):
            LOG.error('file does not exist:%s',self.nios_user_header.file)
            return False

        if(not os.path.isfile(self.nios_user.file)):
            LOG.error('file does not exist:%s',self.nios_user.file)
            return False

        return True

    @classmethod
    def parse_json_cfg(cls,filename,path_bin):
        with open(filename,'r')  as file_config:
             config_data = json.load(file_config)
             if not config_data:
                 LOG.error('Failed to load filename %s', filename)
                 return None;

             o = cls

             # Enum list
             for fd in config_data.get('flash', []):
                if(fd.get('type') =='bmc_root_key_hash'):
                   o.bmc_root_key_hash=flash_data(fd,path_bin)

                if(fd.get('type') =='bmc_root_key_program'):
                   o.bmc_root_key_program=flash_data(fd,path_bin)

                if(fd.get('type') =='sr_root_key_hash'):
                   o.sr_root_key_hash=flash_data(fd,path_bin)

                if(fd.get('type') =='sr_root_key_program'):
                   o.sr_root_key_program=flash_data(fd,path_bin)

                if(fd.get('type') =='nios_bootloader'):
                   o.nios_bootloader=flash_data(fd,path_bin)

                if(fd.get('type') =='nios_factory'):
                   o.nios_factory=flash_data(fd,path_bin)

                if(fd.get('type') =='nios_factory_header'):
                   o.nios_factory_header=flash_data(fd,path_bin)

                if(fd.get('type') =='fpga_factory'):
                   o.fpga_factory=flash_data(fd,path_bin)

                if(fd.get('type') =='max10_factory'):
                   o.max10_factory=flash_data(fd,path_bin)

                if(fd.get('type') =='max10_user'):
                   o.max10_user=flash_data(fd,path_bin)

                if(fd.get('type') =='bmc_dts'):
                   o.bmc_dts=flash_data(fd,path_bin)

                if(fd.get('type') =='nios_user'):
                   o.nios_user=flash_data(fd,path_bin)

                if(fd.get('type') =='nios_user_header'):
                   o.nios_user_header=flash_data(fd,path_bin)

             # product
             o.product = config_data.get('product')
             if o.product is None:
                 LOG.error('Invalid product')
                 return None

             # vendor
             o.vendor = config_data.get('vendor')
             if o.vendor is None:
                 LOG.error('Invalid vendor')
                 return None

             # device
             o.device = config_data.get('device')
             if o.device is None:
                 LOG.error('Invalid device')
                 return None

             return o

        return None


def get_mtd_from_spi_path(mode_path, spi_path):

    write_file(mode_path, "1")

    for i in range(20):

        time.sleep(1)

        mtds = glob.glob(os.path.join(spi_path, '*.*.auto',
                                      'mtd', 'mtd*'))

        for mtd in mtds:
            if not fnmatch.fnmatchcase(mtd, "*ro"):
                mtd_dev = os.path.join('/dev', os.path.basename(mtd))
                LOG.debug('using %s',mtd_dev)
                return mtd_dev

    raise Exception("no mtd node found for mode %s" % (mode_path))


def get_flash_mode(spi_path):
    modes = []

    for prefix in ['fpga', 'bmcfw', 'bmcimg']:
        path = os.path.join(spi_path, '{}_flash_ctrl'.format(prefix),
                            '{}_flash_mode'.format(prefix))
        with open(path, 'r') as fd:
            modes.append(int(fd.read(1)))

    # return 1 if any type of flash is in use
    return 1 if any(modes) else 0



# FPGA D5005 RoT update
def d5005_fpga_update(fpga,fpga_cfg_data):

    LOG.debug('---------------FPGA D5005 RoT update starts-------------')

    # check for ROT 
    sec_dev = fpga.secure_dev
    if sec_dev:
        LOG.info('FPGA Firmware updated to secure')
        return 0

    LOG.debug('FPGA Firmware Not Updated to secure')
    LOG.debug('spi path:%s',fpga.fme.spi_bus.sysfs_path)


    # Check for flash status
    if get_flash_mode(fpga.fme.spi_bus.sysfs_path) ==1:
        LOG.error('fpgaflash on %s is in progress, try later', fpga.fme.spi_bus.sysfs_path)
        return -1


    mode_path = os.path.join(fpga.fme.spi_bus.sysfs_path,
                         'fpga_flash_ctrl', 'fpga_flash_mode')


    # Enable fpga_flash_mode
    mtd_dev = get_mtd_from_spi_path(mode_path, fpga.fme.spi_bus.sysfs_path)

    LOG.debug('mode_path:%s',mode_path)
    LOG.debug('spi mtd_dev:%s',mtd_dev)


    # FLASH SIZE 
    ret_size = mtd.size(mtd_dev)
    LOG.debug('Flash size:0x%08x',ret_size)


    # Erase from 0x780.0000 to 0xfff.ffff
    LOG.debug('Erase Flash from 0x%08x to 0x%08x',
             FPGA_D5005_ERASE_START,FPGA_D5005_ERASE_END)

    try:
        mtd.erase(mtd_dev,
                        FPGA_D5005_ERASE_START ,
                        (FPGA_D5005_ERASE_END -FPGA_D5005_ERASE_START)+1 )
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")


    # NIOS factory
    LOG.debug('Updates NIOS factory  %s from 0x%08x to 0x%08x',fpga_cfg_data.nios_factory.file,
            fpga_cfg_data.nios_factory.start, fpga_cfg_data.nios_factory.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.nios_factory.file,
                                fpga_cfg_data.nios_factory.start,
                                (fpga_cfg_data.nios_factory.end - fpga_cfg_data.nios_factory.start) +1 )
    if retval == 0:
          LOG.debug('Successfully update & verified NIOS factory')
    else:
        LOG.error('Failed update & verify NIOS factory')
        write_file(mode_path, "0")
        return -1

    #NIOS Header
    LOG.debug('Updates NIOS factory header %s from 0x%08x to 0x%08x',fpga_cfg_data.nios_factory_header.file,
            fpga_cfg_data.nios_factory_header.start, fpga_cfg_data.nios_factory_header.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.nios_factory_header.file,
                                fpga_cfg_data.nios_factory_header.start,
                                ( fpga_cfg_data.nios_factory_header.end- fpga_cfg_data.nios_factory_header.start) +1 )
    if retval == 0:
        LOG.debug('Successfully update & verified NIOS factory header')
    else:
        LOG.error('Failed update & verify NIOS factory header')
        write_file(mode_path, "0")
        return -1


    #BMC ROOT KEY HASH
    LOG.debug('Updates bmc root key hash %s from 0x%08x to 0x%08x ',fpga_cfg_data.bmc_root_key_hash.file,
            fpga_cfg_data.bmc_root_key_hash.start, fpga_cfg_data.bmc_root_key_hash.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.bmc_root_key_hash.file,
                                fpga_cfg_data.bmc_root_key_hash.start,
                                (fpga_cfg_data.bmc_root_key_hash.end - fpga_cfg_data.bmc_root_key_hash.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified bmc key hash')
    else:
        LOG.error('Failed update & verify bmc key hash')
        write_file(mode_path, "0")
        return -1

    LOG.debug('Updates bmc root key program %s from 0x%08x to 0x%08x ',fpga_cfg_data.bmc_root_key_program.file,
            fpga_cfg_data.bmc_root_key_program.start, fpga_cfg_data.sr_root_key_program.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.bmc_root_key_program.file,
                                fpga_cfg_data.bmc_root_key_program.start,
                                (fpga_cfg_data.sr_root_key_program.end - fpga_cfg_data.bmc_root_key_program.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified bmc key program')
    else:
        LOG.error('Failed update & verify bmc key program')
        write_file(mode_path, "0")
        return -1


    #SR ROOT KEY HASH
    LOG.debug('Updates SR root key hash %s from 0x%08x to 0x%08x ', fpga_cfg_data.sr_root_key_hash.file,
             fpga_cfg_data.sr_root_key_hash.start, fpga_cfg_data.sr_root_key_hash.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.sr_root_key_hash.file,
                                fpga_cfg_data.sr_root_key_hash.start,
                                (fpga_cfg_data.sr_root_key_hash.end - fpga_cfg_data.sr_root_key_hash.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified Updates SR root key hash')
    else:
        LOG.error('Failed update & verify Updates SR root key hash')
        write_file(mode_path, "0")
        return -1

    LOG.debug('Updates SR root key program %s from 0x%08x to 0x%08x ', fpga_cfg_data.sr_root_key_program.file,
             fpga_cfg_data.sr_root_key_program.start, fpga_cfg_data.sr_root_key_program.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.sr_root_key_program.file,
                                fpga_cfg_data.sr_root_key_program.start,
                                (fpga_cfg_data.sr_root_key_program.end - fpga_cfg_data.sr_root_key_program.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified Updates SR root key program')
    else:
        LOG.error('Failed update & verify Updates SR root key program')
        write_file(mode_path, "0")
        return -1


    #Erase/Write/Verify DTS 0x382.0000 - 3FF.FFFF
    LOG.debug('Updates device tree %s from 0x%08x to 0x%08x',fpga_cfg_data.bmc_dts.file,
             fpga_cfg_data.bmc_dts.start, fpga_cfg_data.bmc_dts.end)


    try:
         mtd.erase(mtd_dev,
                     fpga_cfg_data.bmc_dts.start,
                      (fpga_cfg_data.bmc_dts.end - fpga_cfg_data.bmc_dts.start)+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")


    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.bmc_dts.file,
                                fpga_cfg_data.bmc_dts.start,
                                (fpga_cfg_data.bmc_dts.end - fpga_cfg_data.bmc_dts.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified  device tree')
    else:
        LOG.error('Failed update & verify  device tree')
        write_file(mode_path, "0")
        return -1


    #Erase/Write/Verify FPGA image
    LOG.debug('Updates FPGA image %s from 0x%08x to 0x%08x',fpga_cfg_data.fpga_factory.file,
             fpga_cfg_data.fpga_factory.start, fpga_cfg_data.fpga_factory.end)

    try:
         mtd.erase(mtd_dev,
                      FPGA_D5005_FPGA_FACTORY_START,
                     (FPGA_D5005_FPGA_FACTORY_END - FPGA_D5005_FPGA_FACTORY_START)+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.fpga_factory.file,
                                fpga_cfg_data.fpga_factory.start,
                                (fpga_cfg_data.fpga_factory.end - fpga_cfg_data.fpga_factory.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified  FPGA Image')
    else:
        LOG.error('Failed update & verify  FPGA Image')
        write_file(mode_path, "0")
        return -1

    # Disable fpga_flash_mode
    write_file(mode_path, "0")


     # Enable bmcimg_flash_mode
    if get_flash_mode(fpga.fme.spi_bus.sysfs_path) == 1:
        LOG.error('fpgaflash on %s is in progress, try later', fpga.fme.spi_bus.sysfs_path)
        return -1

    mode_path = os.path.join(fpga.fme.spi_bus.sysfs_path,
                         'bmcimg_flash_ctrl', 'bmcimg_flash_mode')

    mtd_dev = get_mtd_from_spi_path(mode_path, fpga.fme.spi_bus.sysfs_path)

    LOG.debug('mode_path:%s',mode_path)
    LOG.debug('spi mtd_dev:%s',mtd_dev)

    # UFM1 / NIOS Bootloader
    LOG.debug('Updates max10 UFM1 fw %s from 0x%08x to 0x%08x ',fpga_cfg_data.nios_bootloader.file,
              fpga_cfg_data.nios_bootloader.start, fpga_cfg_data.nios_bootloader.end)

    try:
         mtd.erase(mtd_dev,
                      fpga_cfg_data.nios_bootloader.start ,
                     (fpga_cfg_data.nios_bootloader.end - fpga_cfg_data.nios_bootloader.start )+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")



    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.nios_bootloader.file,
                                fpga_cfg_data.nios_bootloader.start,
                                (fpga_cfg_data.nios_bootloader.end - fpga_cfg_data.nios_bootloader.start )+1)

    if retval == 0:
        LOG.debug('Successfully update & verified max10 UFM1 fw')
    else:
        LOG.error('Failed update & verify Updates max10 UFM1 fw')
        write_file(mode_path, "0")
        return -1



    #CFM1 User
    LOG.debug('Updates MAX10 CFM1/User %s from 0x%08x to 0x%08x ',fpga_cfg_data.max10_user.file,
              fpga_cfg_data.max10_user.start, fpga_cfg_data.max10_user.end)

    try:
        mtd.erase(mtd_dev,
                    FPGA_D5005_CFM1_ERASE_START,
                    (FPGA_D5005_CFM1_ERASE_END -FPGA_D5005_CFM1_ERASE_START)+1)

        mtd.erase(mtd_dev,
                  FPGA_D5005_CFM2_ERASE_START,
                  (FPGA_D5005_CFM2_ERASE_END - FPGA_D5005_CFM2_ERASE_START )+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.max10_user.file,
                                fpga_cfg_data.max10_user.start,
                               (fpga_cfg_data.max10_user.end - fpga_cfg_data.max10_user.start)+1)

    if retval == 0:
        LOG.debug('Successfully update & verified max10 user fw')
    else:
        LOG.error('Failed update & verify Updates max10 user fw')
        write_file(mode_path, "0")
        return -1


    # MAX10 FACTORY
    LOG.debug('Updates MAX10 factory fw %s from 0x%08x to 0x%08x ',fpga_cfg_data.max10_factory.file,
              fpga_cfg_data.max10_factory.start, fpga_cfg_data.max10_factory.end)

    try:
        mtd.erase(mtd_dev,
                            fpga_cfg_data.max10_factory.start ,
                            (fpga_cfg_data.max10_factory.end - fpga_cfg_data.max10_factory.start)+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.max10_factory.file,
                                fpga_cfg_data.max10_factory.start,
                                (fpga_cfg_data.max10_factory.end - fpga_cfg_data.max10_factory.start)+1)

    if retval == 0:
        LOG.debug('Successfully update & verified MAX10 factory')
    else:
        LOG.error('Failed update & verify Updates MAX10 factory')
        write_file(mode_path, "0")
        return -1


    # Disable bmcimg_flash_mode
    write_file(mode_path, "0")


    # Enable bmcfw_flash_mode
    if get_flash_mode(fpga.fme.spi_bus.sysfs_path) == 1:
        LOG.error('fpgaflash on %s is in progress, try later', fpga.fme.spi_bus.sysfs_path)
        return -1


    mode_path = os.path.join(fpga.fme.spi_bus.sysfs_path,
                         'bmcfw_flash_ctrl', 'bmcfw_flash_mode')

    mtd_dev = get_mtd_from_spi_path(mode_path, fpga.fme.spi_bus.sysfs_path)

    LOG.debug('mode_path:%s',mode_path)
    LOG.debug('spi mtd_dev:%s',mtd_dev)

    try:
        mtd.erase(mtd_dev,
                  FPGA_D5005_NIOS_USER_ERASE_START,
                  FPGA_D5005_NIOS_USER_ERASE_END)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    #NIOS USER
    LOG.debug('Updates NIOS user  %s from 0x%08x to 0x%08x ',fpga_cfg_data.nios_user.file,
              fpga_cfg_data.nios_user.start, fpga_cfg_data.nios_user.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.nios_user.file,
                                fpga_cfg_data.nios_user.start,
                                (fpga_cfg_data.nios_user.end - fpga_cfg_data.nios_user.start)+1)\

    if retval == 0:
        LOG.debug('Successfully update & verified NIOS user')
    else:
        LOG.error('Failed update & verify Updates NIOS user')
        write_file(mode_path, "0")
        return -1


    #NIOS USER Header
    LOG.debug('Updates NIOS user header %s from 0x%08x to 0x%08x',fpga_cfg_data.nios_user_header.file,
         fpga_cfg_data.nios_user_header.start, fpga_cfg_data.nios_user_header.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.nios_user_header.file,
                                fpga_cfg_data.nios_user_header.start,
                                (fpga_cfg_data.nios_user_header.end - fpga_cfg_data.nios_user_header.start)+1)

    if retval == 0:
        LOG.debug('Successfully update & verified NIOS user header ')
    else:
        LOG.error('Failed update & verify Updates NIOS user header ')
        write_file(mode_path, "0")
        return -1


     # Disable bmcfw_flash_mode
    write_file(mode_path, "0")


    LOG.debug('---------END OF UPDATE------------------ ')
    return 0



def check_n3000_max10_temporary(bdf):
    sysfs_path = os.path.join('/sys/bus/pci/devices/', bdf,
            'fpga/intel-fpga-dev.*/intel-fpga-fme.*',
            'spi-*/spi_master/spi*/spi*/intel-generic-qspi*.auto')

    LOG.debug('sysfs_path:%s',sysfs_path)

    dirs = glob.glob(sysfs_path)

    if len(dirs) < 1:
        LOG.debug('Not updated with temporary Max10 user image')
        return 1

    if len(dirs) >= 1:
        LOG.debug('Updated with temporary Max10 user image')

    return 0


def get_mtd_list_from_spi_path(mode_path, spi_path):

    write_file(mode_path, "1")

    mtd_list = list();

    for i in range(20):

        time.sleep(1)

        mtds = glob.glob(os.path.join(spi_path, '*.*.auto',
                                      'mtd', 'mtd*'))
        for mtd in mtds:
             if not fnmatch.fnmatchcase(mtd, "*ro"):
                  mtd_dev = os.path.join('/dev', os.path.basename(mtd))
                  mtd_list.append(mtd_dev)
                  if len(mtds)/2 == len(mtd_list) :
                       return mtd_list

    raise Exception("no mtd node found for mode %s" % (mode_path))



def n3000_fpga_update(fpga,fpga_cfg_data):
 
    LOG.debug('----n3000 update start-------')

    retval = check_n3000_max10_temporary(fpga.pci_node.pci_address)
    if retval != 0:
        LOG.error('Not Updated with temporary Max10 user image')
        return -1


    # Check for flash status
    if get_flash_mode(fpga.fme.spi_bus.sysfs_path) == 1:
        LOG.error('fpgaflash on %s is in progress, try later', fpga.fme.spi_bus.sysfs_path)
        return -1

    LOG.debug('fpga.fme.spi_bus.sysfs_path:%s',fpga.fme.spi_bus.sysfs_path)

    mode_path = os.path.join(fpga.fme.spi_bus.sysfs_path,
                         'fpga_flash_ctrl', 'fpga_flash_mode')

    mtd_dev_list = get_mtd_list_from_spi_path(mode_path, fpga.fme.spi_bus.sysfs_path)

    LOG.debug('mode_path:%s',mode_path)
    LOG.debug('spi mtd_dev:%s',mtd_dev_list)

    if len(mtd_dev_list) != 2:
        LOG.error('Invalid flash devices')
        return -1

    # FLASH SIZE 
    ret_size = mtd.size(mtd_dev_list[0])
    LOG.debug('mtd_dev_list[0]:%s',mtd_dev_list[0])
    LOG.debug('Flash size:0x%08x',ret_size)

    ret_size = mtd.size(mtd_dev_list[1])
    LOG.debug('mtd_dev_list[1]:%s',mtd_dev_list[1])
    LOG.debug('Flash size:0x%08x',ret_size)


    # Second FLASH
    LOG.debug('Erase Flash from 0x%08x to 0x%08x',
             FPGA_N3000_ERASE_FLASH2_START,FPGA_N3000_ERASE_FLASH2_END)
    try:
        mtd.erase(mtd_dev_list[0],
                  FPGA_N3000_ERASE_FLASH2_START,
                   FPGA_N3000_ERASE_FLASH2_END)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    # Write/Verify NIOS factory firmware to 0x3800000  0x3a00fff of Second FPGA flash
    LOG.debug('Updates NIOS factory  %s from 0x%08x to 0x%08x',fpga_cfg_data.nios_factory.file,
            fpga_cfg_data.nios_factory.start, fpga_cfg_data.nios_factory.end)

    retval = update_flash_verify(mtd_dev_list[0],
                                fpga_cfg_data.nios_factory.file,
                                fpga_cfg_data.nios_factory.start ,
                                (fpga_cfg_data.nios_factory.end - fpga_cfg_data.nios_factory.start)+1 )
    
    if retval == 0:
         LOG.debug('Successfully update & verified NIOS factory')
    else:
        LOG.error('Failed update & verify NIOS factory')
        write_file(mode_path, "0")
        return -1

    #NIOS Header
    LOG.debug('Updates NIOS factory header %s from 0x%08x to 0x%08x',fpga_cfg_data.nios_factory_header.file,
            fpga_cfg_data.nios_factory_header.start, fpga_cfg_data.nios_factory_header.end)

    retval = update_flash_verify(mtd_dev_list[0],
                                fpga_cfg_data.nios_factory_header.file,
                                fpga_cfg_data.nios_factory_header.start,
                                (fpga_cfg_data.nios_factory_header.end - fpga_cfg_data.nios_factory_header.start) +1)

    if retval == 0:
        LOG.debug('Successfully update & verified NIOS factory header')
    else:
        LOG.error('Failed update & verify NIOS factory header')
        write_file(mode_path, "0")
        return -1

    # First FLASH
    # Erase from 0x7FFB0000 to 0x7FFFFFF (end) of FPGA flash
    LOG.debug('Erase Flash from 0x%08x to 0x%08x',
             FPGA_N3000_ERASE_FLASH1_START,FPGA_N3000_ERASE_FLASH1_END)

    try:
        mtd.erase(mtd_dev_list[1],
                 FPGA_N3000_ERASE_FLASH1_START,
                 (FPGA_N3000_ERASE_FLASH1_END - FPGA_N3000_ERASE_FLASH1_START )+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    #BMC ROOT KEY HASH
    # Write/Verify BMC key material to First FPGA flash?
    LOG.debug('Updates bmc root key hash %s from 0x%08x to 0x%08x ',fpga_cfg_data.bmc_root_key_hash.file,
            fpga_cfg_data.bmc_root_key_hash.start, fpga_cfg_data.bmc_root_key_hash.end)

    retval = update_flash_verify(mtd_dev_list[1],
                                fpga_cfg_data.bmc_root_key_hash.file,
                                fpga_cfg_data.bmc_root_key_hash.start,
                                (fpga_cfg_data.bmc_root_key_hash.end - fpga_cfg_data.bmc_root_key_hash.start ) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified bmc key hash')
    else:
        LOG.error('Failed update & verify bmc key hash')
        write_file(mode_path, "0")
        return -1

    LOG.debug('Updates bmc root key program %s from 0x%08x to 0x%08x ',fpga_cfg_data.bmc_root_key_program.file,
            fpga_cfg_data.bmc_root_key_program.start, fpga_cfg_data.sr_root_key_program.end)

    retval = update_flash_verify(mtd_dev_list[1],
                                fpga_cfg_data.bmc_root_key_program.file,
                                fpga_cfg_data.bmc_root_key_program.start,
                                (fpga_cfg_data.sr_root_key_program.end - fpga_cfg_data.bmc_root_key_program.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified bmc key program')
    else:
        LOG.debug('Failed update & verify bmc key program')
        write_file(mode_path, "0")
        return -1

     #SR ROOT KEY HASH
    LOG.debug('Updates SR root key hash %s from 0x%08x to 0x%08x ', fpga_cfg_data.sr_root_key_hash.file,
             fpga_cfg_data.sr_root_key_hash.start, fpga_cfg_data.sr_root_key_hash.end)

    retval = update_flash_verify(mtd_dev_list[1],
                                fpga_cfg_data.sr_root_key_hash.file,
                                fpga_cfg_data.sr_root_key_hash.start,
                                (fpga_cfg_data.sr_root_key_hash.end - fpga_cfg_data.sr_root_key_hash.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified Updates SR root key hash')
    else:
        LOG.error('Failed update & verify Updates SR root key hash')
        write_file(mode_path, "0")
        return -1

    LOG.debug('Updates SR root key program %s from 0x%08x to 0x%08x ', fpga_cfg_data.sr_root_key_program.file,
             fpga_cfg_data.sr_root_key_program.start, fpga_cfg_data.sr_root_key_program.end)

    retval = update_flash_verify(mtd_dev_list[1],
                                fpga_cfg_data.sr_root_key_program.file,
                                fpga_cfg_data.sr_root_key_program.start,
                                (fpga_cfg_data.sr_root_key_program.end - fpga_cfg_data.sr_root_key_program.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified Updates SR root key program')
    else:
        LOG.error('Failed update & verify Updates SR root key program')
        write_file(mode_path, "0")
        return -1


    #Erase/Write/Verify DTS 0x382.0000 - 3FF.FFFF
    LOG.debug('Updates device tree %s from 0x%08x to 0x%08x',fpga_cfg_data.bmc_dts.file,
             fpga_cfg_data.bmc_dts.start, fpga_cfg_data.bmc_dts.end)


    try:
         mtd.erase(mtd_dev_list[1],
                     fpga_cfg_data.bmc_dts.start,
                      (fpga_cfg_data.bmc_dts.end - fpga_cfg_data.bmc_dts.start)+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")


    retval = update_flash_verify(mtd_dev_list[1],
                                fpga_cfg_data.bmc_dts.file,
                                fpga_cfg_data.bmc_dts.start,
                                (fpga_cfg_data.bmc_dts.end - fpga_cfg_data.bmc_dts.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified  device tree')
    else:
        LOG.error('Failed update & verify  device tree')
        write_file(mode_path, "0")
        return -1


    LOG.debug('Updates FPGA image %s from 0x%08x to 0x%08x',fpga_cfg_data.fpga_factory.file,
             fpga_cfg_data.fpga_factory.start, fpga_cfg_data.fpga_factory.end)

    try:
         mtd.erase(mtd_dev_list[1],
                      FPGA_N3000_FPGA_FACTORY_START,
                     (FPGA_N3000_FPGA_FACTORY_END -  FPGA_N3000_FPGA_FACTORY_START)+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    retval = update_flash_verify(mtd_dev_list[1],
                                fpga_cfg_data.fpga_factory.file,
                                fpga_cfg_data.fpga_factory.start,
                                (fpga_cfg_data.fpga_factory.end - fpga_cfg_data.fpga_factory.start) +1 )

    if retval == 0:
        LOG.debug('Successfully update & verified  FPGA Image')
    else:
        LOG.error('Failed update & verify  FPGA Imag')
        write_file(mode_path, "0")
        return -1

    # Disable bmcfw_flash_mode
    write_file(mode_path, "0")


    # Enable bmcimg_flash_mode
    if get_flash_mode(fpga.fme.spi_bus.sysfs_path) == 1:
        LOG.error('fpgaflash on %s is in progress, try later', fpga.fme.spi_bus.sysfs_path)
        return -1

    mode_path = os.path.join(fpga.fme.spi_bus.sysfs_path,
                         'bmcimg_flash_ctrl', 'bmcimg_flash_mode')

    mtd_dev_list = get_mtd_list_from_spi_path(mode_path, fpga.fme.spi_bus.sysfs_path)

    LOG.debug('mode_path:%s',mode_path)
    LOG.debug('spi mtd_dev:%s',mtd_dev_list)

    if len(mtd_dev_list) != 2:
        LOG.error('Invalid flash devices')
        return -1

    mtd_dev = "/dev/mtd1"


    # UFM1 / NIOS Bootloader
    LOG.debug('Updates max10 UFM1 fw %s from 0x%08x to 0x%08x ',fpga_cfg_data.nios_bootloader.file,
              fpga_cfg_data.nios_bootloader.start, fpga_cfg_data.nios_bootloader.end)

    try:
        mtd.erase(mtd_dev,
                       fpga_cfg_data.nios_bootloader.start ,
                       (fpga_cfg_data.nios_bootloader.end - fpga_cfg_data.nios_bootloader.start )+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")


    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.nios_bootloader.file,
                                fpga_cfg_data.nios_bootloader.start ,
                               (fpga_cfg_data.nios_bootloader.end - fpga_cfg_data.nios_bootloader.start )+1)

    if retval == 0:
        LOG.debug('Successfully update & verified max10 UFM1 fw')
    else:
        LOG.error('Failed update & verify Updates max10 UFM1 fw')
        write_file(mode_path, "0")
        return -1


    #CFM1  FACTORY
    LOG.debug('Updates MAX10 CFM1/Factory %s from 0x%08x to 0x%08x ',fpga_cfg_data.max10_factory.file,
              fpga_cfg_data.max10_factory.start, fpga_cfg_data.max10_factory.end)


    try:
        mtd.erase(mtd_dev,
                  FPGA_N3000_CFM1_ERASE_START,
                  (FPGA_N3000_CFM1_ERASE_END - FPGA_N3000_CFM1_ERASE_START)+1)

        mtd.erase(mtd_dev,
                  FPGA_N3000_CFM2_ERASE_START ,
                  (FPGA_N3000_CFM2_ERASE_END - FPGA_N3000_CFM2_ERASE_START )+1)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.max10_factory.file,
                               fpga_cfg_data.max10_factory.start,
                               ( fpga_cfg_data.max10_factory.end - fpga_cfg_data.max10_factory.start ) +1)

    if retval == 0:
        LOG.debug('Successfully update & verified MAX10 factory')
    else:
        LOG.debug('Failed update & verify Updates MAX10 factory')
        write_file(mode_path, "0")
        return -1


    # MAX10 User
    LOG.debug('Updates MAX10 CFM1/User %s from 0x%08x to 0x%08x ',fpga_cfg_data.max10_user.file,
              fpga_cfg_data.max10_user.start, fpga_cfg_data.max10_user.end)



    try:
        mtd.erase(mtd_dev,
                 fpga_cfg_data.max10_user.start,
                 (fpga_cfg_data.max10_user.end - fpga_cfg_data.max10_user.start)+1)

    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.max10_user.file,
                                fpga_cfg_data.max10_user.start,
                                (fpga_cfg_data.max10_user.end - fpga_cfg_data.max10_user.start ) +1)

    if retval == 0:
        LOG.debug('Successfully update & verified max10 CFM1 fw')
    else:
        LOG.error('Failed update & verify Updates max10 CFM1 fw')
        write_file(mode_path, "0")
        return -1


    # Disable bmcimg_flash_mode
    write_file(mode_path, "0")



    # Enable bmcfw_flash_mode
    if get_flash_mode(fpga.fme.spi_bus.sysfs_path) == 1:
        LOG.error('fpgaflash on %s is in progress, try later', fpga.fme.spi_bus.sysfs_path)
        return -1


    mode_path = os.path.join(fpga.fme.spi_bus.sysfs_path,
                         'bmcfw_flash_ctrl', 'bmcfw_flash_mode')

    mtd_dev_list = get_mtd_list_from_spi_path(mode_path, fpga.fme.spi_bus.sysfs_path)

    LOG.debug('mode_path:%s',mode_path)
    LOG.debug('spi mtd_dev:%s',mtd_dev_list)

    if len(mtd_dev_list) != 2:
        LOG.error('Invalid flash devices')
        return -1
 
    mtd_dev = "/dev/mtd1"

    LOG.debug('Erase Flash from 0x%08x to 0x%08x',
             FPGA_N3000_NIOS_USER_ERASE_START,FPGA_N3000_NIOS_USER_ERASE_END)

    try:
        mtd.erase(mtd_dev,
                  FPGA_N3000_NIOS_USER_ERASE_START,
                   FPGA_N3000_NIOS_USER_ERASE_END)
    except IOError:
        write_file(mode_path, "0")
        raise Exception("Failed to Erase Flash")

    #NIOS USER
    LOG.debug('Updates NIOS user  %s from 0x%08x to 0x%08x ',fpga_cfg_data.nios_user.file,
              fpga_cfg_data.nios_user.start, fpga_cfg_data.nios_user.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.nios_user.file,
                                fpga_cfg_data.nios_user.start,
                                (fpga_cfg_data.nios_user.end - fpga_cfg_data.nios_user.start) +1)

    if retval == 0:
        LOG.debug('Successfully update & verified NIOS user')
    else:
        LOG.error('Failed update & verify Updates NIOS user')
        write_file(mode_path, "0")
        return -1

    #NIOS USER Header
    LOG.debug('Updates NIOS user header %s from 0x%08x to 0x%08x',fpga_cfg_data.nios_user_header.file,
         fpga_cfg_data.nios_user_header.start, fpga_cfg_data.nios_user_header.end)

    retval = update_flash_verify(mtd_dev,
                                fpga_cfg_data.nios_user_header.file,
                                fpga_cfg_data.nios_user_header.start,
                               ( fpga_cfg_data.nios_user_header.end - fpga_cfg_data.nios_user_header.start ) +1)

    if retval == 0:
        LOG.debug('Successfully update & verified NIOS user header ')
    else:
        LOG.error('Failed update & verify Updates NIOS user header ')
        write_file(mode_path, "0")
        return -1

     # Disable bmcfw_flash_mode
    write_file(mode_path, "0")


    LOG.debug('---------------- END  -------------------------- ')

    return 0



def do_rsu_fpga(fpga_cfg_data,pci_address):

    LOG.debug('----do_rsu_fpga start-------')
    LOG.debug('pci_address: %s', pci_address)

    # Do RSU all fpga devices
    if(pci_address is None):
        inst = dict({'pci_node.device_id': int(fpga_cfg_data.device,16)})

    if(pci_address is not None):
        inst = dict({'pci_node.pci_address': pci_address})
        LOG.debug('inst=%s',inst)


    for o in fpga.enum([inst]):
        LOG.debug('------FPGA RSU INFO----------')
        LOG.debug('pci_node.pci_address:%s',o.pci_node.pci_address)
        LOG.debug('pci_node.bdf:%s',o.pci_node.bdf)

        if o.pci_node.device_id == D5005_DEV_ID:
            LOG.debug('RSU Not Supported for FPGA D5005')
            return -1

        if o.pci_node.device_id == N3000_DEV_ID:
            LOG.debug('Boot to BMC Start')
            do_rsu('bmcimg',o.pci_node.pci_address,'factory');
            LOG.debug('Boot to BMC END')

            LOG.debug('Boot to FPGA Start')
            do_rsu('fpga',o.pci_node.pci_address,'factory');
            LOG.debug('Boot to FPGA END')


    if(pci_address):
            inst = dict({'pci_node.pci_address': pci_address})

            for o in fpga.enum([inst]):
                LOG.debug('------FPGA RSU INFO----------')
                LOG.debug('pci_node.pci_address:%s',o.pci_node.pci_address)
                LOG.debug('pci_node.bdf:%s',o.pci_node.bdf)
        
            if o.pci_node.device_id == D5005_DEV_ID:
                LOG.debug('RSU Not Supported for FPGA D5005')
                return -1

            if o.pci_node.device_id == N3000_DEV_ID:
                LOG.debug('Boot to BMC Start')
                do_rsu('bmcimg',o.pci_node.pci_address,'factory');
                LOG.debug('Boot to BMC END')

                LOG.debug('Boot to FPGA Start')
                do_rsu('fpga',o.pci_node.pci_address,'factory');
                LOG.debug('Boot to FPGA END')


    return 0

def parse_args():

    """Parses fpgaotsu command line arguments
    """

    parser = argparse.ArgumentParser(
             formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument('path', help='path to fpgaotsu json and flash images \
                       (e.g /usr/share/opae/d5005 or /usr/share/opae/n3000 )' )


    parser.add_argument('--rsu-only', default=False, action='store_true',
                        help='only perform the RSU command')

    rsu_help = "perform remote system update after update"
    rsu_help += " causing the board to be rebooted"
    parser.add_argument('--rsu', default=True, action='store_true',
                        help = rsu_help)

    parser.add_argument('-v', '--verbose', default=False,
                        action='store_true', help='display verbose output')

    return parser.parse_args()


def fpga_update(path, rsu,rsu_only):

    """ Program fpga flash
        Update fpga flash with ROT image
    """

    LOG.debug('path: %s', path)
    LOG.debug('rsu: %s', rsu)
    LOG.debug('rsu_only: %s', rsu_only)

    if not os.path.exists(path):
           LOG.error('Invalid input path:%s',path)

    # Json config file
    json_cfg_path = os.path.join(path, 'fpgaotsu*.json')
    glob_results = glob.glob(json_cfg_path)
    if len(glob_results) != 1:
           LOG.error('No found config json file:%s',path)
           sys.exit(1)

    LOG.debug('Found config json file:%s',glob_results[0])

    # Parse input config json file
    fpga_cfg_instance = fpga_cfg_data.parse_json_cfg(glob_results[0],path)

    if fpga_cfg_instance is None:
        LOG.error('Invalid Input json config:',glob_results[0])
        return -1

    fpga_cfg_instance.print_json_cfg()

    # Check for valid program files
    if not fpga_cfg_instance.check_fpga_images():
        LOG.error('Invalid image files')
        return -1;


    LOG.debug('Number of FPGA devices: %d', \
            len(fpga.enum([{'pci_node.device_id': int(fpga_cfg_instance.device,16)}])))


    if len(fpga.enum([{'pci_node.device_id': int(fpga_cfg_instance.device,16)}])) == 0:
        LOG.error('Invalid Input config device id:%s',fpga_cfg_instance.device)
        return -1


    inst = dict({'pci_node.device_id': int(fpga_cfg_instance.device,16)})

    if(rsu_only):
        retval = do_rsu_fpga(fpga_cfg_instance,None)
        return retval

    for o in fpga.enum([inst]):
        LOG.debug('------FPGA INFO----------')
        LOG.debug('pci_node:%s',o.pci_node)
        LOG.debug('pci_node.root.tree:%s',o.pci_node.root.tree())
        LOG.debug('fme.devpath:%s',o.fme.devpath)
        LOG.debug('fme.devpath:%s',o.fme.devpath)
        LOG.debug('fme.spi_bus.sysfs_path:%s',o.fme.spi_bus.sysfs_path)
        LOG.debug('port.devpath:%s',o.port.devpath)
        LOG.debug('pci_node.pci_address:%s',o.pci_node.pci_address)
        LOG.debug('pci_node.bdf:%s',o.pci_node.bdf)

        # D5005
        if o.pci_node.device_id == D5005_DEV_ID:
            LOG.debug('Found FPGA D5005 Card')

            try:
                retval = d5005_fpga_update(o,fpga_cfg_instance)
                if retval == 0:
                    LOG.debug('Successfully update & verified D5005 FPGA')
                else:
                    LOG.error('Failed to update D5005 FPGA ')
                    continue

            except Exception as e:
                LOG.debug('Failed to update D5005 FPGA')
                LOG.debug(e.message,e.args)
                continue

            try:
                retval = do_rsu_fpga(fpga_cfg_instance,o.pci_node.pci_address)
                if retval == 0:
                    LOG.debug('Successfully Done RSU')
                else:
                    LOG.error('Failed to do RSU ')

            except Exception as e:
                LOG.debug('Failed to do RSU')
                LOG.debug(e.message,e.args)



        #N3000
        if o.pci_node.device_id == N3000_DEV_ID:
            LOG.debug('Found FPGA FPGA N3000 Card')

            try:
                retval = n3000_fpga_update(o,fpga_cfg_instance)
                if retval == 0:
                    LOG.debug('Successfully update & verified N3000 FPGA')
                else:
                    LOG.error('Failed to update N3000 FPGA ')
                    continue

            except Exception as e:
                LOG.debug('Failed to update N3000 FPGA')
                LOG.debug(e.message,e.args)
                continue

            # RSU
            try:
                retval = do_rsu_fpga(fpga_cfg_instance,o.pci_node.pci_address)
                if retval == 0:
                    LOG.debug('Successfully Done RSU')
                else:
                    LOG.error('Failed to do RSU ')

            except Exception as e:
                LOG.debug('Failed to do RSU')
                LOG.debug(e.message,e.args)


    return 0


# main
def main():

    # Logger
    LOG.setLevel(logging.NOTSET)
    log_fmt = ('[%(asctime)-15s] [%(levelname)-8s] ' '%(message)s')
    log_hndlr = logging.StreamHandler(sys.stdout)
    log_hndlr.setFormatter(logging.Formatter(log_fmt))
    log_hndlr.setLevel(logging.DEBUG)
    LOG.addHandler(log_hndlr)

    LOG.debug('Input arguments count: %d', len(sys.argv))


    # Command line args parse
    if ( len(sys.argv) >= 2 ):

        args = parse_args()
        LOG.debug('fpgaotsu Command line arguments')
        LOG.debug('%s',args)

        if args.verbose:
           log_hndlr.setLevel(logging.DEBUG)
        else:
           log_hndlr.setLevel(logging.DEBUG)

        retval = fpga_update(args.path,args.rsu,args.rsu_only)
        if( retval == 0 ):
            sys.exit(0)
        else:
            sys.exit(1)


    else:
        LOG.debug('No input command line args')

        log_hndlr.setLevel(logging.DEBUG)

        json_cfg_path = os.path.join(OPAE_SHARE, "fpgaotsu*.json")

        glob_results = glob.glob(json_cfg_path)
        if len(glob_results) != 1:
            LOG.error('No found config json file:%s',OPAE_SHARE)
            sys.exit(1)

        LOG.debug('Found config json file:%s',glob_results[0])

        path = os.path.dirname(glob_results[0])

        retval = fpga_update(path,args.rsu,None)
        if( retval == 0 ):
            sys.exit(0)
        else:
            sys.exit(1)

    # Exit
    sys.exit(0)



if __name__ == "__main__":
    main()
