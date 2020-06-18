import unittest
from unittest import mock
from opae.admin.tools.rsu import (normalize_bdf,
                                  do_rsu)
from opae.admin.fpga import fpga


class test_rsu(unittest.TestCase):
    def setUp(self):
        self._devices = fpga.enum()
        self._device = self._devices[0]

    def tearDown(self):
        pass

    '''test_normalize_bdf'''

    def test_normalize_bdf(self):
        normalize_bdf_bdf = self._device.pci_node.pci_address
        normalize_bdf(normalize_bdf_bdf)

    '''test_do_rsu'''

    def test_do_rsu(self):
        do_rsu_rsu_type = fpga.BOOT_TYPES[0]
        do_rsu_device = self._device
        do_rsu_boot_page = 'user'
        with mock.patch('subprocess.check_output',
                        side_effect=['0x0'.encode(),
                                     '0x0'.encode(),
                                     ''.encode(),
                                     ''.encode(),
                                     ''.encode(),
                                     ''.encode()]):
            with self.assertRaises(OSError):
                do_rsu(do_rsu_rsu_type, do_rsu_device, do_rsu_boot_page)
