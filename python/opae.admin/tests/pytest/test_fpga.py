import unittest
from unittest import mock
from opae.admin.fpga import (region,
                             flash_control,
                             fpga)


class test_fpga(unittest.TestCase):
    def setUp(self):
        self._device = fpga.enum()[0]
        self._device.pci_node.sriov_numvfs = 0

    def tearDown(self):
        pass

    '''test_pci_node_get'''

    def test_pci_node_get(self):
        init_sysfs_path = mock.MagicMock()
        init_pci_node = mock.MagicMock()
        region_test = region(init_sysfs_path, init_pci_node)
        _ = region_test.pci_node

    '''test_devpath_get'''

    def test_devpath_get(self):
        with open('/dev/mtd0', 'w+b') as fp:
            fp.truncate(1024)
        flash_control_test = flash_control('flash', 'mtd0')

        _ = flash_control_test.devpath

    '''test_ioctl'''

    def test_ioctl(self):
        region_test = region(self._device.fme.sysfs_path,
                             self._device.pci_node)
        ioctl_req = mock.MagicMock()
        ioctl_data = mock.MagicMock()
        with mock.patch('fcntl.ioctl'):
            region_test.ioctl(ioctl_req, ioctl_data)

    '''test_name_get'''

    def test_name_get(self):
        init_name = 'flash'
        init_mtd_dev = 'mtd0'
        init_control_node = mock.MagicMock()
        init_spi = mock.MagicMock()
        flash_control_test = flash_control(
            init_name, init_mtd_dev, init_control_node, init_spi)
        _ = flash_control_test.name

    '''test_enabled_get'''

    def test_enabled_get(self):
        init_name = mock.MagicMock()
        init_mtd_dev = mock.MagicMock()
        init_control_node = mock.MagicMock()
        init_spi = mock.MagicMock()
        flash_control_test = flash_control(
            init_name, init_mtd_dev, init_control_node, init_spi)
        _ = flash_control_test.enabled

    '''test_enable'''

    def test_enable(self):
        init_name = mock.MagicMock()
        init_mtd_dev = mock.MagicMock()
        init_control_node = mock.MagicMock()
        init_spi = mock.MagicMock()
        flash_control_test = flash_control(
            init_name, init_mtd_dev, init_control_node, init_spi)
        flash_control_test.enable()

    '''test_disable'''

    def test_disable(self):
        init_name = mock.MagicMock()
        init_mtd_dev = mock.MagicMock()
        init_control_node = mock.MagicMock()
        init_spi = mock.MagicMock()
        flash_control_test = flash_control(
            init_name, init_mtd_dev, init_control_node, init_spi)
        disable_interval = mock.MagicMock()
        disable_retries = mock.MagicMock()
        flash_control_test.disable(disable_interval, disable_retries)

    '''test_pr_interface_id_get'''

    def test_pr_interface_id_get(self):
        fme_test = self._device.fme
        _ = fme_test.pr_interface_id

    '''test_i2c_bus_get'''

    def test_i2c_bus_get(self):
        fme_test = self._device.fme
        _ = fme_test.i2c_bus

    '''test_spi_bus_get'''

    def test_spi_bus_get(self):
        fme_test = self._device.fme
        _ = fme_test.spi_bus

    '''test_altr_asmip_get'''

    def test_altr_asmip_get(self):
        fme_test = self._device.fme
        _ = fme_test.altr_asmip

    '''test_max10_version_get'''

    def test_max10_version_get(self):
        fme_test = self._device.fme
        _ = fme_test.max10_version

    '''test_bmcfw_version_get'''

    def test_bmcfw_version_get(self):
        fme_test = self._device.fme
        _ = fme_test.bmcfw_version

    '''test_fpga_image_load_get'''

    def test_fpga_image_load_get(self):
        fme_test = self._device.fme
        _ = fme_test.fpga_image_load

    '''test_flash_controls'''

    def test_flash_controls(self):
        fme_test = self._device.fme
        fme_test.flash_controls()

    '''test_num_ports_get'''

    def test_num_ports_get(self):
        fme_test = self._device.fme
        _ = fme_test.num_ports

    '''test_release_port'''

    def test_release_port(self):
        fme_test = self._device.fme
        with mock.patch('fcntl.ioctl'):
            fme_test.release_port(0)

    '''test_assign_port'''

    def test_assign_port(self):
        fme_test = self._device.fme
        with mock.patch('fcntl.ioctl'):
            fme_test.assign_port(0)

    '''test_afu_id_get'''

    def test_afu_id_get(self):
        port_test = self._device.port
        _ = port_test.afu_id

    '''test_fme_get'''

    def test_fme_get(self):
        fpga_test = self._device
        _ = fpga_test.fme

    '''test_secure_dev_get'''

    def test_secure_dev_get(self):
        fpga_test = self._device
        _ = fpga_test.secure_dev

    '''test_port_get'''

    def test_port_get(self):
        fpga_test = self._device
        _ = fpga_test.port

    '''test_supports_rsu_get'''

    def test_supports_rsu_get(self):
        fpga_test = self._device
        _ = fpga_test.supports_rsu

    '''test_rsu_boot'''

    def test_rsu_boot(self):
        fpga_test = self._device
        rsu_boot_page = mock.MagicMock()
        fpga_test.rsu_boot(rsu_boot_page)

    '''test_disable_aer'''

    def test_disable_aer(self):
        fpga_test = self._device
        fpga_test.disable_aer()

    '''test_safe_rsu_boot'''

    def test_safe_rsu_boot(self):
        fpga_test = self._device
        with mock.patch('subprocess.check_output',
                        side_effect=['0x0'.encode(),
                                     '0x0'.encode(),
                                     ''.encode(),
                                     ''.encode(),
                                     ''.encode(),
                                     ''.encode()]):
            with self.assertRaises(OSError):
                fpga_test.safe_rsu_boot(0)
