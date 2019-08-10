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
import time

from contextlib import contextmanager
from opae.admin.sysfs import class_node, sysfs_node
from opae.admin.utils.log import loggable


class region(sysfs_node):
    def __init__(self, sysfs_path, pci_node):
        super(region, self).__init__(sysfs_path)
        self._pci_node = pci_node
        self._fd = -1

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

    def __enter__(self):
        self._fd = os.open(self.devpath, os.O_SYNC | os.O_RDWR)
        fcntl.lockf(self._fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return self._fd

    def __exit__(self, ex_type, ex_val, ex_traceback):
        fcntl.lockf(self._fd, fcntl.LOCK_UN)
        os.close(self._fd)


class flash_control(loggable):
    def __init__(self, name='flash', mtd_dev=None, control_node=None):
        super(flash_control, self).__init__()
        self._name = name
        self._mtd_dev = mtd_dev
        self._control_node = control_node
        self._enabled = False
        self._dev_path = None

    @property
    def name(self):
        return self._name

    @property
    def enabled(self):
        return self._enabled

    def _devpath(self, interval, retries):
        if self._dev_path:
            return self._dev_path

        sysfs_path = os.path.dirname(self._control_node.sysfs_path)
        sysfs_path = os.path.dirname(sysfs_path)

        pattern = 'intel-*.*.auto/mtd/mtd*'

        while retries:
            mtds = sysfs_node(sysfs_path).find_all(pattern)

            if not len(mtds):
                time.sleep(interval)
                retries -= 1
                continue

            mtd = [m for m in mtds if m.sysfs_path[-2:] != 'ro']
            if len(mtd) > 1:
                self.log.warn('found more than one: "/mtd/mtdX"')

            return os.path.join('/dev',
                                os.path.basename(mtd[0].sysfs_path))

        msg = 'timeout waiting for %s to appear' % (pattern)
        self.log.error(msg)
        raise IOError(msg)

    def enable(self):
        if self._control_node:
            if not isinstance(self._control_node, sysfs_node):
                raise ValueError('%s not a sysfs_node' % str(sysfs_node))
            self._control_node.value = 1
        self._enabled = True
        self._dev_path = self.devpath

    def disable(self, interval=0.1, retries=100):
        if not self._enabled:
            raise IOError('attempt to disable when not enabled')
        if self._control_node:
            self._control_node.value = 0
            while os.path.exists(self._dev_path):
                time.sleep(interval)
                retries -= 1
                if not retries:
                    msg = 'timeout waiting for %s to vanish' % (self._dev_path)
                    raise IOError(msg)
            self._dev_path = None
        self._enabled = False

    @property
    def devpath(self):
        if not self._enabled:
            raise IOError('cannot query devpath attribute outside context')
        if not self._mtd_dev:
            return self._devpath(0.1, 100)
        dev_path = os.path.join('/dev', self._mtd_dev)
        if not os.path.exists(dev_path):
            raise AttributeError('no device found: %s' % dev_path)
        return dev_path

    def __enter__(self):
        self.enable()
        return self

    def __exit__(self, ex_type, ex_val, ex_traceback):
        self.disable()


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

    @property
    def altr_asmip(self):
        return self.find_one('altr-asmip*.*.auto')

    def flash_controls(self):
        if self.spi_bus:
            sec = self.spi_bus.find_one('ifpga_sec_mgr/ifpga_sec*')
            if sec:
                return []
            return [flash_control(name='fpga', mtd_dev=None,
                                  control_node=self.spi_bus.node(
                                      'fpga_flash_ctrl/fpga_flash_mode')),
                    flash_control(name='bmcfw', mtd_dev=None,
                                  control_node=self.spi_bus.node(
                                      'bmcfw_flash_ctrl/bmcfw_flash_mode')),
                    flash_control(name='bmcimg', mtd_dev=None,
                                  control_node=self.spi_bus.node(
                                      'bmcimg_flash_ctrl/bmcimg_flash_mode'))]
        elif self.altr_asmip:
            mtds = self.altr_asmip.find_all('mtd/mtd*')
            mtd = [m for m in mtds if m.sysfs_path[-2:] != 'ro']
            if len(mtd) > 1:
                self.log.warn('found more than one: "/mtd/mtdX"')
            return [flash_control(mtd_dev=os.path.basename(mtd[0].sysfs_path))]


class port(region):
    @property
    def afu_id(self):
        return self.node('afu_id').value


class secure_dev(region):
    pass


class fpga(class_node):
    FME_PATTERN = 'intel-fpga-fme.*'
    PORT_PATTERN = 'intel-fpga-port.*'
    BOOT_TYPES = ['bmcimg', 'fpga']

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
    def secure_dev(self):
        f = self.fme
        if not f:
            self.log.error('no FME found')
            return None
        spi = f.spi_bus
        if not spi:
            self.log.error('no spi bus')
            return None
        sec = spi.find_one('ifpga_sec_mgr/ifpga_sec*')
        if sec:
            return secure_dev(sec.sysfs_path, self.pci_node)

    @property
    def port(self):
        items = self.find_all(self.PORT_PATTERN)
        if len(items) == 1:
            return port(items[0].sysfs_path, self.pci_node)
        if not items:
            self.log.warning('could not find PORT')
        if len(items) > 1:
            self.log.warning('found more than one PORT')

    def rsu_boot(self, page, **kwargs):
        boot_type = kwargs.pop('type', 'bmcimg')
        if kwargs:
            self.log.exception('unrecognized kwargs: %s', kwargs)
            raise ValueError('unrecognized kwargs: {}'.format(kwargs))
        if boot_type not in fpga.BOOT_TYPES:
            raise TypeError('type: {} not recognized'.format(boot_type))

        node_path = '{boot_type}_flash_ctrl/{boot_type}_image_load'.format(
            boot_type=boot_type)
        node = self.fme.spi_bus.node(node_path)
        node.value = page

    @contextmanager
    def disable_aer(self, *nodes):
        aer_values = []
        to_disable = nodes or self.pci_node.root
        try:
            for node in to_disable:
                aer_values.append((node, node.aer))
                node.aer = (0xFFFFFFFF, 0xFFFFFFFF)
            yield True if aer_values else None
        finally:
            for n, v in aer_values:
                n.aer = v

    def safe_rsu_boot(self, page, **kwargs):
        wait_time = kwargs.pop('wait', 10)
        boot_type = kwargs.get('type', 'bmcimg')
        if boot_type not in fpga.BOOT_TYPES:
            raise TypeError('type: {} not recognized'.format(boot_type))

        if boot_type == 'fpga':
            to_remove = self.pci_node.root.endpoints
            to_disable = [ep.parent for ep in to_remove]
        else:
            to_remove = [self.pci_node.branch[1]]
            to_disable = [self.pci_node.root]

        with self.disable_aer(*to_disable):
            self.rsu_boot(page, **kwargs)

            to_rescan = []
            for node in to_remove:
                to_rescan.append((node.parent, '{}:{}'.format(node.domain,
                                                              node.bus)))
                node.remove()
            time.sleep(wait_time)

            for node, bus in to_rescan:
                node.rescan_bus(bus)
