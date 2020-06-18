import unittest
from unittest import mock
from opae.admin.tools.fpgaport import (fpga_filter,
                                       set_numvfs,
                                       assign,
                                       release)
from opae.admin.fpga import fpga


class test_fpgaport(unittest.TestCase):
    def setUp(self):
        self._device = fpga.enum()[0]

    def tearDown(self):
        pass


    '''test_fpga_filter'''

    def test_fpga_filter(self):
        fpga_filter(self._device.pci_node.pci_address)

    '''test_set_numvfs'''

    def test_set_numvfs(self):
        set_numvfs(self._device, 1)

    '''test_assign'''

    def test_assign(self):
        assign_args = mock.MagicMock()
        assign_args.numvfs = 1
        assign_args.port = 0
        assign_args.destroy_vfs = False
        with mock.patch('fcntl.ioctl'):
            assign(assign_args, self._device)

        with mock.patch('fcntl.ioctl', side_effect=IOError('mock')):
            assign(assign_args, self._device)

        self._device.pci_node.sriov_numvfs = 1
        with mock.patch('fcntl.ioctl'):
            assign(assign_args, self._device)
        self._device.pci_node.sriov_numvfs = 0

    '''test_release'''

    def test_release(self):
        release_args = mock.MagicMock()
        release_args.destroy_vfs = False
        release_args.port = 0
        release_args.numvfs = 1
        with mock.patch('fcntl.ioctl'):
            release(release_args, self._device)

        with mock.patch('fcntl.ioctl', side_effect=IOError('mock')):
            release(release_args, self._device)

        release_args.destroy_vfs = True
        with mock.patch('fcntl.ioctl'):
            release(release_args, self._device)
