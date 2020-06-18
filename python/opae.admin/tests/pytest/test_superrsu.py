import json
import os
import tempfile
import unittest
from unittest import mock

from opae.admin.tools.super_rsu import (sys_exit,
                                        trace,
                                        update_thread,
                                        process_task,
                                        ignore_signals,
                                        find_subdevices,
                                        flashable,
                                        bmc_img,
                                        bmc_pkg,
                                        a10,
                                        dtb,
                                        bmc_fw_pkvl,
                                        eth_instance,
                                        pac,
                                        make_pac,
                                        discover_boards,
                                        do_rsu,
                                        get_update_threads,
                                        update_wait,
                                        do_verify,
                                        run_tests,
                                        need_requires,
                                        check_requirements,
                                        find_config)
from opae.admin.fpga import fpga


class test_super_rsu(unittest.TestCase):
    def setUp(self):
        self._devices = fpga.enum()
        self._device = self._devices[0]
        self._boards = [pac(d.pci_node, d) for d in self._devices]
        self._rsu_config = {'version': 4,
                            'product': 'n3000',
                            'vendor': '0x8086',
                            'program': 'super-rsu',
                            'configuration': '2x1x25G',
                            'flash': [{
                                    'enabled': True,
                                    'filename': 'dummy_bmcfw.bin',
                                    'secure': True,
                                    'timeout': '15m',
                                    'type': 'bmc_pkg',
                                    'version': ['2.0.6', '2.0.16']
                                }]}
        self._args = mock.MagicMock()
        self._args.rsu_config = tempfile.NamedTemporaryFile('w+')
        self._tempdir = os.path.dirname(self._args.rsu_config.name)
        self._dummy_bmcfw = open(
            os.path.join(self._tempdir, 'dummy_bmcfw.bin'), 'wb+')
        with open('/dev/urandom', 'rb') as fp:
            self._dummy_bmcfw.write(fp.read(4096))
        self._args.timeout = 1
        json.dump(self._rsu_config, self._args.rsu_config)
        self._args.rsu_config.seek(0)

    def tearDown(self):
        self._args.rsu_config.close()

    '''test_sys_exit'''

    def test_sys_exit(self):
        sys_exit_code = os.EX_IOERR
        sys_exit_msg = 'this is a test'
        with mock.patch('sys.exit') as m:
            sys_exit(sys_exit_code, sys_exit_msg)
            m.assert_called_with(os.EX_IOERR)

    '''test_trace'''

    def test_trace(self):
        trace_msg = "This is a trace message"
        trace(trace_msg)

    '''test_board_get'''

    def test_board_get(self):
        init_board = mock.MagicMock()
        init_fn = mock.MagicMock()
        update_thread_test = update_thread(init_board, init_fn)
        _ = update_thread_test.board

    '''test_timeout_get'''

    def test_timeout_get(self):
        init_board = self._boards[0]
        init_fn = mock.MagicMock()
        update_thread_test = update_thread(init_board, init_fn)
        _ = update_thread_test.timeout

    '''test_terminate'''

    def test_terminate(self):
        init_cmd = mock.MagicMock()
        process_task_test = process_task(init_cmd)
        terminate_timeout = mock.MagicMock()
        process_task_test.terminate(terminate_timeout)

    '''test_cmd_get'''

    def test_cmd_get(self):
        init_cmd = mock.MagicMock()
        process_task_test = process_task(init_cmd)
        _ = process_task_test.cmd

    '''test_start_time_get'''

    def test_start_time_get(self):
        init_cmd = mock.MagicMock()
        process_task_test = process_task(init_cmd)
        _ = process_task_test.start_time

    '''test_ignore_signals'''

    def test_ignore_signals(self):
        with mock.patch('signal.signal'):
            ignore_signals()

    '''test_find_subdevices'''

    def test_find_subdevices(self):
        find_subdevices_node = mock.MagicMock()
        find_subdevices(find_subdevices_node)

    '''test_can_verify_get'''

    def test_can_verify_get(self):
        init_fpga = mock.MagicMock()
        init_is_factory = mock.MagicMock()
        flashable_test = flashable(init_fpga, init_is_factory)
        _ = flashable_test.can_verify

    '''test_image_load_get'''

    def test_image_load_get(self):
        a10_test = a10(self._device)
        _ = a10_test.image_load

    '''test_needs_flash'''

    def test_needs_flash(self):
        dtb_test = dtb(self._device)
        needs_flash_flash_info = mock.MagicMock()
        dtb_test.needs_flash(needs_flash_flash_info)

    '''test_is_supported'''

    def test_is_supported(self):
        init_fpga = self._device
        init_is_factory = False
        bmc_pkg_test = bmc_pkg(init_fpga, init_is_factory)
        is_supported_flash_info = self._rsu_config['flash'][0]
        bmc_pkg_test.is_supported(is_supported_flash_info)

    '''test_command'''

    def test_command(self):
        bmc_fw_pkvl_test = bmc_fw_pkvl(self._device)
        command_flash_info = self._rsu_config['flash'][0]
        command_filename = self._rsu_config['flash'][0]['filename']
        command_pci_address = self._device.pci_node.pci_address
        bmc_fw_pkvl_test.command(
            command_flash_info,
            command_filename,
            command_pci_address)

    '''test_version_get'''

    def test_version_get(self):
        init_node = mock.MagicMock()
        init_version = mock.MagicMock()
        init_address = mock.MagicMock()
        init_node.attrib = {'bus': '63', 'subdevice': '0', 'func': '0'}
        eth_instance_test = eth_instance(init_node, init_version, init_address)
        _ = eth_instance_test.version

    '''test_is_factory_get'''

    def test_is_factory_get(self):
        init_fpga = mock.MagicMock()
        init_is_factory = mock.MagicMock()
        flashable_test = flashable(init_fpga, init_is_factory)
        _ = flashable_test.is_factory

    '''test_version_path_get'''

    def test_version_path_get(self):
        bmc_img_test = bmc_img(self._boards[0])
        _ = bmc_img_test.version_path

    '''test_bus_get'''

    def test_bus_get(self):
        init_node = mock.MagicMock()
        init_version = mock.MagicMock()
        init_address = mock.MagicMock()
        init_node.attrib = {'bus': '63', 'subdevice': '0', 'func': '0'}
        eth_instance_test = eth_instance(init_node, init_version, init_address)
        _ = eth_instance_test.bus

    '''test_device_get'''

    def test_device_get(self):
        init_node = mock.MagicMock()
        init_version = mock.MagicMock()
        init_address = mock.MagicMock()
        init_node.attrib = {'bus': '63', 'subdevice': '0', 'func': '0'}
        eth_instance_test = eth_instance(init_node, init_version, init_address)
        _ = eth_instance_test.device

    '''test_function_get'''

    def test_function_get(self):
        init_node = mock.MagicMock()
        init_version = mock.MagicMock()
        init_address = mock.MagicMock()
        init_node.attrib = {'bus': '63', 'subdevice': '0', 'func': '0'}
        eth_instance_test = eth_instance(init_node, init_version, init_address)
        _ = eth_instance_test.function

    '''test_check'''

    def test_check(self):
        init_node = mock.MagicMock()
        init_version = mock.MagicMock()
        init_address = mock.MagicMock()
        init_node.attrib = {'bus': '63', 'subdevice': '0', 'func': '0'}
        eth_instance_test = eth_instance(init_node, init_version, init_address)
        eth_instance_test.check()

    '''test_add_flashables'''

    def test_add_flashables(self):
        init_pci_node = mock.MagicMock()
        init_fpga = mock.MagicMock()
        pac_test = pac(init_pci_node, init_fpga)
        pac_test.add_flashables()

    '''test_get_flashable'''

    def test_get_flashable(self):
        init_pci_node = mock.MagicMock()
        init_fpga = mock.MagicMock()
        pac_test = pac(init_pci_node, init_fpga)
        get_flashable_flash_type = mock.MagicMock()
        pac_test.get_flashable(get_flashable_flash_type)

    '''test_errors_get'''

    def test_errors_get(self):
        init_pci_node = mock.MagicMock()
        init_fpga = mock.MagicMock()
        pac_test = pac(init_pci_node, init_fpga)
        _ = pac_test.errors

    '''test_pci_node_get'''

    def test_pci_node_get(self):
        init_pci_node = mock.MagicMock()
        init_fpga = mock.MagicMock()
        pac_test = pac(init_pci_node, init_fpga)
        _ = pac_test.pci_node

    '''test_pci_node_set'''

    def test_pci_node_set(self):
        init_pci_node = mock.MagicMock()
        init_fpga = mock.MagicMock()
        pac_test = pac(init_pci_node, init_fpga)
        pci_node_value = mock.MagicMock()
        pac_test.pci_node = pci_node_value

    '''test_fpga_get'''

    def test_fpga_get(self):
        init_pci_node = mock.MagicMock()
        init_fpga = mock.MagicMock()
        pac_test = pac(init_pci_node, init_fpga)
        _ = pac_test.fpga

    '''test_flash_task'''

    def test_flash_task(self):
        pac_test = self._boards[0]
        flash_task_flash_dir = self._tempdir
        flash_task_flash_info = self._rsu_config['flash'][0]
        flash_task_args = self._args
        pac_test.flash_task(
            flash_task_flash_dir,
            flash_task_flash_info,
            flash_task_args)

    '''test_reset_flash_mode'''

    def test_reset_flash_mode(self):
        init_pci_node = mock.MagicMock()
        init_fpga = mock.MagicMock()
        pac_test = pac(init_pci_node, init_fpga)
        pac_test.reset_flash_mode()

    '''test_update'''

    def test_update(self):
        pac_test = self._boards[0]
        update_flash_dir = os.path.dirname(self._args.rsu_config.name)
        update_rsu_config = self._rsu_config
        update_args = self._args
        pac_test.update(update_flash_dir, update_rsu_config, update_args)

    '''test_rsu'''

    def test_rsu(self):
        pac_test = self._boards[0]
        rsu_boot_page = 0
        pac_test.rsu(rsu_boot_page)

    '''test_terminate_update'''

    def test_terminate_update(self):
        init_pci_node = mock.MagicMock()
        init_fpga = mock.MagicMock()
        pac_test = pac(init_pci_node, init_fpga)
        pac_test.terminate_update()

    '''test_verify'''

    def test_verify(self):
        pac_test = self._boards[0]
        verify_rsu_config = self._rsu_config
        verify_args = self._args
        pac_test.verify(verify_rsu_config, verify_args)

    '''test_run_tests'''

    def test_run_tests(self):
        run_tests_boards = mock.MagicMock()
        run_tests_args = mock.MagicMock()
        run_tests_rsu_config = mock.MagicMock()
        run_tests(run_tests_boards, run_tests_args, run_tests_rsu_config)

    '''test_is_secure_get'''

    def test_is_secure_get(self):
        pac_test = self._boards[0]
        _ = pac_test.is_secure

    '''test_make_pac'''

    def test_make_pac(self):
        make_pac_node = mock.MagicMock()
        make_pac_fpga = mock.MagicMock()
        make_pac(make_pac_node, make_pac_fpga)

    '''test_discover_boards'''

    def test_discover_boards(self):
        discover_boards_rsu_config = self._rsu_config
        discover_boards_args = self._args
        discover_boards(discover_boards_rsu_config, discover_boards_args)

    '''test_do_rsu'''

    def test_do_rsu(self):
        do_rsu_boards = mock.MagicMock()
        do_rsu_args = mock.MagicMock()
        do_rsu_config = mock.MagicMock()
        do_rsu(do_rsu_boards, do_rsu_args, do_rsu_config)

    '''test_get_update_threads'''

    def test_get_update_threads(self):
        get_update_threads_boards = self._boards
        get_update_threads_args = self._args
        get_update_threads_rsu_config = self._rsu_config
        get_update_threads(
            get_update_threads_boards,
            get_update_threads_args,
            get_update_threads_rsu_config)

    '''test_update_wait'''

    def test_update_wait(self):
        test_thread = mock.MagicMock()
        test_thread.timeout = 1
        test_thread.name = 'test_thread'
        update_wait_threads = [test_thread]
        update_wait_args = self._args
        update_wait_rsu_config = self._rsu_config
        update_wait(
            update_wait_threads,
            update_wait_args,
            update_wait_rsu_config)

    '''test_do_verify'''

    def test_do_verify(self):
        do_verify_boards = mock.MagicMock()
        do_verify_args = mock.MagicMock()
        do_verify_rsu_config = mock.MagicMock()
        do_verify(do_verify_boards, do_verify_args, do_verify_rsu_config)

    '''test_need_requires'''

    def test_need_requires(self):
        need_requires_boards = mock.MagicMock()
        need_requires_flash_spec = mock.MagicMock()
        need_requires_comparator = mock.MagicMock()
        need_requires(
            need_requires_boards,
            need_requires_flash_spec,
            need_requires_comparator)

    '''test_check_requirements'''

    def test_check_requirements(self):
        check_requirements_boards = self._boards
        check_requirements_args = self._args
        check_requirements_rsu_config = self._rsu_config
        check_requirements(
            check_requirements_boards,
            check_requirements_args,
            check_requirements_rsu_config)

    '''test_find_config'''

    def test_find_config(self):
        find_config_program = mock.MagicMock()
        find_config_configuration = mock.MagicMock()
        find_config(find_config_program, find_config_configuration)
