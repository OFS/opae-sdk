# Copyright(c) 2019-2023, Intel Corporation
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

from __future__ import absolute_import
import errno
import fcntl
import os
import struct
import time

from array import array
from contextlib import contextmanager
from opae.admin.config import Config
from opae.admin.path import device_path, sysfs_path
from opae.admin.sysfs import sysfs_device, sysfs_node
from opae.admin.utils.log import loggable
from opae.admin.utils.process import call_process
from opae.admin.utils import max10_or_nios_version


class region(sysfs_node):
    def __init__(self, path, pci_node):
        super(region, self).__init__(path)
        self._pci_node = pci_node
        self._fd = -1

    @property
    def pci_node(self):
        return self._pci_node

    @property
    def devpath(self):
        basename = os.path.basename(self.sysfs_path)
        dev_path = device_path(basename)
        if not os.path.exists(dev_path):
            raise AttributeError('no device found: {}'.format(dev_path))
        return dev_path

    def ioctl(self, req, data, **kwargs):
        mode = kwargs.get('mode', 'w')
        with open(self.devpath, mode) as fd:
            try:
                fcntl.ioctl(fd.fileno(), req, data)
            except (IOError, OSError) as err:
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
    _mtd_pattern = 'intel-*.*.auto/mtd/mtd*'

    def __init__(self, name='flash', mtd_dev=None, control_node=None,
                 spi=None):
        super(flash_control, self).__init__()
        self._name = name
        self._mtd_dev = mtd_dev
        self._control_node = control_node
        self._spi = spi
        self._existing = []
        self._always_on = mtd_dev is not None
        self._enabled = False
        self._dev_path = None
        self._enabled_count = 0

    @property
    def name(self):
        return self._name

    @property
    def enabled(self):
        return self._always_on or self._enabled

    def _find_mtds(self):
        if self._spi:
            mtds = [os.path.basename(mtd.sysfs_path)
                    for mtd in self._spi.find_all(self._mtd_pattern)
                    if not mtd.sysfs_path.endswith('ro')]
            return mtds
        return []

    def _wait_devpath(self, interval, retries):
        if self._dev_path:
            return self._dev_path

        while retries:
            current_mtds = self._find_mtds()
            mtds = list(set(current_mtds).difference(set(self._existing)))

            if not len(mtds):
                time.sleep(interval)
                retries -= 1
                continue

            if len(mtds) > 1:
                self.log.warning('found more than one: "/mtd/mtdX"')

            return device_path(mtds[0])

        msg = 'timeout waiting for %s to appear' % (self._mtd_pattern)
        self.log.error(msg)
        raise IOError(msg)

    def enable(self):
        if self._always_on:
            return

        if self._enabled and self._enabled_count:
            self._enabled_count += 1
            return

        if self._control_node:
            if not isinstance(self._control_node, sysfs_node):
                raise ValueError('%s not a sysfs_node' % str(sysfs_node))
            self._existing = self._find_mtds()
            self._control_node.value = 1
        self._enabled = True
        self._dev_path = self.devpath
        self._enabled_count += 1

    def disable(self, interval=0.1, retries=100):
        if self._always_on:
            return

        if not self._enabled or self._enabled_count < 1:
            raise IOError('attempt to disable when not enabled: {}'.format(
                self.name))

        self._enabled_count -= 1
        if self._enabled_count < 1:
            if self._control_node:
                self._control_node.value = 0
                while self._dev_path and os.path.exists(self._dev_path):
                    time.sleep(interval)
                    retries -= 1
                    if not retries:
                        msg = 'timeout waiting for {} to vanish'.format(
                            self._dev_path)
                        raise IOError(msg)
                self._dev_path = None
            self._enabled = False

    @property
    def devpath(self):
        if self._always_on and self._mtd_dev:
            dev_path = device_path(self._mtd_dev)
            if not os.path.exists(dev_path):
                raise AttributeError('no device found: %s' % dev_path)
            return dev_path

        if not self._enabled:
            raise IOError('cannot query devpath attribute outside context')
        if not self._mtd_dev:
            return self._wait_devpath(0.1, 100)

    def __enter__(self):
        self.enable()
        return self

    def __exit__(self, ex_type, ex_val, ex_traceback):
        self.disable()


class fme(region):
    DFL_FPGA_FME_PORT_RELEASE = 0x4004b681
    DFL_FPGA_FME_PORT_ASSIGN = 0x4004b682

    @property
    def pr_interface_id(self):
        if os.path.basename(self.sysfs_path).startswith('dfl'):
            glob_pat = 'dfl-fme-region.*/fpga_region/region*/compat_id'
            return self.find_one(glob_pat).value
        return self.node('pr/interface_id').value

    @property
    def i2c_bus(self):
        return self.find_one('*i2c*/i2c*')

    @property
    def spi_bus(self):
        if os.path.basename(self.sysfs_path).startswith('dfl'):
            patterns = ['dfl*.*/*spi*/spi_master/spi*/spi*',
                        'dfl*.*/spi_master/spi*/spi*']
            for pattern in patterns:
                spi = self.find_one(pattern)
                if spi:
                    return spi
        return self.find_one('spi*/spi_master/spi*/spi*')

    @property
    def pmci_bus(self):
        if os.path.basename(self.sysfs_path).startswith('dfl'):
            patterns = ['dfl*.*/*-sec*.*.auto']
            for pattern in patterns:
                pmci = self.find_one(pattern)
                if pmci:
                    return pmci
        return self.find_one('*-sec*.*.auto')

    @property
    def boot_page(self):
        pmci = self.pmci_bus
        if pmci:
            return pmci.find_one('**/fpga_boot_image')

    @property
    def altr_asmip(self):
        return self.find_one('altr-asmip*.*.auto')

    @property
    def avmmi_bmc(self):
        node = self.find_one('avmmi-bmc.*.auto')
        if node is not None:
            return avmmi_bmc(node.sysfs_path, self.pci_node)

    @property
    def max10_version(self):
        spi = self.spi_bus
        if spi:
            node = spi.find_one('max10_version')
            value = int(node.value, 16)
            return max10_or_nios_version(value)
        else:
            pmci = self.pmci_bus
            if pmci:
                node = spi.find_one('max10_version')
                value = int(node.value, 16)
                return max10_or_nios_version(value)

    @property
    def bmcfw_version(self):
        spi = self.spi_bus
        if spi:
            node = spi.find_one('bmcfw_version')
            value = int(node.value, 16)
            return max10_or_nios_version(value)
        else:
            pmci = self.pmci_bus
            if pmci:
                node = spi.find_one('bmcfw_version')
                value = int(node.value, 16)
                return max10_or_nios_version(value)

    @property
    def fpga_image_load(self):
        spi = self.spi_bus
        if spi:
            node = spi.find_one('fpga_flash_ctrl/fpga_image_load')
            if node:
                return node.value

    @property
    def bmc_aux_fw_rev(self):
        bmc = self.avmmi_bmc
        if bmc:
            node = bmc.find_one('bmc_info/device_id')
            return struct.unpack_from('<15BL', node.value)[15]

    def flash_controls(self):
        if self.spi_bus:
            sec = self.spi_bus.find_one('*fpga_sec_mgr/*fpga_sec*')
            if sec:
                return []
            pattern = 'intel-*.*.auto/mtd/mtd*'
            current = []
            for mtd in self.spi_bus.find_all(pattern):
                if not mtd.sysfs_path.endswith('ro'):
                    devname = os.path.basename(mtd.sysfs_path)
                    current.append(devname)
            controls = [flash_control(mtd_dev=name, spi=self.spi_bus)
                        for name in current]
            for name in ['fpga', 'bmcimg', 'bmcfw']:
                node_path = '{name}_flash_ctrl/{name}_flash_mode'.format(
                    name=name)
                control_node = self.spi_bus.node(node_path)
                if control_node.value == '0':
                    controls.append(
                        flash_control(
                            name=name, mtd_dev=None,
                            control_node=control_node,
                            spi=self.spi_bus)
                        )
                else:
                    self.log.warning('skipping control %s (already enabled)',
                                     node_path)
            return controls
        elif self.altr_asmip:
            mtds = self.altr_asmip.find_all('mtd/mtd*')
            mtd = [m for m in mtds if m.sysfs_path[-2:] != 'ro']
            if len(mtd) > 1:
                self.log.warning('found more than one: "/mtd/mtdX"')
            return [flash_control(mtd_dev=os.path.basename(mtd[0].sysfs_path))]

    @property
    def num_ports(self):
        """num_ports Get the number of ports supported by FME"""
        if self.have_node('ports_num'):
            return int(self.node('ports_num').value)

    def release_port(self, port_num):
        """release_port Release port device (enable SR-IOV).
                        This will "release" the port device and allow
                        VFs to be created.

        Args:
            port_num: The port number to release.
        Raises:
            ValueError: If the port number is invalid.
            OSError: If current process is unable to open FME.
            IOError: If an error occurred with the IOCTL request.
        """
        if port_num >= self.num_ports:
            msg = 'port number is invalid: {}'.format(port_num)
            self.log.error(msg)
            raise ValueError(msg)
        data = struct.pack('i', port_num)
        self.ioctl(self.DFL_FPGA_FME_PORT_RELEASE, data)

    def assign_port(self, port_num):
        """assign_port Assign port device (disable SR-IOV).
                        This will "assign" the port device back to the FME.
                        SR-IOV will be disabled.

        Args:
            port_num: The port number to assign.
        Raises:
            ValueError: If the port number is invalid.
            ValueError: If the pci device has VFs created.
            OSError: If current process is unable to open FME.
            IOError: If an error occurred with the IOCTL request.
        """
        if port_num >= self.num_ports:
            msg = 'port number is invalid: {}'.format(port_num)
            self.log.error(msg)
            raise ValueError(msg)
        if self.pci_node.sriov_numvfs:
            msg = 'Cannot assign a port while VFs exist'
            raise ValueError(msg)
        data = struct.pack('i', port_num)
        self.ioctl(self.DFL_FPGA_FME_PORT_ASSIGN, data)


class port(region):
    @property
    def afu_id(self):
        return self.node('afu_id').value


class upload_dev(region):
    pass


class secure_update(region):
    pass


class avmmi_bmc(region):
    BMC_BOOT_REQ = 0xc0187600

    def reboot(self):
        """reboot Issue a boot request via IOCTL to trigger a board reboot.

        Raises:
            IOError: If IOCTL was not successful or if completion code is
            non-zero.
        """

        tx = array('B', [0xB8, 0x00, 0x09, 0x18, 0x7B, 0x00, 0x01, 0x05])
        rx = array('B', [0, 0, 0, 0, 0, 0, 0])
        data = struct.pack('IHHQQ', 24,
                           tx.buffer_info()[1], rx.buffer_info()[1],
                           tx.buffer_info()[0], rx.buffer_info()[0])

        with self as dev:
            fcntl.ioctl(dev, self.BMC_BOOT_REQ, data)

        ccode = rx.pop(3)
        if ccode:
            raise IOError('bad completion code: 0x{:8x}'.format(ccode))


class fpga_base(sysfs_device):
    FME_PATTERN = 'intel-fpga-fme.*'
    PORT_PATTERN = 'intel-fpga-port.*'
    PCI_DRIVER = 'intel-fpga-pci'

    def __init__(self, path):
        super(fpga_base, self).__init__(path)

    @property
    def fme(self):
        items = self.find_all(self.FME_PATTERN)
        if len(items) == 1:
            return fme(items[0].sysfs_path, self.pci_node)
        # if I can't find FME and I am not a VF
        # (as indicated by the presence of 'physfn')
        if not items and not self.pci_node.have_node('physfn'):
            self.log.warning('could not find FME')
        if len(items) > 1:
            self.log.warning('found more than one FME')

    @property
    def upload_dev(self):
        f = self.fme
        if not f:
            self.log.error('no FME found')
            return None
        spi = f.spi_bus
        if spi:
            patterns = ['*-sec*.auto/*fpga_sec_mgr*/*fpga_sec*',
                        '*-sec*.auto/fpga_image_load/fpga_image*',
                        '*-sec*.auto/firmware/secure-update*' ]
            for pattern in patterns:
                fpga_sec = spi.find_one(pattern)
                if fpga_sec:
                    return upload_dev(fpga_sec.sysfs_path, self.pci_node)
        else:
            pmci = f.pmci_bus
            if not pmci:
                self.log.debug('no PMCI found')
                return None

            patterns = ['*fpga_sec_mgr*/*fpga_sec*',
                        'fpga_image_load/fpga_image*',
                        'firmware/secure-update*' ]
            for pattern in patterns:
                fpga_sec = pmci.find_one(pattern)
                if fpga_sec:
                    return upload_dev(fpga_sec.sysfs_path, self.pci_node)

    @property
    def secure_update(self):
        f = self.fme
        if not f:
            self.log.error('no FME found')
            return None
        spi = f.spi_bus
        if spi:
            sec = spi.find_one('*-sec*.*.auto')
            if sec:
                return secure_update(sec.sysfs_path, self.pci_node)
        else:
            pmci = f.pmci_bus
            if pmci:
                return secure_update(pmci.sysfs_path, self.pci_node)

    @property
    def port(self):
        items = self.find_all(self.PORT_PATTERN)
        if len(items) == 1:
            return port(items[0].sysfs_path, self.pci_node)
        if not items:
            self.log.warning('could not find PORT')
        if len(items) > 1:
            self.log.warning('found more than one PORT')

    @property
    def supports_rsu(self):
        """supports_rsu Indicates if device supports RSU

        Returns: True if device supports RSU, false otherwise.
        """
        return Config.rsu_is_supported(*self.pci_node.pci_id)

    @property
    def rsu_controls(self):
        available_images = None
        image_load = None

        patterns = ['',
                    '*-sec*.*.auto',
                    '*-sec*.*.auto/*fpga_sec_mgr/*fpga_sec*',
                    '*-sec*.*.auto/fpga_image_load/fpga_image*']

        spi = self.fme.spi_bus
        if spi:
            for pat in patterns:
                for d in ['control', 'update']:
                    available_images = spi.find_one(
                        os.path.join(pat, d, 'available_images'))
                    image_load = spi.find_one(
                        os.path.join(pat, d, 'image_load'))
                    if available_images:
                        return available_images, image_load

        pmci = self.fme.pmci_bus
        if pmci:
            for pat in patterns:
                for d in ['control', 'update']:
                    available_images = pmci.find_one(
                        os.path.join(pat, d, 'available_images'))
                    image_load = pmci.find_one(
                        os.path.join(pat, d, 'image_load'))
                    if available_images:
                        return available_images, image_load

        return None, None

    def rsu_boot(self, available_image, **kwargs):
        # look for non-max10 solution
        fme = self.fme
        if fme:
            bmc = fme.avmmi_bmc
            if bmc is None:
                self._rsu_boot_sysfs(available_image, **kwargs)
            else:
                bmc.reboot()
        else:
            self.log.warn('do not have FME device')

    def _rsu_boot_sysfs(self, available_image, **kwargs):

        if kwargs:
            self.log.exception('unrecognized kwargs: %s', kwargs)
            raise ValueError('unrecognized kwargs: {}'.format(kwargs))

        available_images, image_load = self.rsu_controls

        if not available_images or not image_load:
            msg = 'rsu not supported by this (0x{:04x},0x{:04x})'.format(
                self.pci_node.pci_id[0], self.pci_node.pci_id[1])
            self.log.exception(msg)
            raise TypeError(msg)

        available_images = available_images.value

        if available_image in available_images:
            image_load.value = available_image
        else:
            msg = 'Boot type {} is not supported ' \
                  'by this (0x{:04x},0x{:04x})'.format(
                      available_image,
                      self.pci_node.pci_id[0],
                      self.pci_node.pci_id[1])
            self.log.exception(msg)
            raise ValueError(msg)

    @contextmanager
    def disable_aer(self, *nodes):
        aer_values = []
        to_disable = nodes or [self.pci_node.root]
        try:
            for node in to_disable:
                aer_values.append((node, node.aer))
                node.aer = (0xFFFFFFFF, 0xFFFFFFFF)
            yield True if aer_values else None
        finally:
            for n, v in aer_values:
                n.aer = v

    def safe_rsu_boot(self, available_image, **kwargs):
        root_port = self.pci_node.root
        force = kwargs.pop('force', False)
        supports_dpc = root_port.supports_ecap('dpc')
        if root_port.supports_ecap('aer'):
            self.log.debug('ECAP_AER is supported')
            if (not supports_dpc) or force:
                if supports_dpc:
                    self.log.warn('forcing disable ECAP_AER even ECAP_DPC supported')
                with self.disable_aer(root_port):
                    self.rsu_routine(available_image, **kwargs)
            else:
                self.log.debug('ECAP_DPC is supported')
                self.rsu_routine(available_image, **kwargs)
        else:
            if supports_dpc or force:
                if supports_dpc:
                    self.log.debug('ECAP_DPC is supported')
                else:
                    self.log.warn('forcing rsu')
                self.rsu_routine(available_image, **kwargs)
            else:
                msg = 'ECAP_AER and ECAP_DPC not supported'
                self.log.error(msg)
                raise IOError(msg)

    def rsu_routine(self, available_image, **kwargs):
        wait_time = kwargs.pop('wait', 10)
        to_remove = [self.pci_node.root]
        # rescan at the pci bus, if found
        # if for some reason it can't be found, do a full system rescan
        to_rescan = self.pci_node.pci_bus
        if not to_rescan:
            self.log.warning(('cannot find pci bus to rescan, will do a '
                              'system pci rescan'))
            to_rescan = sysfs_node('/sys/bus/pci')

        self.log.info('[%s] performing RSU operation', self.pci_node)

        self.log.debug('unbinding drivers from other endpoints')

        for ep in self.pci_node.root.endpoints:
            if ep.pci_address != self.pci_node.pci_address:
                ep.unbind()

        try:
            self.rsu_boot(available_image, **kwargs)
        except IOError as err:
            if err.errno == errno.EBUSY:
                self.log.warn('device busy, cannot perform RSU operation')
            else:
                self.log.error('error triggering RSU operation: %s', err)
            raise

        for node in to_remove:
            self.log.info('[%s] removing device from PCIe bus', node)
            node.remove()

        self.log.info('waiting %s seconds for boot', wait_time)
        time.sleep(wait_time)

        self.log.info('rescanning PCIe bus: %s', to_rescan.sysfs_path)
        to_rescan.node('rescan').value = 1

    def clear_status_and_errors(self):
        time.sleep(1)

        root = self.pci_node.root
        self.clear_device_status(root)
        self.clear_uncorrectable_errors(root)
        self.clear_correctable_errors(root)

        for ep in root.endpoints:
            self.clear_device_status(ep)
            self.clear_uncorrectable_errors(ep)
            self.clear_correctable_errors(ep)

    def clear_device_status(self, device):
        self.log.debug(f'Clearing device status for {device.pci_address}')
        cmd = f'setpci -s {device.pci_address} CAP_EXP+0x08.L'
        try:
            output = int(call_process(cmd), 16)
        except ValueError: # Error during conversion. setpci() call failed.
            return
        output &= ~0xFF000
        output |= 0xF5000
        call_process(f'{cmd}={output:08x}')

    def clear_uncorrectable_errors(self, device):
        if not device.supports_ecap('aer'):
            self.log.debug(f'No AER support in device {device.pci_address}')
            return
        self.log.debug(f'Clearing uncorrectable errors for {device.pci_address}')
        cmd = f'setpci -s {device.pci_address} ECAP_AER+0x04.L'
        call_process(f'{cmd}=FFFFFFFF')

    def clear_correctable_errors(self, device):
        if not device.supports_ecap('aer'):
            self.log.debug(f'No AER support in device {device.pci_address}')
            return
        self.log.debug(f'Clearing correctable errors for {device.pci_address}')
        cmd = f'setpci -s {device.pci_address} ECAP_AER+0x10.L'
        call_process(f'{cmd}=FFFFFFFF')


class fpga_region(fpga_base):
    FME_PATTERN = 'dfl-fme.*'
    PORT_PATTERN = 'dfl-port.*'
    PCI_DRIVER = 'dfl-pci'
    DEVICE_ROOT = 'class/fpga_region'

    @classmethod
    def enum_filter(cls, node):
        if not node.have_node('device'):
            return False
        if 'dfl-fme-region' in os.readlink(node.node('device').sysfs_path):
            return False
        return True


class fpga(fpga_base):
    _drivers = [fpga_region, fpga_base]
    DEVICE_ROOT = 'class/fpga'

    @classmethod
    def enum(cls, filt=[]):
        for drv in cls._drivers:
            drv_path = sysfs_path('/sys/bus/pci/drivers', drv.PCI_DRIVER)
            if os.path.exists(drv_path):
                return drv.enum(filt)

        print('no fpga drivers loaded')
