import json
import logging
import os
import signal
import tempfile
import unittest
from unittest import mock
from opae.admin.tools.fpgaotsu import (to_int,
                                       all_or_none,
                                       otsu_manifest_loader,
                                       otsu_updater,
                                       sig_handler,
                                       run_updaters)
from opae.admin.tools import fpgaotsu
from opae.admin.fpga import fpga, flash_control


class test_fpgaotsu(unittest.TestCase):
    def setUp(self):
        self._device = fpga.enum()[0]
        self._dummy_infile = tempfile.NamedTemporaryFile('w+')
        self._dummy_mtd = open('/dev/mtd0', 'w+b')
        self._dummy_mtd.truncate(128*1024)
        self._dummy_mtd.seek(0)
        self._dummy_flash_bin1 = tempfile.NamedTemporaryFile('w+b',
                                                             prefix='bin')
        self._dummy_json1 = {'requires': [],
                             'device': '0x09c4',
                             'product': 'dummy',
                             'version': 2,
                             'vendor': '0x8086',
                             'program': 'one-time-update',
                             'flash': []}
        self._dummy_flash1 = {'comment': 'mock',
                              'end': '0xff',
                              'erase_start': '0x0',
                              'erase_end': '0xff',
                              'filename': self._dummy_flash_bin1.name,
                              'seek': '0x0',
                              'start': '0x0',
                              'type': 'flash',
                              'verify': True}

        self._dummy_dir = os.path.dirname(self._dummy_infile.name)
        self._otsu_updater = otsu_updater(self._dummy_dir,
                                          self._device,
                                          self._dummy_json1)

    def tearDown(self):
        self._dummy_infile.close()
        self._dummy_mtd.close()
        self._dummy_flash_bin1.close()

    def test_sig_handler(self):
        with self.assertRaises(KeyboardInterrupt):
            sig_handler(signal.SIGTERM, None)

    '''test_to_int'''

    def test_to_int(self):
        self.assertEqual(to_int('0xFF'), (True, 255))
        self.assertEqual(to_int('1'), (True, 1))
        self.assertEqual(to_int(1), (True, 1))
        self.assertEqual(to_int('zx01'), (False, -1))

    '''test_all_or_none'''

    def test_all_or_none(self):
        all_or_none(self._dummy_flash1, 'filename', 'comment', 'start')
        with self.assertRaises(KeyError):
            all_or_none(self._dummy_flash1, 'filename', 'comment', 'mock')

    '''test_validate_mandatory_keys'''

    def test_validate_mandatory_keys(self):
        self._dummy_infile.truncate(0)
        json.dump(self._dummy_json1, self._dummy_infile)
        self._dummy_infile.flush()
        self._dummy_infile.seek(0)
        otsu_manifest_loader_test = otsu_manifest_loader(self._dummy_infile)
        otsu_manifest_loader_test.validate_mandatory_keys(self._dummy_json1)

    '''test_validate_requires_section'''

    def test_validate_requires_section(self):
        self._dummy_infile.truncate(0)
        json.dump(self._dummy_json1, self._dummy_infile)
        self._dummy_infile.flush()
        self._dummy_infile.seek(0)
        loader = otsu_manifest_loader(self._dummy_infile)
        self.assertIsNone(loader.validate_requires_section({'requires': []}))

        with self.assertRaises(TypeError):
            loader.validate_requires_section({'requires': 1})
        with self.assertRaises(TypeError):
            loader.validate_requires_section({'requires': [0, 1]})
        with self.assertRaises(ValueError):
            loader.validate_requires_section({'requires': ['m', 'o']})
        with self.assertRaises(ValueError):
            self.assertIsNone(loader.validate_requires_section(
                {'requires': ['mock == 1']}
            ))
        cmplist = ['bmcfw_version == 1.2.3', 'max10_version <= 2.3.4']
        self.assertIsNone(loader.validate_requires_section(
            {'requires': cmplist}
        ))

    '''test_validate_flash_section'''

    def test_validate_flash_section(self):
        otsu_manifest_loader_test = otsu_manifest_loader(self._dummy_infile)
        self._dummy_flash_bin1.truncate(1024)
        otsu_manifest_loader_test.validate_flash_section(
            self._dummy_flash1, self._dummy_dir)


        self._dummy_flash_bin1.truncate(0)
        with self.assertRaises(ValueError):
            otsu_manifest_loader_test.validate_flash_section(
                self._dummy_flash1, self._dummy_dir)



        self._dummy_flash_bin1.truncate(1024)
        _type = self._dummy_flash1.pop('type')
        with self.assertRaises(KeyError):
            otsu_manifest_loader_test.validate_flash_section(
                self._dummy_flash1, self._dummy_dir)

        self._dummy_flash1['type'] = _type
        self._dummy_flash1['erase_start'] = 'mock'
        with self.assertRaises(TypeError):
            otsu_manifest_loader_test.validate_flash_section(
                self._dummy_flash1, self._dummy_dir)

        self._dummy_flash1['erase_start'] = '0x0'
        self._dummy_flash1['erase_end'] = '0xZ0'
        with self.assertRaises(TypeError):
            otsu_manifest_loader_test.validate_flash_section(
                self._dummy_flash1, self._dummy_dir)



    '''test_load_and_validate'''

    def test_load_and_validate(self):
        json.dump(self._dummy_json1, self._dummy_infile)
        self._dummy_infile.flush()
        self._dummy_infile.seek(0)
        otsu_manifest_loader_test = otsu_manifest_loader(self._dummy_infile)
        otsu_manifest_loader_test.load_and_validate()

    '''test_load'''

    def test_load(self):
        json.dump(self._dummy_json1, self._dummy_infile)
        self._dummy_infile.flush()
        self._dummy_infile.seek(0)
        otsu_manifest_loader_test = otsu_manifest_loader(self._dummy_infile)
        otsu_manifest_loader_test.load()

    '''test_error_count_get'''

    def test_error_count_get(self):
        _ = self._otsu_updater.error_count

    '''test_error'''

    def test_error(self):
        init_fw_dir = mock.MagicMock()
        init_pac = mock.MagicMock()
        init_config = mock.MagicMock()
        init_chunk_size = mock.MagicMock()
        self._otsu_updater = otsu_updater(
            init_fw_dir, init_pac, init_config, init_chunk_size)
        error_msg = mock.MagicMock()
        self._otsu_updater.error(error_msg)

    '''test_log_errors'''

    def test_log_errors(self):
        init_fw_dir = mock.MagicMock()
        init_pac = mock.MagicMock()
        init_config = mock.MagicMock()
        init_chunk_size = mock.MagicMock()
        self._otsu_updater = otsu_updater(
            init_fw_dir, init_pac, init_config, init_chunk_size)
        log_errors_logfn = mock.MagicMock()
        self._otsu_updater.log_errors(log_errors_logfn)

    '''test_pac_get'''

    def test_pac_get(self):
        init_fw_dir = mock.MagicMock()
        init_pac = mock.MagicMock()
        init_config = mock.MagicMock()
        init_chunk_size = mock.MagicMock()
        self._otsu_updater = otsu_updater(
            init_fw_dir, init_pac, init_config, init_chunk_size)
        _ = self._otsu_updater.pac

    '''test_check_requires'''

    def test_check_requires(self):
        init_fw_dir = mock.MagicMock()
        init_pac = mock.MagicMock()
        init_config = mock.MagicMock()
        init_chunk_size = mock.MagicMock()
        self._otsu_updater = otsu_updater(
            init_fw_dir, init_pac, init_config, init_chunk_size)
        self._otsu_updater.check_requires()

    '''test_erase'''

    def test_erase(self):
        init_fw_dir = mock.MagicMock()
        init_pac = mock.MagicMock()
        init_config = mock.MagicMock()
        init_chunk_size = mock.MagicMock()
        self._otsu_updater = otsu_updater(
            init_fw_dir, init_pac, init_config, init_chunk_size)
        erase_obj = mock.MagicMock()
        erase_mtd_dev = mock.MagicMock()
        self._otsu_updater.erase(erase_obj, erase_mtd_dev)

    '''test_read_modify_write'''

    def test_read_modify_write(self):
        mock_mtd = mock.MagicMock()
        self._otsu_updater.read_modify_write(
            self._dummy_flash1, mock_mtd)

    '''test_write'''

    def test_write(self):
        write_mtd_dev = mock.MagicMock()
        self._otsu_updater.write(self._dummy_flash1, write_mtd_dev)

    '''test_verify'''

    def test_verify(self):
        mock_mtd = mock.MagicMock()
        self._otsu_updater.verify(
            self._dummy_flash1,
            mock_mtd,
            self._dummy_flash_bin1,
            True)

        self._otsu_updater.verify(
            self._dummy_flash1,
            mock_mtd,
            self._dummy_flash_bin1,
            False)

    '''test_wait'''

    def test_wait(self):
        self._otsu_updater._thread = mock.MagicMock()
        self._otsu_updater.wait()

    '''test_update'''

    def test_update(self):
        self._otsu_updater.update()

    '''test_process_flash_item'''

    def test_process_flash_item(self):
        fpgaotsu.LOG.setLevel(logging.DEBUG)
        with self.assertRaises(AttributeError):
            self._otsu_updater.process_flash_item([], self._dummy_flash1)

        mock_control = flash_control('flash')
        with self.assertRaises(IOError):
            with self.assertLogs():
                self._otsu_updater.process_flash_item(
                    [mock_control], self._dummy_flash1)

        mock_control = flash_control('flash', 'mtd0')
        self._dummy_flash_bin1.seek(0)
        with open('/dev/urandom', 'rb') as fp:
            self._dummy_flash_bin1.write(fp.read(1024))
        self._dummy_flash_bin1.flush()
        self._dummy_mtd.close()
        self._otsu_updater._errors = []
        with mock.patch('fcntl.ioctl'):
            self.assertTrue(self._otsu_updater.process_flash_item(
                [mock_control], self._dummy_flash1))

    '''test_run_updaters'''

    def test_run_updaters(self):
        run_updaters_updaters = mock.MagicMock()
        run_updaters(run_updaters_updaters)
