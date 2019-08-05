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
import signal
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
FPGA_FLASH_BLOCK_SIZE = 4096

# FPGA device ID
D5005_DEV_ID = 0x0b2b
N3000_DEV_ID = 0x0b30

# Json config file & Binary path
OPAE_SHARE = '/usr/share/opae/*/one-time-update'

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
        position = 0
        last_progress = -1
        file_size = nbytes
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
                position = ofile.tell()

                # print read progress
                progress = int(((position) / float(file_size)) * 100.00)
                LOG.debug('Read bytes:0x%08x  progress: %3.0f%%',position,progress)
                if progress >last_progress:
                   last_progress = progress
                   LOG.info('Read bytes:0x%08x progress: %3.0f%%',position,progress)

                nbytes -= rbytes

    @staticmethod
    def write(dev, start, nbytes, ifile):
        LOG.debug('writing 0x%08x bytes to 0x%08x',nbytes, start)
        last_write_position = start
        position = 0
        last_progress = -1
        with os.fdopen(os.open(dev, os.O_SYNC | os.O_RDWR), 'a') as file_:
            file_size = os.path.getsize(ifile.name)
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

                # print Write progress
                position = file_.tell()
                progress = int(((position - start) / float(file_size)) * 100.00)
                LOG.debug('Wrote bytes:0x%08x  progress: %3.0f%%',position - start,progress)
                if progress >last_progress:
                   last_progress = progress
                   LOG.info('Wrote bytes:0x%08x progress: %3.0f%%',position - start,progress)

                nbytes -= rbytes

        bytes_written = last_write_position - start
        LOG.debug('actual bytes written 0x%x - 0x%x = 0x%x',last_write_position, start, bytes_written)

        return bytes_written

    @staticmethod
    def update_verify(dev, filename, intput_offset, input_len):
        with open(filename, 'rb') as fd:
            file_size = os.path.getsize(fd.name)
            if file_size > input_len:
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

# end of MTD

class flash_data(object):
    def __init__(self,data,path):
        self.filename = data.get('filename',None)
        self.file = os.path.join(path, data.get('filename',None))
        self.type = data.get('type',None)
        self.start = int(data.get('start',0),16)
        self.end = int(data.get('end',0),16)

    def __str__(self):
        return 'type: {:25s}  filename: {:60s} start: {:0x}  end: {:0x} '.format(
            self.type, self.filename, self.start, self.end)

    @classmethod
    def print_data(self):
        LOG.debug('file :%s', self.filename)
        LOG.debug('start: 0x%x ', self.start)
        LOG.debug('end: 0x%x ', self.start)

# fogaotsu_cfg_data

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
        self.fpga_user = flash_data()
        self.nios_bootloader = flash_data()
        self.max10_factory = flash_data()
        self.max10_user = flash_data()
        self.nios_user_header = flash_data()
        self.nios_user = flash_data()

    @classmethod
    def print_json_cfg(self):
        LOG.info('\n\n')
        LOG.info('FPGA JSON CONFIG DATA')
        LOG.info('product:%s',self.product)
        LOG.info('vendor:%s',self.vendor)
        LOG.info('device:%s',self.device)
        LOG.info(str(self.bmc_root_key_hash))
        LOG.info(str(self.bmc_root_key_program))
        LOG.info(str(self.sr_root_key_hash))
        LOG.info(str(self.sr_root_key_program))
        LOG.info(str(self.bmc_dts))
        LOG.info(str(self.nios_factory_header))
        LOG.info(str(self.nios_factory))
        LOG.info(str(self.fpga_user))
        LOG.info(str(self.nios_bootloader))
        LOG.info(str(self.max10_factory))
        LOG.info(str(self.max10_user))
        LOG.info(str(self.nios_user_header))
        LOG.info(str(self.nios_user))
        LOG.info('\n \n ')

    @classmethod
    def check_fpga_images(self):
        if(self.bmc_root_key_hash.file is not None and 
            not os.path.isfile(self.bmc_root_key_hash.file)):
            LOG.error('file does not exist:%s',self.bmc_root_key_hash.file)
            return False

        if(self.bmc_root_key_program.file is not None and 
            not os.path.isfile(self.bmc_root_key_program.file)):
            LOG.error('file does not exist:%s',self.bmc_root_key_program.file)
            return False

        if(self.sr_root_key_hash.file is not None and 
            not os.path.isfile(self.sr_root_key_hash.file)):
            LOG.error('file does not exist:%s',self.sr_root_key_hash.file)
            return False

        if(self.sr_root_key_program.file is not None and 
            not os.path.isfile(self.sr_root_key_program.file)):
            LOG.error('file does not exist:%s',self.sr_root_key_program.file)
            return False

        if(self.bmc_dts.file is not None and 
            not os.path.isfile(self.bmc_dts.file)):
            LOG.error('file does not exist:%s',self.bmc_dts.file)
            return False

        if(self.nios_factory_header.file is not None and 
            not os.path.isfile(self.nios_factory_header.file)):
            LOG.error('file does not exist:%s',self.nios_factory_header.file)
            return False

        if(self.fpga_factory.file is not None and 
            not os.path.isfile(self.fpga_factory.file)):
            LOG.error('file does not exist:%s',self.fpga_factory.file)
            return False

        if(self.nios_bootloader.file is not None and 
            not os.path.isfile(self.nios_bootloader.file)):
            LOG.error('file does not exist:%s',self.nios_bootloader.file)
            return False

        if(self.max10_factory.file is not None and 
            not os.path.isfile(self.max10_factory.file)):
            LOG.error('file does not exist:%s',self.max10_factory.file)
            return False

        if(self.max10_user.file is not None and 
            not os.path.isfile(self.max10_user.file)):
            LOG.error('file does not exist:%s',self.max10_user.file)
            return False

        if(self.nios_user_header.file is not None and 
            not os.path.isfile(self.nios_user_header.file)):
            LOG.error('file does not exist:%s',self.nios_user_header.file)
            return False

        if(self.nios_user.file is not None and 
            not os.path.isfile(self.nios_user.file)):
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

                if(fd.get('type') =='fpga_user'):
                   o.fpga_user=flash_data(fd,path_bin)

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

# class fpgaotsu
class fpgaotsu(object):
    def __init__(self, fpga,fpga_cfg_data):
        self._fpga = fpga
        self._fpga_cfg_data = fpga_cfg_data

    # get flash mode
    def get_flash_mode(self):
        modes = []
        for prefix in ['fpga', 'bmcfw', 'bmcimg']:
            path = os.path.join(self._fpga.fme.spi_bus.sysfs_path, '{}_flash_ctrl'.format(prefix),
                                '{}_flash_mode'.format(prefix))
            with open(path, 'r') as fd:
                modes.append(int(fd.read(1)))

        # return 1 if any type of flash is in use
        return 1 if any(modes) else 0

    # write to sysfs
    def write_file(self,fname, data):
        with open(fname, 'wb') as wfile:
            wfile.write(data)

    # RoT is update status
    def RoT_is_updated(self):
        sec_dev = self._fpga.secure_dev
        if sec_dev:
            LOG.info('FPGA Firmware updated to secure')
            return True

        LOG.debug('FPGA Firmware Not Updated to secure')
        return False

    # diasble mtd
    def set_mtd(self,ctrl, mode):
        mode_path = os.path.join(self._fpga.fme.spi_bus.sysfs_path,
                                 ctrl, mode)
        self.write_file(mode_path, "0")

    # enable mtd
    def get_mtd(self,ctrl, mode):
        mode_path = os.path.join(self._fpga.fme.spi_bus.sysfs_path,
                                 ctrl, mode)
        self.write_file(mode_path, "1")
        LOG.debug('mode_path=%s',mode_path)

        for i in range(20):
            time.sleep(1)
            LOG.debug('self._fpga.fme.spi_bus.sysfs_path=%s',self._fpga.fme.spi_bus.sysfs_path)
            mtds = glob.glob(os.path.join(self._fpga.fme.spi_bus.sysfs_path, '*.*.auto',
                                            'mtd', 'mtd*'))
            for mtd in mtds:
                if not fnmatch.fnmatchcase(mtd, "*ro"):
                    mtd_dev = os.path.join('/dev', os.path.basename(mtd))
                    LOG.debug('using %s',mtd_dev)
                    return mtd_dev

        raise Exception("no mtd node found for mode %s" % (mode_path))

    # get list of mtd devices
    def get_mtd_list(self,ctrl, mode):
        mode_path = os.path.join(self._fpga.fme.spi_bus.sysfs_path, ctrl, mode)
        self.write_file(mode_path, "1")
        mtd_list = list();

        for i in range(20):
            time.sleep(1)
            mtds = glob.glob(os.path.join(self._fpga.fme.spi_bus.sysfs_path, '*.*.auto',
                                          'mtd', 'mtd*'))
            for mtd in mtds:
                 if not fnmatch.fnmatchcase(mtd, "*ro"):
                      mtd_dev = os.path.join('/dev', os.path.basename(mtd))
                      mtd_list.append(mtd_dev)
                      if len(mtds)/2 == len(mtd_list) :
                           return mtd_list

        raise Exception("no mtd node found for mode %s" % (mode_path))

    # get mtd from path
    def get_mtd_from_path(self,path):
        mtd_path = os.path.join(self._fpga.fme.spi_bus.sysfs_path,path)
        for i in range(20):
            time.sleep(1)
            mtds = glob.glob(os.path.join(mtd_path,
                                          'mtd', 'mtd*'))
            for mtd in mtds:
                if not fnmatch.fnmatchcase(mtd, "*ro"):
                    mtd_dev = os.path.join('/dev', os.path.basename(mtd))
                    LOG.debug('using %s',mtd_dev)
                    return mtd_dev

        raise Exception("no mtd node found for mode %s" % (mode_path))

# end class fpgaotsu

# class fpgaotsu_d5005
class fpgaotsu_d5005(fpgaotsu):
    device_id = 0x0b2b
    fpga_erase_start = 0x7800000
    fpga_erase_end = 0xfffffff
    nios_erase_start = 0x0
    nios_erase_end = 0x800000
    fpga_factory_erase_start = 0x20000
    fpga_factory_erase_end = 0x381FFFF
    cfm1_erase_start = 0x70000
    cfm1_erase_end = 0xB7FFF
    cfm2_erase_start = 0x10000
    cfm2_erase_end = 0x6FFFF

    def __init__(self,fpga,fpga_cfg_data):
        fpgaotsu.__init__(self,fpga,fpga_cfg_data)

    # nios user update
    def d5005_nios_user_update(self):
        try:
            if super(fpgaotsu_d5005,self).get_flash_mode() ==1:
                LOG.error('fpgaflash on %s is in progress, try later', self._fpga.fme.spi_bus.sysfs_path)
                return false

            # Enable bmcfw_flash_mode
            mtd_dev = super(fpgaotsu_d5005,self).get_mtd('bmcfw_flash_ctrl', 'bmcfw_flash_mode')
            LOG.debug('spi mtd_dev:%s',mtd_dev)

            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     fpgaotsu_d5005.nios_erase_start,fpgaotsu_d5005.nios_erase_end)
            mtd.erase(mtd_dev,
                        fpgaotsu_d5005.nios_erase_start,
                        fpgaotsu_d5005.nios_erase_end)

            #Nios user
            LOG.info('Updates Nios user  %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.nios_user.file,
                      self._fpga_cfg_data.nios_user.start, self._fpga_cfg_data.nios_user.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.nios_user.file,
                                        self._fpga_cfg_data.nios_user.start,
                                        (self._fpga_cfg_data.nios_user.end - self._fpga_cfg_data.nios_user.start)+1)
            if retval == 0:
                LOG.info('Successfully update & verified Nios user')
            else:
                LOG.exception('Failed update & verify Nios user')
                raise Exception("Failed update & verify Nios user")

            #Nios user header
            LOG.info('Updates Nios user header %s from 0x%08x to 0x%08x',self._fpga_cfg_data.nios_user_header.file,
                 self._fpga_cfg_data.nios_user_header.start, self._fpga_cfg_data.nios_user_header.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.nios_user_header.file,
                                        self._fpga_cfg_data.nios_user_header.start,
                                        (self._fpga_cfg_data.nios_user_header.end - self._fpga_cfg_data.nios_user_header.start)+1)
            if retval == 0:
                LOG.info('Successfully update & verified Nios user header')
            else:
                LOG.exception('Failed update & verify Nios user header')
                raise Exception("Failed update & verify Nios user header")

        except Exception as e:
            LOG.exception('Failed to update D5005 FPGA')
            LOG.exception(e.message,e.args)
            retval = -1

        finally:
            super(fpgaotsu_d5005,self).set_mtd('bmcfw_flash_ctrl', 'bmcfw_flash_mode')

        return retval

    # fpga user update
    def d5005_fpga_user(self):
        try:
            if super(fpgaotsu_d5005,self).get_flash_mode() ==1:
                LOG.error('fpgaflash on %s is in progress, try later', self._fpga.fme.spi_bus.sysfs_path)
                return -1

            # Enable fpga_flash_mode
            mtd_dev = super(fpgaotsu_d5005,self).get_mtd('fpga_flash_ctrl', 'fpga_flash_mode')
            LOG.debug('spi mtd_dev:%s',mtd_dev)

            #Erase/Write/Verify FPGA user iamge
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     self._fpga_cfg_data.fpga_user.start,self._fpga_cfg_data.fpga_user.end)

            mtd.erase(mtd_dev,
                        self._fpga_cfg_data.fpga_user.start,
                        (self._fpga_cfg_data.fpga_user.end - self._fpga_cfg_data.fpga_user.start)+1)

            LOG.info('Updates FPGA user image %s from 0x%08x to 0x%08x',self._fpga_cfg_data.fpga_user.file,
                     self._fpga_cfg_data.fpga_user.start, self._fpga_cfg_data.fpga_user.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.fpga_user.file,
                                        self._fpga_cfg_data.fpga_user.start,
                                        (self._fpga_cfg_data.fpga_user.end - self._fpga_cfg_data.fpga_user.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified  FPGA user Image')
            else:
                LOG.exception('Failed update & verify  FPGA user Image')
                raise Exception("Failed update & verify  FPGA user Image")

        except Exception as e:
            LOG.exception('Failed to update D5005 FPGA')
            LOG.exception(e.message,e.args)
            retval = -1
        finally:
            super(fpgaotsu_d5005,self).set_mtd('fpga_flash_ctrl', 'fpga_flash_mode')

        return retval

    # max10 update
    def d5005_max10_update(self):
        try:
            if super(fpgaotsu_d5005,self).get_flash_mode() ==1:
                LOG.error('fpgaflash on %s is in progress, try later', self._fpga.fme.spi_bus.sysfs_path)
                return false

            # Enable bmcimg_flash_mode
            mtd_dev = super(fpgaotsu_d5005,self).get_mtd('bmcimg_flash_ctrl', 'bmcimg_flash_mode')
            LOG.debug('spi mtd_dev:%s',mtd_dev)

            # UFM1 / NIOS Bootloader
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     self._fpga_cfg_data.max10_factory.start,self._fpga_cfg_data.nios_bootloader.end)
            mtd.erase(mtd_dev,
                        self._fpga_cfg_data.nios_bootloader.start,
                        (self._fpga_cfg_data.nios_bootloader.end - self._fpga_cfg_data.nios_bootloader.start )+1)

            LOG.info('Updates max10 UFM1 %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.nios_bootloader.file,
                      self._fpga_cfg_data.nios_bootloader.start, self._fpga_cfg_data.nios_bootloader.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.nios_bootloader.file,
                                        self._fpga_cfg_data.nios_bootloader.start,
                                        (self._fpga_cfg_data.nios_bootloader.end - self._fpga_cfg_data.nios_bootloader.start )+1)
            if retval == 0:
                LOG.info('Successfully update & verified max10 UFM1')
            else:
                LOG.exception('Failed update & verify Updates max10 UFM1')
                raise Exception("Failed update & verify Updates max10 UFM1'")

            # MAX10 FACTORY /CFM0
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     self._fpga_cfg_data.max10_factory.start,self._fpga_cfg_data.max10_factory.end)
            mtd.erase(mtd_dev,
                      self._fpga_cfg_data.max10_factory.start ,
                      (self._fpga_cfg_data.max10_factory.end - self._fpga_cfg_data.max10_factory.start)+1)

            LOG.info('Updates MAX10 factory /CFM0 %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.max10_factory.file,
                      self._fpga_cfg_data.max10_factory.start, self._fpga_cfg_data.max10_factory.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.max10_factory.file,
                                        self._fpga_cfg_data.max10_factory.start,
                                        (self._fpga_cfg_data.max10_factory.end - fpga_cfg_data.max10_factory.start)+1)
            if retval == 0:
                LOG.info('Successfully update & verified MAX10 factory')
            else:
                LOG.exception('Failed update & verify MAX10 factory')
                raise Exception("Failed update & verify MAX10 factory")

            #MAX10 user / CFM1
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     fpgaotsu_d5005.cfm2_erase_start,fpgaotsu_d5005.cfm1_erase_end)

            mtd.erase(mtd_dev,
                        fpgaotsu_d5005.cfm1_erase_start,
                        (fpgaotsu_d5005.cfm1_erase_end -fpgaotsu_d5005.cfm1_erase_start)+1)

            mtd.erase(mtd_dev,
                        fpgaotsu_d5005.cfm2_erase_start ,
                        (fpgaotsu_d5005.cfm2_erase_end - fpgaotsu_d5005.cfm2_erase_start  )+1)

            LOG.info('Updates MAX10 user /CFM1 %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.max10_user.file,
                      self._fpga_cfg_data.max10_user.start, fpga_cfg_data.max10_user.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.max10_user.file,
                                        self._fpga_cfg_data.max10_user.start,
                                       (self._fpga_cfg_data.max10_user.end - self._fpga_cfg_data.max10_user.start)+1)

            if retval == 0:
                LOG.info('Successfully update & verified max10 user')
            else:
                LOG.exception('Failed update & verify max10 user')
                raise Exception("Failed update & verify max10 user")

        except Exception as e:
            LOG.exception('Failed to update D5005 FPGA')
            LOG.exception(e.message,e.args)
            retval = -1

        finally:
            super(fpgaotsu_d5005,self).set_mtd('bmcimg_flash_ctrl', 'bmcimg_flash_mode')

        return retval

    # fpga nios factory/ keys/ dtb / fpga image update
    def d5005_fpga_flash_update(self):
        try:
            if super(fpgaotsu_d5005,self).get_flash_mode() ==1:
                LOG.error('fpgaflash on %s is in progress, try later', self._fpga.fme.spi_bus.sysfs_path)
                return -1

            # Enable fpga_flash_mode
            mtd_dev = super(fpgaotsu_d5005,self).get_mtd('fpga_flash_ctrl', 'fpga_flash_mode')
            LOG.debug('spi mtd_dev:%s',mtd_dev)

            # FLASH SIZE 
            ret_size = mtd.size(mtd_dev)
            LOG.debug('Flash size:0x%08x',ret_size)

            # Erase from 0x780.0000 to 0xfff.ffff
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     fpgaotsu_d5005.fpga_erase_start,fpgaotsu_d5005.fpga_erase_end)
            mtd.erase(mtd_dev,
                            fpgaotsu_d5005.fpga_erase_start ,
                            (fpgaotsu_d5005.fpga_erase_end -fpgaotsu_d5005.fpga_erase_start)+1 )

            # NIOS factory
            LOG.info('Updates NIOS factory  %s from 0x%08x to 0x%08x',self._fpga_cfg_data.nios_factory.file,
                    self._fpga_cfg_data.nios_factory.start, self._fpga_cfg_data.nios_factory.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.nios_factory.file,
                                        self._fpga_cfg_data.nios_factory.start,
                                        (self._fpga_cfg_data.nios_factory.end - self._fpga_cfg_data.nios_factory.start) +1 )
            if retval == 0:
                  LOG.info('Successfully update & verified NIOS factory')
            else:
                LOG.exception('Failed update & verify NIOS factory')
                raise Exception("Failed update & verify NIOS factory")

            #NIOS Header
            LOG.info('Updates NIOS factory header %s from 0x%08x to 0x%08x',self._fpga_cfg_data.nios_factory_header.file,
                    self._fpga_cfg_data.nios_factory_header.start, self._fpga_cfg_data.nios_factory_header.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.nios_factory_header.file,
                                        self._fpga_cfg_data.nios_factory_header.start,
                                        ( self._fpga_cfg_data.nios_factory_header.end- self._fpga_cfg_data.nios_factory_header.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified NIOS factory header')
            else:
                LOG.exception('Failed update & verify NIOS factory header')
                raise Exception("Failed update & verify NIOS factory header")

            #BMC ROOT KEY HASH
            LOG.info('Updates bmc root key hash %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.bmc_root_key_hash.file,
                    self._fpga_cfg_data.bmc_root_key_hash.start, self._fpga_cfg_data.bmc_root_key_hash.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.bmc_root_key_hash.file,
                                        self._fpga_cfg_data.bmc_root_key_hash.start,
                                        (self._fpga_cfg_data.bmc_root_key_hash.end - self._fpga_cfg_data.bmc_root_key_hash.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified bmc key hash')
            else:
                LOG.exception('Failed update & verify bmc key hash')
                raise Exception("Failed update & verify bmc key hash")

            LOG.info('Updates bmc root key program %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.bmc_root_key_program.file,
                    self._fpga_cfg_data.bmc_root_key_program.start, self._fpga_cfg_data.sr_root_key_program.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.bmc_root_key_program.file,
                                        self._fpga_cfg_data.bmc_root_key_program.start,
                                        (self._fpga_cfg_data.sr_root_key_program.end - self._fpga_cfg_data.bmc_root_key_program.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified bmc key program')
            else:
                LOG.error('Failed update & verify bmc key program')
                raise Exception("Failed update & verify bmc key program")

            #SR ROOT KEY HASH
            LOG.info('Updates SR root key hash %s from 0x%08x to 0x%08x ', self._fpga_cfg_data.sr_root_key_hash.file,
                     self._fpga_cfg_data.sr_root_key_hash.start, self._fpga_cfg_data.sr_root_key_hash.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.sr_root_key_hash.file,
                                        self._fpga_cfg_data.sr_root_key_hash.start,
                                        (self._fpga_cfg_data.sr_root_key_hash.end - self._fpga_cfg_data.sr_root_key_hash.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified Updates SR root key hash')
            else:
                LOG.exception('Failed update & verify Updates SR root key hash')
                raise Exception("Failed update & verify bmc key program")

            LOG.info('Updates SR root key program %s from 0x%08x to 0x%08x ', self._fpga_cfg_data.sr_root_key_program.file,
                     self._fpga_cfg_data.sr_root_key_program.start, self._fpga_cfg_data.sr_root_key_program.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.sr_root_key_program.file,
                                        self._fpga_cfg_data.sr_root_key_program.start,
                                        (self._fpga_cfg_data.sr_root_key_program.end - self._fpga_cfg_data.sr_root_key_program.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified Updates SR root key program')
            else:
                LOG.error('Failed update & verify Updates SR root key program')
                raise Exception("Failed update & verify bmc key program")

            #Erase/Write/Verify DTS 0x382.0000 - 3FF.FFFF
            LOG.info('Updates device tree %s from 0x%08x to 0x%08x',self._fpga_cfg_data.bmc_dts.file,
                     self._fpga_cfg_data.bmc_dts.start, self._fpga_cfg_data.bmc_dts.end)

            mtd.erase(mtd_dev,
                    self._fpga_cfg_data.bmc_dts.start,
                    (self._fpga_cfg_data.bmc_dts.end - self._fpga_cfg_data.bmc_dts.start)+1)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.bmc_dts.file,
                                        self._fpga_cfg_data.bmc_dts.start,
                                        (self._fpga_cfg_data.bmc_dts.end - self._fpga_cfg_data.bmc_dts.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified  device tree')
            else:
                LOG.exception('Failed update & verify  device tree')
                raise Exception("Failed update & verify  device tree")

            #Erase/Write/Verify FPGA image
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     fpgaotsu_d5005.fpga_factory_erase_start,fpgaotsu_d5005.fpga_factory_erase_end)

            mtd.erase(mtd_dev,
                        fpgaotsu_d5005.fpga_factory_erase_start,
                        (fpgaotsu_d5005.fpga_factory_erase_end - fpgaotsu_d5005.fpga_factory_erase_start)+1)

            LOG.info('Updates FPGA image %s from 0x%08x to 0x%08x',self._fpga_cfg_data.fpga_factory.file,
                     self._fpga_cfg_data.fpga_factory.start, self._fpga_cfg_data.fpga_factory.end)

            retval = mtd.update_verify(mtd_dev,
                                        self._fpga_cfg_data.fpga_factory.file,
                                        self._fpga_cfg_data.fpga_factory.start,
                                        (self._fpga_cfg_data.fpga_factory.end - self._fpga_cfg_data.fpga_factory.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified  FPGA Image')
            else:
                LOG.exception('Failed update & verify  FPGA Image')
                raise Exception("Failed update & verify  FPGA Image")

        except Exception as e:
            LOG.exception('Failed to update D5005 FPGA')
            LOG.exception(e.message,e.args)
            retval = -1
        finally:
            super(fpgaotsu_d5005,self).set_mtd('fpga_flash_ctrl', 'fpga_flash_mode')

        return retval


    def d5005_fpga_update(self):
        if super(fpgaotsu_d5005,self).RoT_is_updated():
            LOG.info('FPGA Firmware updated to secure')
            return 0 

        retval = self.d5005_fpga_flash_update()
        if retval == 0:
            LOG.debug('Successfully updated FPGA flash')
        else:
            LOG.error('Failed to update FPGA flash')
            return retval

        retval = self.d5005_max10_update()
        if retval == 0:
            LOG.debug('Successfully updated Max10')
        else:
            LOG.error('Failed to update max10')
            return retval

        retval = self.d5005_nios_user_update()
        if retval == 0:
            LOG.debug('Successfully updated Nios user')
        else:
            LOG.error('Failed to update NIOS user')
            return retval

        retval = self.d5005_fpga_user()
        if retval == 0:
            LOG.debug('Successfully updated FPGA user')
        else:
            LOG.error('Failed to update FPGA user')
            return retval

        return retval
# end class fpgaotsu_d5005

# start class fpgaotsu_n3000
class fpgaotsu_n3000(fpgaotsu):
    def __init__(self, fpga,fpga_cfg_data):
        fpgaotsu.__init__(self, fpga,fpga_cfg_data)
        self._second_mtd_dev = None

    device_id = 0x0b30
    fpga_erase_flash1_start = 0x7800000
    fpga_erase_flash1_end = 0x7FFFFFF
    fpga_erase_flash2_start = 0x0
    fpga_erase_flash2_end = 0x8000000
    fpga_factory_erase_start = 0x20000
    fpga_factory_erase_end = 0x381FFFF
    cfm1_erase_start = 0x70000
    cfm1_erase_end = 0xB7FFF
    cfm2_erase_start = 0x10000
    cfm2_erase_end = 0x6FFFF
    nios_user_erase_start = 0x0
    nios_user_erase_end = 0x800000

    # temporary image update or not
    def check_temporary_max10(slef,bdf):
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

    # RSU support
    def do_rsu_only(self,pci_address):
        LOG.info('pci_address: %s', pci_address)
        if(pci_address is None):
            inst = dict({'pci_node.device_id': int(self._fpga_cfg_data.device,16)})

        if(pci_address):
            inst = dict({'pci_node.pci_address': pci_address})
            LOG.debug('inst=%s',inst)

        for o in fpga.enum([inst]):
            LOG.debug('------FPGA RSU INFO----------')
            LOG.debug('pci_node.pci_address:%s',o.pci_node.pci_address)
            LOG.debug('pci_node.bdf:%s',o.pci_node.bdf)
            LOG.info('pci_node:%s',o.pci_node)

            if o.pci_node.device_id == N3000_DEV_ID:
                LOG.info('RSU BMC Start')
                do_rsu('bmcimg',o.pci_node.pci_address,'factory');
                LOG.info('RSU BMC END')

                LOG.info('RSU FPGA Start')
                do_rsu('fpga',o.pci_node.pci_address,'factory');
                LOG.info('RSU FPGA END')

        return 0

    # fpga user update
    def n3000_fpga_user(self):
        try:
            if super(fpgaotsu_n3000,self).get_flash_mode() ==1:
                LOG.error('fpgaflash on %s is in progress, try later', self._fpga.fme.spi_bus.sysfs_path)
                return false

            mtd_dev_list = super(fpgaotsu_n3000,self).get_mtd_list('fpga_flash_ctrl', 'fpga_flash_mode')
            LOG.debug('mtd_dev_list:%s',mtd_dev_list)

            if len(mtd_dev_list) != 2:
                LOG.error('Invalid flash devices')
                return -1

            for i in range(len(mtd_dev_list)):
                if mtd_dev_list[i] != self._second_mtd_dev:
                    first_mtd_dev = mtd_dev_list[i]
            LOG.info('first_mtd_dev:%s',first_mtd_dev)

            #Erase/Write/Verify FPGA user iamge
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     self._fpga_cfg_data.fpga_user.start,self._fpga_cfg_data.fpga_user.end)

            mtd.erase(first_mtd_dev,
                        self._fpga_cfg_data.fpga_user.start,
                        (self._fpga_cfg_data.fpga_user.end - self._fpga_cfg_data.fpga_user.start)+1)

            LOG.info('Updates FPGA user image %s from 0x%08x to 0x%08x',self._fpga_cfg_data.fpga_user.file,
                     self._fpga_cfg_data.fpga_user.start, self._fpga_cfg_data.fpga_user.end)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.fpga_user.file,
                                        self._fpga_cfg_data.fpga_user.start,
                                        (self._fpga_cfg_data.fpga_user.end - self._fpga_cfg_data.fpga_user.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified  FPGA user Image')
            else:
                LOG.exception('Failed update & verify  FPGA user Image')
                raise Exception("Failed update & verify  FPGA user Image")

        except Exception as e:
            LOG.exception('Failed to update D5005 FPGA')
            LOG.exception(e.message,e.args)
            retval = -1
        finally:
            super(fpgaotsu_n3000,self).set_mtd('fpga_flash_ctrl', 'fpga_flash_mode')

        return retval

    # update second flash with nios
    def n3000_second_flash_update(self):
        try:
            self._second_mtd_dev = super(fpgaotsu_n3000,self).get_mtd_from_path('intel-generic-qspi*.*.auto')
            LOG.debug('second_mtd_dev:%s',self._second_mtd_dev)

            if super(fpgaotsu_n3000,self).get_flash_mode() ==1:
                LOG.error('fpgaflash on %s is in progress, try later', self._fpga.fme.spi_bus.sysfs_path)
                return false

            mtd_dev_list = super(fpgaotsu_n3000,self).get_mtd_list('fpga_flash_ctrl', 'fpga_flash_mode')
            LOG.debug('mtd_dev_list:%s',mtd_dev_list)

            if len(mtd_dev_list) != 2:
                LOG.error('Invalid flash devices')
                return -1

            # FLASH SIZE 
            ret_size = mtd.size(self._second_mtd_dev)
            LOG.info('second_mtd_dev:%s',self._second_mtd_dev)
            LOG.debug('Flash size:0x%08x',ret_size)

            # Second FLASH
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                    fpgaotsu_n3000.fpga_erase_flash2_start,fpgaotsu_n3000.fpga_erase_flash2_end)

            mtd.erase(self._second_mtd_dev,
                        fpgaotsu_n3000.fpga_erase_flash2_start,
                        fpgaotsu_n3000.fpga_erase_flash2_end)

            # Write/Verify NIOS factory firmware to 0x3800000  0x3a00fff of Second FPGA flash
            LOG.info('Updates NIOS factory  %s from 0x%08x to 0x%08x',self._fpga_cfg_data.nios_factory.file,
                    self._fpga_cfg_data.nios_factory.start, self._fpga_cfg_data.nios_factory.end)

            retval = mtd.update_verify(self._second_mtd_dev,
                                        self._fpga_cfg_data.nios_factory.file,
                                        self._fpga_cfg_data.nios_factory.start ,
                                        (self._fpga_cfg_data.nios_factory.end - self._fpga_cfg_data.nios_factory.start)+1 )

            if retval == 0:
                    LOG.info('Successfully update & verified NIOS factory')
            else:
                LOG.exception('Failed update & verify NIOS factory')
                raise Exception("Failed update & verify NIOS factory")

            #NIOS Header
            LOG.info('Updates NIOS factory header %s from 0x%08x to 0x%08x',self._fpga_cfg_data.nios_factory_header.file,
                    self._fpga_cfg_data.nios_factory_header.start, self._fpga_cfg_data.nios_factory_header.end)

            retval = mtd.update_verify(self._second_mtd_dev,
                                        fpga_cfg_data.nios_factory_header.file,
                                        fpga_cfg_data.nios_factory_header.start,
                                        (fpga_cfg_data.nios_factory_header.end - fpga_cfg_data.nios_factory_header.start) +1)

            if retval == 0:
                LOG.info('Successfully update & verified NIOS factory header')
            else:
                LOG.exception('Failed update & verify NIOS factory header')
                raise Exception("Failed update & verify NIOS factory header")

        except Exception as e:
            LOG.exception('Failed to update D5005 FPGA')
            LOG.exception(e.message,e.args)
            retval = -1

        finally:
            super(fpgaotsu_n3000,self).set_mtd('fpga_flash_ctrl', 'fpga_flash_mode')

        return retval

    # fpga keys/ dtb / fpga image update
    def n3000_first_flash_update(self):
        try:
            if super(fpgaotsu_n3000,self).get_flash_mode() ==1:
                LOG.error('fpgaflash on %s is in progress, try later', self._fpga.fme.spi_bus.sysfs_path)
                return false

            mtd_dev_list = super(fpgaotsu_n3000,self).get_mtd_list('fpga_flash_ctrl', 'fpga_flash_mode')
            LOG.debug('mtd_dev_list:%s',mtd_dev_list)

            if len(mtd_dev_list) != 2:
                LOG.error('Invalid flash devices')
                return -1

            for i in range(len(mtd_dev_list)):
                if mtd_dev_list[i] != self._second_mtd_dev:
                    first_mtd_dev = mtd_dev_list[i]

            ret_size = mtd.size(first_mtd_dev)
            LOG.info('first_mtd_dev:%s',first_mtd_dev)
            LOG.debug('Flash size:0x%08x',ret_size)

            # Erase from 0x7FFB0000 to 0x7FFFFFF (end) of FPGA flash
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     fpgaotsu_n3000.fpga_erase_flash1_start,fpgaotsu_n3000.fpga_erase_flash1_end)

            mtd.erase(first_mtd_dev,
                        fpgaotsu_n3000.fpga_erase_flash1_start,
                        (fpgaotsu_n3000.fpga_erase_flash1_end - fpgaotsu_n3000.fpga_erase_flash1_start )+1)

            #BMC ROOT KEY HASH
            LOG.info('Updates bmc root key hash %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.bmc_root_key_hash.file,
                    self._fpga_cfg_data.bmc_root_key_hash.start, self._fpga_cfg_data.bmc_root_key_hash.end)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.bmc_root_key_hash.file,
                                        self._fpga_cfg_data.bmc_root_key_hash.start,
                                        (self._fpga_cfg_data.bmc_root_key_hash.end - self._fpga_cfg_data.bmc_root_key_hash.start ) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified bmc key hash')
            else:
                LOG.exception('Failed update & verify bmc key hash')
                raise Exception("Failed update & verify bmc key hash")

            LOG.info('Updates bmc root key program %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.bmc_root_key_program.file,
                    self._fpga_cfg_data.bmc_root_key_program.start, self._fpga_cfg_data.sr_root_key_program.end)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.bmc_root_key_program.file,
                                        self._fpga_cfg_data.bmc_root_key_program.start,
                                        (self._fpga_cfg_data.sr_root_key_program.end - self._fpga_cfg_data.bmc_root_key_program.start) +1 )

            if retval == 0:
                LOG.debug('Successfully update & verified bmc key program')
            else:
                LOG.exception('Failed update & verify bmc key program')
                raise Exception("Failed update & verify bmc key program")

             #SR ROOT KEY HASH
            LOG.info('Updates SR root key hash %s from 0x%08x to 0x%08x ', fpga_cfg_data.sr_root_key_hash.file,
                     fpga_cfg_data.sr_root_key_hash.start, fpga_cfg_data.sr_root_key_hash.end)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.sr_root_key_hash.file,
                                        self._fpga_cfg_data.sr_root_key_hash.start,
                                        (self._fpga_cfg_data.sr_root_key_hash.end - self._fpga_cfg_data.sr_root_key_hash.start) +1 )

            if retval == 0:
                LOG.debug('Successfully update & verified Updates SR root key hash')
            else:
                LOG.error('Failed update & verify Updates SR root key hash')
                raise Exception("Failed update & verify Updates SR root key hash")

            LOG.info('Updates SR root key program %s from 0x%08x to 0x%08x ', self._fpga_cfg_data.sr_root_key_program.file,
                     self._fpga_cfg_data.sr_root_key_program.start, self._fpga_cfg_data.sr_root_key_program.end)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.sr_root_key_program.file,
                                        self._fpga_cfg_data.sr_root_key_program.start,
                                        (self._fpga_cfg_data.sr_root_key_program.end - self._fpga_cfg_data.sr_root_key_program.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified Updates SR root key program')
            else:
                LOG.exception('Failed update & verify Updates SR root key program')
                raise Exception("Failed update & verify Updates SR root key program")

            # DTS
            LOG.info('Updates device tree %s from 0x%08x to 0x%08x',self._fpga_cfg_data.bmc_dts.file,
                     self._fpga_cfg_data.bmc_dts.start, self._fpga_cfg_data.bmc_dts.end)

            mtd.erase(first_mtd_dev,
                      self._fpga_cfg_data.bmc_dts.start,
                      (self._fpga_cfg_data.bmc_dts.end - self._fpga_cfg_data.bmc_dts.start)+1)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.bmc_dts.file,
                                        self._fpga_cfg_data.bmc_dts.start,
                                        (self._fpga_cfg_data.bmc_dts.end - self._fpga_cfg_data.bmc_dts.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified  device tree')
            else:
                LOG.exception('Failed update & verify  device tree')
                raise Exception("Failed update & verify  device tree")

            # FPGA IMAGE
            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     fpgaotsu_n3000.fpga_factory_erase_start,fpgaotsu_n3000.fpga_factory_erase_end)

            mtd.erase(first_mtd_dev,
                        fpgaotsu_n3000.fpga_factory_erase_start,
                        (fpgaotsu_n3000.fpga_factory_erase_end - fpgaotsu_n3000.fpga_factory_erase_start)+1)

            LOG.info('Updates FPGA image %s from 0x%08x to 0x%08x',self._fpga_cfg_data.fpga_factory.file,
                     self._fpga_cfg_data.fpga_factory.start, self._fpga_cfg_data.fpga_factory.end)

            retval = mtd.update_verify(first_mtd_dev,
                                       self._fpga_cfg_data.fpga_factory.file,
                                       self._fpga_cfg_data.fpga_factory.start,
                                       (self._fpga_cfg_data.fpga_factory.end - self._fpga_cfg_data.fpga_factory.start) +1 )

            if retval == 0:
                LOG.info('Successfully update & verified  FPGA Image')
            else:
                LOG.exception('Failed update & verify  FPGA Image')
                raise Exception("Failed update & verify  FPGA Image")

        except Exception as e:
            LOG.exception('Failed to update D5005 FPGA')
            LOG.exception(e.message,e.args)
            retval = -1

        finally:
            super(fpgaotsu_n3000,self).set_mtd('fpga_flash_ctrl', 'fpga_flash_mode')

        return retval

    # Max10 update
    def n3000_max10_update(self):
        try:
            if super(fpgaotsu_n3000,self).get_flash_mode() ==1:
                LOG.error('fpgaflash on %s is in progress, try later', self._fpga.fme.spi_bus.sysfs_path)
                return false

            mtd_dev_list = super(fpgaotsu_n3000,self).get_mtd_list('bmcimg_flash_ctrl', 'bmcimg_flash_mode')
            LOG.debug('mtd_dev_list:%s',mtd_dev_list)

            if len(mtd_dev_list) != 2:
                LOG.error('Invalid flash devices')
                return -1

            for i in range(len(mtd_dev_list)):
                if mtd_dev_list[i] != self._second_mtd_dev:
                    first_mtd_dev = mtd_dev_list[i]
            LOG.info('first_mtd_dev:%s',first_mtd_dev)

            # UFM1 / NIOS Bootloader
            LOG.info('Updates max10 UFM1 fw %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.nios_bootloader.file,
                      self._fpga_cfg_data.nios_bootloader.start, self._fpga_cfg_data.nios_bootloader.end)

            mtd.erase(first_mtd_dev,
                        self._fpga_cfg_data.nios_bootloader.start ,
                        (self._fpga_cfg_data.nios_bootloader.end - self._fpga_cfg_data.nios_bootloader.start )+1)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.nios_bootloader.file,
                                        self._fpga_cfg_data.nios_bootloader.start,
                                        (self._fpga_cfg_data.nios_bootloader.end - self._fpga_cfg_data.nios_bootloader.start )+1)

            if retval == 0:
                LOG.info('Successfully update & verified max10 UFM1')
            else:
                LOG.exception('Failed update & verify Updates max10 UFM1')
                raise Exception("Failed update & verify Updates max10 UFM1")

           #CFM1  FACTORY
            LOG.info('Updates MAX10 CFM1/Factory %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.max10_factory.file,
                      self._fpga_cfg_data.max10_factory.start, self._fpga_cfg_data.max10_factory.end)

            mtd.erase(first_mtd_dev,
                        fpgaotsu_n3000.cfm1_erase_start,
                        (fpgaotsu_n3000.cfm1_erase_end - fpgaotsu_n3000.cfm1_erase_start)+1)

            mtd.erase(first_mtd_dev,
                        fpgaotsu_n3000.cfm2_erase_start ,
                        (fpgaotsu_n3000.cfm2_erase_end - fpgaotsu_n3000.cfm2_erase_start )+1)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.max10_factory.file,
                                       self._fpga_cfg_data.max10_factory.start,
                                       ( self._fpga_cfg_data.max10_factory.end - self._fpga_cfg_data.max10_factory.start ) +1)

            if retval == 0:
                LOG.info('Successfully update & verified MAX10 factory')
            else:
                LOG.exception('Failed update & verify Updates MAX10 factory')
                raise Exception("Failed update & verify Updates MAX10 factory")

            # MAX10 User
            LOG.info('Updates MAX10 CFM1/User %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.max10_user.file,
                      self._fpga_cfg_data.max10_user.start, self._fpga_cfg_data.max10_user.end)

            mtd.erase(first_mtd_dev,
                        self._fpga_cfg_data.max10_user.start,
                        (self._fpga_cfg_data.max10_user.end - self._fpga_cfg_data.max10_user.start)+1)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.max10_user.file,
                                        self._fpga_cfg_data.max10_user.start,
                                        (self._fpga_cfg_data.max10_user.end - self._fpga_cfg_data.max10_user.start ) +1)

            if retval == 0:
                LOG.info('Successfully update & verified max10 user')
            else:
                LOG.exception('Failed update & verify Updates max10 user')
                raise Exception("Failed update & verify Updates max10 user")

        except Exception as e:
            LOG.exception('Failed to update D5005 FPGA')
            LOG.exception(e.message,e.args)
            retval = -1

        finally:
            super(fpgaotsu_n3000,self).set_mtd('bmcimg_flash_ctrl', 'bmcimg_flash_mode')

        return retval

    # Nios User update
    def n3000_nios_user_update(self):
        try:
            if super(fpgaotsu_n3000,self).get_flash_mode() ==1:
                LOG.error('fpgaflash on %s is in progress, try later', self._fpga.fme.spi_bus.sysfs_path)
                return false

            mtd_dev_list = super(fpgaotsu_n3000,self).get_mtd_list('bmcfw_flash_ctrl', 'bmcfw_flash_mode')
            LOG.debug('mtd_dev_list:%s',mtd_dev_list)

            if len(mtd_dev_list) != 2:
                LOG.error('Invalid flash devices')
                return -1

            for i in range(len(mtd_dev_list)):
                if mtd_dev_list[i] != self._second_mtd_dev:
                    first_mtd_dev = mtd_dev_list[i]

            LOG.info('Erase Flash from 0x%08x to 0x%08x',
                     fpgaotsu_n3000.nios_user_erase_start,fpgaotsu_n3000.nios_user_erase_end)
            mtd.erase(first_mtd_dev,
                fpgaotsu_n3000.nios_user_erase_start,
                fpgaotsu_n3000.nios_user_erase_end)

            #NIOS USER
            LOG.info('Updates NIOS user  %s from 0x%08x to 0x%08x ',self._fpga_cfg_data.nios_user.file,
                      self._fpga_cfg_data.nios_user.start, self._fpga_cfg_data.nios_user.end)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.nios_user.file,
                                        self._fpga_cfg_data.nios_user.start,
                                        (self._fpga_cfg_data.nios_user.end - self._fpga_cfg_data.nios_user.start)+1)\

            if retval == 0:
                LOG.info('Successfully update & verified NIOS user')
            else:
                LOG.exception('Failed update & verify Updates NIOS user')
                raise Exception("Failed update & verify Updates NIOS user")

            #NIOS USER Header
            LOG.info('Updates NIOS user header %s from 0x%08x to 0x%08x',self._fpga_cfg_data.nios_user_header.file,
                 self._fpga_cfg_data.nios_user_header.start, self._fpga_cfg_data.nios_user_header.end)

            retval = mtd.update_verify(first_mtd_dev,
                                        self._fpga_cfg_data.nios_user_header.file,
                                        self._fpga_cfg_data.nios_user_header.start,
                                        (self._fpga_cfg_data.nios_user_header.end - self._fpga_cfg_data.nios_user_header.start)+1)

            if retval == 0:
                LOG.info('Successfully update & verified NIOS user header ')
            else:
                LOG.exception('Failed update & verify Updates NIOS user header')
                raise Exception("Failed update & verify Updates NIOS user header")

        except Exception as e:
            LOG.exception('Failed to update D5005 FPGA')
            LOG.exception(e.message,e.args)
            retval = -1

        finally:
            super(fpgaotsu_n3000,self).set_mtd('bmcfw_flash_ctrl', 'bmcfw_flash_mode')

        return retval

    # update n300
    def n3000_fpga_update(self):
        if super(fpgaotsu_n3000,self).RoT_is_updated():
            LOG.info('FPGA Firmware updated to secure')
            return 0 

        retval = self.check_temporary_max10(self._fpga.pci_node.pci_address)
        if retval != 0:
            LOG.error('Not Updated with temporary Max10 user image')
            return -1

        retval = self.n3000_second_flash_update()
        if retval == 0:
            LOG.debug('Successfully updated Nios factory')
        else:
            LOG.error('Failed to update Nios factory')
            return retval

        retval = self.n3000_first_flash_update()
        if retval == 0:
            LOG.debug('Successfully updated FPGA image')
        else:
            LOG.error('Failed to update FPGA image')
            return retval

        retval = self.n3000_max10_update()
        if retval == 0:
            LOG.debug('Successfully updated max10')
        else:
            LOG.error('Failed to update max10')
            return retval

        retval = self.n3000_nios_user_update()
        if retval == 0:
            LOG.debug('Successfully updated Nios user')
        else:
            LOG.error('Failed to update Nios user')
            return retval

        retval = self.n3000_fpga_user()
        if retval == 0:
            LOG.debug('Successfully updated FPGA user')
        else:
            LOG.error('Failed to update FPGA user')
            return retval

        return retval
#end class fpgaotsu_n3000

def fpga_update(path_json, rsu,rsu_only):
    """ Program fpga flash
        Update fpga flash with ROT image
    """
    LOG.debug('path_json: %s', path_json)
    LOG.debug('rsu: %s', rsu)
    LOG.debug('rsu_only: %s', rsu_only)


    path = os.path.dirname(path_json)
    # Parse input config json file
    fpga_cfg_instance = fpga_cfg_data.parse_json_cfg(path_json,path)

    if fpga_cfg_instance is None:
        LOG.error('Invalid Input json config:',path_json)
        return -1

    fpga_cfg_instance.print_json_cfg()

    # Check for valid program files
    if not fpga_cfg_instance.check_fpga_images():
        LOG.error('Invalid image files')
        return -1;


    LOG.info('Number of FPGA devices: %d', \
            len(fpga.enum([{'pci_node.device_id': int(fpga_cfg_instance.device,16)}])))


    if len(fpga.enum([{'pci_node.device_id': int(fpga_cfg_instance.device,16)}])) == 0:
        LOG.error('Invalid Input config device id:%s',fpga_cfg_instance.device)
        return -1


    inst = dict({'pci_node.device_id': int(fpga_cfg_instance.device,16)})

    if(rsu_only):
        retval = do_rsu_fpga(fpga_cfg_instance,None)
        for o in fpga.enum([inst]):
            if o.pci_node.device_id == N3000_DEV_ID:
                LOG.debug('Found FPGA FPGA N3000 Card')
                try:
                    n3000 = fpgaotsu_n3000(o,fpga_cfg_instance)
                    retval = n3000.do_rsu_only(o.pci_node.pci_address)
                    if retval == 0:
                        LOG.debug('Successfully Done RSU')
                    else:
                        LOG.error('Failed to do RSU ')

                except Exception as e:
                    LOG.exception('Failed to do RSU')
                    LOG.exception(e.message,e.args)

        return retval

    for o in fpga.enum([inst]):
        LOG.debug('------FPGA INFO----------')
        LOG.info('pci_node:%s',o.pci_node)
        LOG.info('pci_node.root.tree:%s',o.pci_node.root.tree())
        LOG.debug('fme.devpath:%s',o.fme.devpath)
        LOG.debug('fme.devpath:%s',o.fme.devpath)
        LOG.debug('fme.spi_bus.sysfs_path:%s',o.fme.spi_bus.sysfs_path)
        LOG.debug('port.devpath:%s',o.port.devpath)
        LOG.debug('pci_node.pci_address:%s',o.pci_node.pci_address)
        LOG.debug('pci_node.bdf:%s',o.pci_node.bdf)

        # D5005
        if o.pci_node.device_id == D5005_DEV_ID:
            LOG.info('Found FPGA D5005 Card')
            try:
                d5005 = fpgaotsu_d5005(o,fpga_cfg_instance)
                retval = d5005.d5005_fpga_update()
                if retval == 0:
                    LOG.info('One time udpate tool successfully updated RoT')
                else:
                    LOG.error('One time udpate tool failed to update RoT')
            except Exception as e:
                LOG.exception('One time udpate tool failed to update RoT')
                LOG.exception(e.message,e.args)
                retval = -1

        #N3000
        if o.pci_node.device_id == N3000_DEV_ID:
            LOG.info('Found FPGA FPGA N3000 Card')

            try:
                n3000 = fpgaotsu_n3000(o,fpga_cfg_instance)
                retval = n3000.n3000_fpga_update()
                if retval == 0:
                    LOG.info('One time udpate tool successfully updated RoT')
                else:
                    LOG.error('One time udpate tool failed to update RoT')
                    continue

            except Exception as e:
                LOG.exception('One time udpate tool failed to update RoT')
                LOG.exception(e.message,e.args)
                retval = -1
                continue

            # RSU
            try:
                retval = n3000.do_rsu_only(o.pci_node.pci_address)
                if retval == 0:
                    LOG.info('Done RSU')
                else:
                    LOG.error('Failed RSU')

            except Exception as e:
                LOG.exception('Failed RSU')
                LOG.exception(e.message,e.args)
                retval = -1


    return retval

def sig_handler(signum, frame):
    """raise exception for SIGTERM
    """
    LOG.error('fpgaotsu update interrupted')

def parse_args():
    """Parses fpgaotsu command line arguments
    """
    parser = argparse.ArgumentParser(
             formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument('fpgaotsu_json', help='path to config file \
                       (e.g /usr/share/opae/d5005/one-time-update/fpgaotsu_d5005.json\
                        or /usr/share/opae/n3000/one-time-update/fpgaotsu_n3000.json )')

    parser.add_argument('--rsu-only', default=False, action='store_true',
                        help='only perform the RSU command')

    rsu_help = "perform remote system update after update"
    rsu_help += " causing the board to be rebooted"
    parser.add_argument('--rsu', default=True, action='store_true',
                        help = rsu_help)

    parser.add_argument('-v', '--verbose', default=False,
                        action='store_true', help='display verbose output')

    return parser.parse_args()


# main
def main():

    # Logger
    LOG.setLevel(logging.NOTSET)
    log_fmt = ('[%(asctime)-15s] [%(levelname)-8s] ' '%(message)s')
    log_hndlr = logging.StreamHandler(sys.stdout)
    log_hndlr.setFormatter(logging.Formatter(log_fmt))
    log_hndlr.setLevel(logging.DEBUG)
    LOG.addHandler(log_hndlr)

    signal.signal(signal.SIGTERM, sig_handler)

    LOG.info('Input arguments count: %d', len(sys.argv))

    # Command line args parse

    args = parse_args()

    if args.verbose:
        log_hndlr.setLevel(logging.DEBUG)
    else:
        log_hndlr.setLevel(logging.INFO)

    LOG.debug('fpgaotsu Command line arguments')
    LOG.info('%s',args)

    if not os.path.isfile(args.fpgaotsu_json):
        LOG.error('Invalid input path')
        sys.exit(1)


    retval = fpga_update(args.fpgaotsu_json,args.rsu,args.rsu_only)
    if( retval == 0 ):
        sys.exit(0)
    else:
        sys.exit(1)

    # Exit
    sys.exit(0)


if __name__ == "__main__":
    main()
