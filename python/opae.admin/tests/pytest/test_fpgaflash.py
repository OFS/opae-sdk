import contextlib
import io
import struct
import unittest
import tempfile
from subprocess import CalledProcessError
from unittest import mock
from opae.admin.tools import fpgaflash
from opae.admin.tools.fpgaflash import (assert_not_running,
                                        check_rpd,
                                        reverse_bits,
                                        reverse_bits_in_file,
                                        get_flash_size,
                                        flash_erase,
                                        flash_write,
                                        flash_read,
                                        get_pvid,
                                        get_bdf_pvid_mapping,
                                        get_bdf_mtd_mapping,
                                        get_bdf_spi_mapping,
                                        print_bdf_mtd_mapping,
                                        normalize_bdf,
                                        update_flash,
                                        fpga_update,
                                        get_dev_bmc,
                                        BittwareBmc,
                                        bmc_update,
                                        write_file,
                                        get_mtd_from_spi_path,
                                        bmc_fw_wait_for_nios,
                                        bmc_fw_update,
                                        validate_bdf,
                                        rc_fpga_update,
                                        get_aer,
                                        set_aer,
                                        get_upstream_bdf,
                                        rescan_pci_bus,
                                        dc_vc_reconfigure,
                                        board_rsu,
                                        dc_vc_fpga_update,
                                        vc_update_eeprom,
                                        dc_update_eeprom,
                                        get_flash_mode,
                                        check_file_dc,
                                        check_file,
                                        check_file_extension,
                                        vc_phy_eeprom_update)
from opae.admin.fpga import fpga


class test_fpgaflash(unittest.TestCase):
    def setUp(self):
        self._device = fpga.enum()[0]
        self._mtd0 = open('/dev/mtd0', 'wb+')
        self._infile = tempfile.NamedTemporaryFile('wb+')
        self._outfile = tempfile.NamedTemporaryFile('wb+')
        with open('/dev/urandom', 'rb') as fp:
            self._infile.write(fp.read(1024))
            self._mtd0.write(fp.read(1024))

        self._hexfile = tempfile.NamedTemporaryFile('w+')
        self._hexfile.write(':10010000214601360121470136007EFE09D2190140\n')
        self._hexfile.write(':100110002146017E17C20001FF5F16002148011928\n')
        self._hexfile.write(':10012000194E79234623965778239EDA3F01B2CAA7\n')
        self._hexfile.write(':100130003F0156702B5E712B722B732146013421C7\n')
        self._hexfile.write(':00000001FF\n')
        self._hexfile.seek(0)
        self._mtd0.seek(0)
        self._infile.seek(0)
        self._sleep = mock.patch('time.sleep')
        self._sleep.start()

    def tearDown(self):
        self._mtd0.close()
        self._infile.close()
        self._outfile.close()
        self._hexfile.close()
        self._sleep.stop()

    '''test_assert_not_running'''

    def test_assert_not_running(self):
        assert_not_running_programs = ['fpgaflash']

        with mock.patch('subprocess.run') as _p:
            with self.assertRaises(SystemExit):
                assert_not_running(assert_not_running_programs)
            self.assertListEqual(['pidof'] + assert_not_running_programs,
                                 _p.call_args[0][0])

        with mock.patch('subprocess.run',
                        side_effect=CalledProcessError(-1, '')) as _p:
            _p.configure_mock(stdout='')
            assert_not_running(assert_not_running_programs)
            self.assertListEqual(['pidof'] + assert_not_running_programs,
                                 _p.call_args[0][0])

        # with mock.patch('subprocess.check_output',
        #                 side_effect=subprocess.CalledProcessError):
        #     assert_not_running(assert_not_running_programs)

        # with mock.patch('subprocess.check_output') as _p:
        #     assert_not_running(assert_not_running_programs)
        #     _p.assert_called_with(['pidof', 'fpgaflash'])

    '''test_check_rpd'''

    def test_check_rpd(self):
        self._infile.truncate(0x20)
        self._infile.seek(0)
        ffs = [0xffffffff]*3 + [0x6a6a6a6a]
        self._infile.write(struct.pack('IIII',*ffs))
        self._infile.flush()
        self._infile.seek(0)
        check_rpd(self._infile)

    '''test_reverse_bits'''

    def test_reverse_bits(self):
        self.assertEquals(reverse_bits(0b1010101010), 0b0101010101)

    '''test_reverse_bits_in_file'''

    def test_reverse_bits_in_file(self):
        reverse_bits_in_file_ifile = self._infile
        reverse_bits_in_file_ofile = tempfile.NamedTemporaryFile('wb+')
        reverse_bits_in_file(reverse_bits_in_file_ifile,
                             reverse_bits_in_file_ofile)

    '''test_get_flash_size'''

    def test_get_flash_size(self):
        data = struct.pack('BIIIIIQ', 0, 0, 128, 0, 0, 0, 0)
        with mock.patch('fcntl.ioctl', return_value=data):
            self.assertEqual(get_flash_size(self._mtd0.name), 128)

    '''test_flash_erase'''

    def test_flash_erase(self):
        with mock.patch('fcntl.ioctl'):
            flash_erase(self._mtd0.name, 0x0, 0x100)

    '''test_flash_write'''

    def test_flash_write(self):
        flash_write_dev = self._mtd0.name
        flash_write_start = 0
        flash_write_nbytes = 1024
        flash_write_ifile = self._infile
        flash_write(
            flash_write_dev,
            flash_write_start,
            flash_write_nbytes,
            flash_write_ifile)

    '''test_flash_read'''

    def test_flash_read(self):
        flash_read_dev = self._mtd0.name
        flash_read_start = 0
        flash_read_nbytes = 1024
        flash_read_ofile = self._outfile
        flash_read(
            flash_read_dev,
            flash_read_start,
            flash_read_nbytes,
            flash_read_ofile)

    '''test_get_pvid'''

    def test_get_pvid(self):
        get_pvid_bdf = '0000:01:23.4'
        data = ['0x8086', '0x0b30']
        class _f(io.StringIO):
            def __init__(self, *args):
                super(_f, self).__init__()

            def read(self, *args):
                return data.pop(0)

        with mock.patch('builtins.open', mock.mock_open(mock=_f)):
            self.assertEquals(get_pvid(get_pvid_bdf), '0x8086:0x0b30')

    '''test_get_bdf_pvid_mapping'''

    def test_get_bdf_pvid_mapping(self):
        get_bdf_pvid_mapping()

    '''test_get_bdf_mtd_mapping'''

    def test_get_bdf_mtd_mapping(self):
        get_bdf_mtd_mapping()

    '''test_get_bdf_spi_mapping'''

    def test_get_bdf_spi_mapping(self):
        get_bdf_spi_mapping()

    '''test_print_bdf_mtd_mapping'''

    def test_print_bdf_mtd_mapping(self):
        with self.assertRaises(SystemExit):
            with mock.patch('sys.stdout', new=io.StringIO()) as _stdout:
                print_bdf_mtd_mapping({'key':'value'})
                self.assertEqual(_stdout.getvalue(), '    key\n')

    '''test_normalize_bdf'''

    def test_normalize_bdf(self):
        bdf = '0000:01:23.4'
        self.assertEqual(normalize_bdf(bdf), bdf)
        self.assertEqual(normalize_bdf('01:23.4'), bdf)

    '''test_update_flash'''

    def test_update_flash(self):
        update_flash_ifile = self._infile
        update_flash_mtd_dev = self._mtd0.name
        update_flash_target_offset = mock.MagicMock()
        update_flash_input_offset = mock.MagicMock()
        update_flash_erase_len = mock.MagicMock()
        update_flash_no_verify = mock.MagicMock()
        with mock.patch('fcntl.ioctl'):
            update_flash(
                update_flash_ifile,
                update_flash_mtd_dev,
                update_flash_target_offset,
                update_flash_input_offset,
                update_flash_erase_len,
                update_flash_no_verify)

    '''test_fpga_update'''

    def test_fpga_update(self):
        fpga_update_ifile = self._infile
        fpga_update_mtd_dev = self._mtd0.name
        fpga_update_target_offset = 128
        fpga_update_input_offset = 128
        fpga_update_erase_len = 100
        fpga_update_no_verify = False
        with mock.patch('stat.S_ISCHR', return_value=True):
            with mock.patch('fcntl.ioctl'):
                fpga_update(fpga_update_ifile,
                            fpga_update_mtd_dev,
                            fpga_update_target_offset,
                            fpga_update_input_offset,
                            fpga_update_erase_len,
                            fpga_update_no_verify)

    '''test_get_dev_bmc'''

    def _not_get_dev_bmc(self):
        get_dev_bmc_bdf = mock.MagicMock()
        get_dev_bmc(get_dev_bmc_bdf)

    '''test_verify_segments'''

    def _not_verify_segments(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        verify_segments_utype = mock.MagicMock()
        BittwareBmc_test.verify_segments(verify_segments_utype)

    '''test_bw_xact'''

    def _not_bw_xact(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        bw_xact_txarray = mock.MagicMock()
        bw_xact_rxlen = mock.MagicMock()
        BittwareBmc_test.bw_xact(bw_xact_txarray, bw_xact_rxlen)

    '''test_bw_bl_xact'''

    def _not_bw_bl_xact(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        bw_bl_xact_bltx = mock.MagicMock()
        bw_bl_xact_blrx = mock.MagicMock()
        BittwareBmc_test.bw_bl_xact(bw_bl_xact_bltx, bw_bl_xact_blrx)

    '''test_bl_version'''

    def _not_bl_version(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        BittwareBmc_test.bl_version()

    '''test_bl_jump_other'''

    def _not_bl_jump_other(self):
        init_dev_path = self._mtd0.name
        init_ifile = self._infile
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        bl_jump_other_app = mock.MagicMock()
        BittwareBmc_test.bl_jump_other(bl_jump_other_app)

    '''test_bl_read'''

    def _not_bl_read(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        bl_read_device = mock.MagicMock()
        bl_read_offset = mock.MagicMock()
        bl_read_count = mock.MagicMock()
        BittwareBmc_test.bl_read(bl_read_device, bl_read_offset, bl_read_count)

    '''test_bl_write'''

    def _not_bl_write(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        bl_write_device = mock.MagicMock()
        bl_write_offset = mock.MagicMock()
        bl_write_txdata = mock.MagicMock()
        BittwareBmc_test.bl_write(
            bl_write_device,
            bl_write_offset,
            bl_write_txdata)

    '''test_verify_partition'''

    def _not_verify_partition(self):
        init_dev_path = self._mtd0.name
        self._hexfile.seek(0)
        init_ifile = self._hexfile.name
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        verify_partition_part = 1
        BittwareBmc_test.verify_partition(verify_partition_part)

    '''test_verify_partitions'''

    def _not_verify_partitions(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        verify_partitions_utype = mock.MagicMock()
        BittwareBmc_test.verify_partitions(verify_partitions_utype)

    '''test_write_range'''

    def _not_write_range(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        write_range_ih = mock.MagicMock()
        write_range_start = mock.MagicMock()
        write_range_end = mock.MagicMock()
        BittwareBmc_test.write_range(
            write_range_ih,
            write_range_start,
            write_range_end)

    '''test_write_page0'''

    def _not_write_page0(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        BittwareBmc_test.write_page0()

    '''test_write_partitions'''

    def _not_write_partitions(self):
        init_dev_path = mock.MagicMock()
        init_ifile = mock.MagicMock()
        BittwareBmc_test = BittwareBmc(init_dev_path, init_ifile)
        write_partitions_utype = mock.MagicMock()
        BittwareBmc_test.write_partitions(write_partitions_utype)

    '''test_bmc_update'''

    def _not_bmc_update(self):
        bmc_update_utype = mock.MagicMock()
        bmc_update_ifile = mock.MagicMock()
        bmc_update_bdf = mock.MagicMock()
        bmc_update(bmc_update_utype, bmc_update_ifile, bmc_update_bdf)

    '''test_write_file'''

    def test_write_file(self):
        self._infile.truncate(0)
        write_file(self._infile.name, '100')
        self._infile.seek(0)
        self.assertEquals(self._infile.read().decode(), '100')

    '''test_get_mtd_from_spi_path'''

    def test_get_mtd_from_spi_path(self):
        with mock.patch('builtins.open', mock.mock_open()):
            with mock.patch('glob.glob', return_value=['mtd0']):
                get_mtd_from_spi_path('', '')

    '''test_bmc_fw_wait_for_nios'''

    def test_bmc_fw_wait_for_nios(self):
        self.skipTest('TODO')
        bmc_fw_wait_for_nios_ver_file = mock.MagicMock()
        bmc_fw_wait_for_nios(bmc_fw_wait_for_nios_ver_file)

    '''test_bmc_fw_update'''

    def test_bmc_fw_update(self):
        with mock.patch('builtins.open', mock.mock_open()):
            with mock.patch('glob.glob', return_value=['mtd0']):
                with mock.patch('fcntl.ioctl'):
                    bmc_fw_update(
                        self._hexfile,
                        self._device.fme.spi_bus.sysfs_path,
                        True,
                        True)

    '''test_validate_bdf'''

    def test_validate_bdf(self):
        with self.assertRaises(SystemExit):
            validate_bdf('', {'0000:01:23.4': 0x0b30})

        with self.assertRaises(SystemExit):
            validate_bdf('', {'0000:01:23.4': 0x0b30,
                              '0000:01:23.5': 0x0b30})

        self.assertEqual(validate_bdf('01:23.4', {'0000:01:23.4':
                                                  '0x8086:0x0b30'}),
                         '0000:01:23.4')
        with self.assertRaises(SystemExit):
            validate_bdf('nothere', {'0000:01:23.4': '0x8086:0x0b30'})

    '''test_rc_fpga_update'''

    def test_rc_fpga_update(self):
        self.skipTest('TODO')
        rc_fpga_update_ifile = mock.MagicMock()
        rc_fpga_update_utype = mock.MagicMock()
        rc_fpga_update_bdf = mock.MagicMock()
        rc_fpga_update_no_verify = mock.MagicMock()
        rc_fpga_update(
            rc_fpga_update_ifile,
            rc_fpga_update_utype,
            rc_fpga_update_bdf,
            rc_fpga_update_no_verify)

    '''test_get_aer'''

    def test_get_aer(self):
        with mock.patch('subprocess.check_output',
                        return_value='0x1234'.encode()):
            self.assertEquals(get_aer('0000:01:23.4'), [0x1234, 0x1234])

    '''test_set_aer'''

    def test_set_aer(self):
        with mock.patch('subprocess.check_output',
                        side_effect=[''.encode(), ''.encode()]):
            set_aer('0000:01:23.4', 0x1234, 0x1234)

    '''test_get_upstream_bdf'''

    def test_get_upstream_bdf(self):
        link = ('../../../../devices/pci0000:3a/0000:3a:00.0/0000:3b:00.0/'
                '0000:3c:09.0/0000:3f:00.0')
        with mock.patch('os.readlink', return_value=link):
            get_upstream_bdf(self._device.pci_node.pci_address)

    '''test_rescan_pci_bus'''

    def test_rescan_pci_bus(self):
        self.skipTest('not needed')
        rescan_pci_bus(self._device.pci_node.root.pci_address,
                       self._device.pci_node.root.pci_bus)

    '''test_dc_vc_reconfigure'''

    def test_dc_vc_reconfigure(self):
        dc_vc_reconfigure_bdf = self._device.pci_node.pci_address
        dc_vc_reconfigure_ld_path = self._infile.name
        dc_vc_reconfigure_boot_page = 0
        link = ('../../../../devices/pci0000:3a/0000:3a:00.0/0000:3b:00.0/'
                '0000:3c:09.0/0000:3f:00.0')
        with mock.patch('os.readlink', return_value=link):
            with mock.patch('subprocess.check_output', return_value='0x0'):
                with mock.patch('subprocess.check_call'):
                    with mock.patch.object(fpgaflash, 'rescan_pci_bus'):
                        dc_vc_reconfigure(dc_vc_reconfigure_bdf,
                                          dc_vc_reconfigure_ld_path,
                                          dc_vc_reconfigure_boot_page)

    '''test_board_rsu'''

    def test_board_rsu(self):
        link = ('../../../../devices/pci0000:3a/0000:3a:00.0/0000:3b:00.0/'
                '0000:3c:09.0/0000:3f:00.0')
        with mock.patch('os.readlink', return_value=link):
            with mock.patch('subprocess.check_output', return_value='0x0'):
                with mock.patch('subprocess.check_call'):
                    with mock.patch.object(fpgaflash, 'rescan_pci_bus'):
                        board_rsu(self._device.pci_node.pci_address,
                                    self._device.fme.spi_bus.sysfs_path,
                                    0)

    '''test_dc_vc_fpga_update'''

    def test_dc_vc_fpga_update(self):
        real_open = open

        @contextlib.contextmanager
        def _open(path, *args, **kwargs):
            try:
                if path.endswith('bmcimg_flash_mode'):
                    fp = io.StringIO()
                else:
                    fp = real_open(path, *args, **kwargs)
                yield fp
            finally:
                fp.close()

        link = ('../../../../devices/pci0000:3a/0000:3a:00.0/0000:3b:00.0/'
                '0000:3c:09.0/0000:3f:00.0')
        with mock.patch('os.readlink', return_value=link):
            with mock.patch('builtins.open', new=_open):
                with mock.patch('glob.glob', return_value=['mtd0']):
                    dc_vc_fpga_update(
                        self._infile,
                        'bmc_img',
                        self._device.pci_node.pci_address,
                        self._device.fme.spi_bus.sysfs_path,
                        False,
                        0,
                        False)

    '''test_vc_update_eeprom'''

    def test_vc_update_eeprom(self):
        self.skipTest('TODO')
        vc_update_eeprom_ifile = mock.MagicMock()
        vc_update_eeprom_bdf = mock.MagicMock()
        vc_update_eeprom(vc_update_eeprom_ifile, vc_update_eeprom_bdf)

    '''test_dc_update_eeprom'''

    def test_dc_update_eeprom(self):
        self.skipTest('TODO')
        dc_update_eeprom_ifile = mock.MagicMock()
        dc_update_eeprom_spi_path = mock.MagicMock()
        dc_update_eeprom(dc_update_eeprom_ifile, dc_update_eeprom_spi_path)

    '''test_get_flash_mode'''

    def test_get_flash_mode(self):
        with mock.patch('glob.glob'):
            with mock.patch('builtins.open', mock.mock_open(read_data='1')):
                self.assertEqual(get_flash_mode(''), 1)
            with mock.patch('builtins.open', mock.mock_open(read_data='0')):
                self.assertEqual(get_flash_mode(''), 0)

    '''test_check_file_dc'''

    def test_check_file_dc(self):
        self.skipTest('TODO')
        check_file_dc_ifile = mock.MagicMock()
        check_file_dc(check_file_dc_ifile)

    '''test_check_file'''

    def test_check_file(self):
        check_file_ifile = mock.MagicMock()
        check_file_utype = mock.MagicMock()
        check_file(check_file_ifile, check_file_utype)

    '''test_check_file_extension'''

    def test_check_file_extension(self):
        check_file_extension_ifile = mock.MagicMock()
        check_file_extension_utype = mock.MagicMock()
        check_file_extension(
            check_file_extension_ifile,
            check_file_extension_utype)

    '''test_vc_phy_eeprom_update'''

    def test_vc_phy_eeprom_update(self):
        self.skipTest('TODO')
        vc_phy_eeprom_update_ifile = mock.MagicMock()
        vc_phy_eeprom_update_spi_path = mock.MagicMock()
        vc_phy_eeprom_update_no_verify = mock.MagicMock()
        vc_phy_eeprom_update_no_nios_release = mock.MagicMock()
        vc_phy_eeprom_update(
            vc_phy_eeprom_update_ifile,
            vc_phy_eeprom_update_spi_path,
            vc_phy_eeprom_update_no_verify,
            vc_phy_eeprom_update_no_nios_release)
