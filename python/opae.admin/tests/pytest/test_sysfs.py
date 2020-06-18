import os
import random
import shutil
import tempfile
import unittest
from unittest import mock
from opae.admin.sysfs import (class_node,
                              pci_node,
                              sysfs_node)
from opae.admin.fpga import fpga


class test_sysfs(unittest.TestCase):
    def setUp(self):
        self._device = fpga.enum()[0]
        self._tmpdir = tempfile.mkdtemp()
        self._dirs = ['one', 'two', 'three']
        self._files = ['a', 'b', 'c']

        for d in self._dirs:
            dirpath = os.path.join(self._tmpdir, d)
            os.makedirs(dirpath)
            for f in self._files:
                with open(os.path.join(dirpath, f), 'w') as fp:
                    fp.write(f'{os.path.join(d, f)}\n')
        self._sysfs_node = sysfs_node(self._tmpdir)

    def tearDown(self):
        shutil.rmtree(self._tmpdir)

    '''test_node'''

    def test_node(self):
        sysfs_node_test = self._device
        sysfs_node_test.node()
        with self.assertRaises(NameError):
            sysfs_node_test.node('not-found')

    '''test_have_node'''

    def test_have_node(self):
        for d in self._dirs:
            self.assertTrue(self._sysfs_node.have_node(d))

    '''test_find'''

    def test_find(self):
        found = self._sysfs_node.find(self._dirs[0])
        self.assertIsInstance(next(found), sysfs_node)
        with self.assertRaises(StopIteration):
            next(found)

    '''test_find_all'''

    def test_find_all(self):
        self.assertEquals(len(self._sysfs_node.find_all('*')),
                          len(self._dirs))

    '''test_find_one'''

    def test_find_one(self):
        self.assertIsInstance(self._sysfs_node.find_one(self._dirs[0]),
                              sysfs_node)

    '''test_value_get'''

    def test_value_get(self):
        for d, f in zip(self._dirs, self._files):
            self.assertIsNotNone(self._sysfs_node.node(d, f).value)

    '''test_value_set'''

    def test_value_set(self):
        values = [random.randint(0, 10) for f in self._files]
        for d, f,v in zip(self._dirs, self._files, values):
            self._sysfs_node.node(d, f).value = v

        for d, f,v in zip(self._dirs, self._files, values):
            with open(os.path.join(self._tmpdir, d, f), 'r') as fp:
                self.assertEquals(int(fp.read().encode()), v)


    '''test_sysfs_path_get'''

    def test_sysfs_path_get(self):
        self.assertEquals(self._sysfs_node.sysfs_path, self._tmpdir)

    '''test_parse_address'''

    def test_parse_address(self):
        self.assertDictEqual(pci_node.parse_address('0000:ab:01.2'),
                             {'segment': '0000',
                              'bus': 'ab',
                              'device': '01',
                              'function': '2',
                              'pci_address': '0000:ab:01.2',
                              'bdf': 'ab:01.2'})

    '''test_tree'''

    def test_tree(self):
        pci_node_test = self._device.pci_node
        tree_level = mock.MagicMock()
        pci_node_test.tree(tree_level)

    '''test_root_get'''

    def test_root_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.root

    '''test_branch_get'''

    def test_branch_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.branch

    '''test_endpoints_get'''

    def test_endpoints_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.endpoints

    '''test_pci_address_get'''

    def test_pci_address_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.pci_address

    '''test_bdf_get'''

    def test_bdf_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.bdf

    '''test_segment_get'''

    def test_segment_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.segment

    '''test_domain_get'''

    def test_domain_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.domain

    '''test_bus_get'''

    def test_bus_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.bus

    '''test_device_get'''

    def test_device_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.device

    '''test_function_get'''

    def test_function_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.function

    '''test_parent_get'''

    def test_parent_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.parent

    '''test_parent_set'''

    def test_parent_set(self):
        pci_node_test = self._device.pci_node
        parent_value = mock.MagicMock()
        pci_node_test.parent = parent_value

    '''test_children_get'''

    def test_children_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.children

    '''test_all_children_get'''

    def test_all_children_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.all_children

    '''test_vendor_id_get'''

    def test_vendor_id_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.vendor_id

    '''test_device_id_get'''

    def test_device_id_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.device_id

    '''test_pci_id_get'''

    def test_pci_id_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.pci_id

    '''test_pci_bus_get'''

    def test_pci_bus_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.pci_bus

    '''test_remove'''

    def test_remove(self):
        pci_node_test = self._device.pci_node
        pci_node_test.remove()

    '''test_rescan'''

    def test_rescan(self):
        pci_node_test = self._device.pci_node
        pci_node_test.rescan()

    '''test_rescan_bus'''

    def test_rescan_bus(self):
        pci_node_test = self._device.pci_node
        with mock.patch.object(pci_node_test, 'node') as _p:
            pci_node_test.rescan_bus('3f', False)

    '''test_aer_get'''

    def test_aer_get(self):
        pci_node_test = self._device.pci_node
        with mock.patch('subprocess.check_output',
                        return_value='0xff'.encode()):
            self.assertEquals(pci_node_test.aer, (0xff, 0xff))

    '''test_aer_set'''

    def test_aer_set(self):
        pci_node_test = self._device.pci_node
        aer_value = 0xff, 0xff
        with mock.patch('subprocess.check_output',
                        return_value=''.encode()):
            pci_node_test.aer = aer_value

    '''test_sriov_totalvfs_get'''

    def test_sriov_totalvfs_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.sriov_totalvfs

    '''test_sriov_numvfs_get'''

    def test_sriov_numvfs_get(self):
        pci_node_test = self._device.pci_node
        _ = pci_node_test.sriov_numvfs

    '''test_sriov_numvfs_set'''

    def test_sriov_numvfs_set(self):
        self._device.pci_node.sriov_numvfs = 1

    '''test_supports_sriov_get'''

    def test_supports_sriov_get(self):
        _ = self._device.pci_node.supports_sriov

    '''test_enum_class'''

    def test_enum_class(self):
        class _fpga(class_node):
            pass

        results = _fpga.enum_class('fpga')
        self.assertNotEquals(results, [])

    '''test_enum'''

    def test_enum(self):
        class fpga_manager(class_node):
            pass

        self.assertNotEquals(fpga_manager.enum([]), [])
        self.assertNotEquals(fpga_manager.enum([{'pci_node.bus': '3f'}]), [])
