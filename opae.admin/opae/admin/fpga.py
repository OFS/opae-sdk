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
import fcntl
import os
from opae.admin.sysfs import class_node, sysfs_node


class region(sysfs_node):
    def __init__(self, sysfs_path, pci_node):
        super(region, self).__init__(sysfs_path)
        self._pci_node = pci_node

    @property
    def pci_node(self):
        return self._pci_node

    @property
    def devpath(self):
        basename = os.path.basename(self.sysfs_path)
        dev_path = os.path.join('/dev', basename)
        if not os.path.exists(dev_path):
            raise AttributeError('no device found: {}'.format(dev_path))
        return dev_path

    def ioctl(self, req, data, mutate_flag=True):
        with open(self.devpath, 'rwb') as fd:
            try:
                fcntl.ioctl(fd, req, data, mutate_flag)
            except IOError as err:
                self.log.exception('error calling ioctl: %s', err)
                raise
            else:
                return data


class fme(region):
    @property
    def pr_interface_id(self):
        return self.node('pr/interface_id').value

    @property
    def i2c_bus(self):
        return self.find_one('*i2c*/i2c*')

    @property
    def spi_bus(self):
        return self.find_one('spi*/spi_master/spi*/spi*')


class port(region):
    @property
    def afu_id(self):
        return self.node('afu_id').value


class fpga(class_node):
    FME_PATTERN = 'intel-fpga-fme.*'
    PORT_PATTERN = 'intel-fpga-port.*'

    def __init__(self, path):
        super(fpga, self).__init__(path)

    @property
    def fme(self):
        items = self.find_all(self.FME_PATTERN)
        if len(items) == 1:
            return fme(items[0].sysfs_path, self.pci_node)
        if not items:
            self.log.warning('could not find FME')
        if len(items) > 1:
            self.log.warning('found more than one FME')

    @property
    def port(self):
        items = self.find_all(self.PORT_PATTERN)
        if len(items) == 1:
            return port(items[0].sysfs_path, self.pci_node)
        if not items:
            self.log.warning('could not find PORT')
        if len(items) > 1:
            self.log.warning('found more than one PORT')
