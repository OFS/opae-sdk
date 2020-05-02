import unittest
from unittest import mock
from pacsign.reader import (_READER_BASE,
                            CANCEL_reader,
                            UPDATE_reader,
                            RHP_reader)
'''test_is_Darby_PR'''

def test_is_Darby_PR():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    is_Darby_PR_contents = mock.MagicMock()
    is_Darby_PR_offset = mock.MagicMock()
    _READER_BASE_test.is_Darby_PR(is_Darby_PR_contents, is_Darby_PR_offset)

'''test_finalize'''

def test_finalize():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    finalize_fd = mock.MagicMock()
    finalize_block0 = mock.MagicMock()
    finalize_block1 = mock.MagicMock()
    finalize_payload = mock.MagicMock()
    _READER_BASE_test.finalize(finalize_fd, finalize_block0, finalize_block1, finalize_payload)

'''test_make_block0'''

def test_make_block0():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_block0_payload = mock.MagicMock()
    make_block0_payload_size = mock.MagicMock()
    _READER_BASE_test.make_block0(make_block0_payload, make_block0_payload_size)

'''test_make_block0_dc'''

def test_make_block0_dc():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_block0_dc_payload = mock.MagicMock()
    make_block0_dc_payload_size = mock.MagicMock()
    _READER_BASE_test.make_block0_dc(make_block0_dc_payload, make_block0_dc_payload_size)

'''test_make_root_entry'''

def test_make_root_entry():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_root_entry_pub_key = mock.MagicMock()
    _READER_BASE_test.make_root_entry(make_root_entry_pub_key)

'''test_make_root_entry_dc'''

def test_make_root_entry_dc():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_root_entry_dc_pub_key = mock.MagicMock()
    _READER_BASE_test.make_root_entry_dc(make_root_entry_dc_pub_key)

'''test_make_csk_entry'''

def test_make_csk_entry():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_csk_entry_root_key = mock.MagicMock()
    make_csk_entry_CSK_pub_key = mock.MagicMock()
    _READER_BASE_test.make_csk_entry(make_csk_entry_root_key, make_csk_entry_CSK_pub_key)

'''test_make_csk_entry_dc'''

def test_make_csk_entry_dc():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_csk_entry_dc_root_key = mock.MagicMock()
    make_csk_entry_dc_CSK_pub_key = mock.MagicMock()
    _READER_BASE_test.make_csk_entry_dc(make_csk_entry_dc_root_key, make_csk_entry_dc_CSK_pub_key)

'''test_make_block0_entry'''

def test_make_block0_entry():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_block0_entry_block0 = mock.MagicMock()
    make_block0_entry_CSK_key = mock.MagicMock()
    _READER_BASE_test.make_block0_entry(make_block0_entry_block0, make_block0_entry_CSK_key)

'''test_make_block0_entry_dc'''

def test_make_block0_entry_dc():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_block0_entry_dc_block0 = mock.MagicMock()
    make_block0_entry_dc_CSK_key = mock.MagicMock()
    _READER_BASE_test.make_block0_entry_dc(make_block0_entry_dc_block0, make_block0_entry_dc_CSK_key)

'''test_make_block1'''

def test_make_block1():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_block1_root_entry = mock.MagicMock()
    make_block1_block0_entry = mock.MagicMock()
    make_block1_CSK = mock.MagicMock()
    _READER_BASE_test.make_block1(make_block1_root_entry, make_block1_block0_entry, make_block1_CSK)

'''test_make_block1_dc'''

def test_make_block1_dc():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    make_block1_dc_block0 = mock.MagicMock()
    make_block1_dc_root_entry = mock.MagicMock()
    make_block1_dc_block0_entry = mock.MagicMock()
    make_block1_dc_CSK = mock.MagicMock()
    _READER_BASE_test.make_block1_dc(make_block1_dc_block0, make_block1_dc_root_entry, make_block1_dc_block0_entry, make_block1_dc_CSK)

'''test_msb_calculate_crc32'''

def test_msb_calculate_crc32():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    _READER_BASE_test = _READER_BASE(init_args, init_hsm_manager, init_config)
    msb_calculate_crc32_datas = mock.MagicMock()
    msb_calculate_crc32_init = mock.MagicMock()
    msb_calculate_crc32_CRC_POLYNOMIAL = mock.MagicMock()
    msb_calculate_crc32_reverse = mock.MagicMock()
    msb_calculate_crc32_xor = mock.MagicMock()
    _READER_BASE_test.msb_calculate_crc32(msb_calculate_crc32_datas, msb_calculate_crc32_init, msb_calculate_crc32_CRC_POLYNOMIAL, msb_calculate_crc32_reverse, msb_calculate_crc32_xor)

'''test_run'''

def test_run():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    RHP_reader_test = RHP_reader(init_args, init_hsm_manager, init_config)
    RHP_reader_test.run()

'''test_is_Rush_BMC'''

def test_is_Rush_BMC():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    UPDATE_reader_test = UPDATE_reader(init_args, init_hsm_manager, init_config)
    is_Rush_BMC_payload = mock.MagicMock()
    is_Rush_BMC_offset = mock.MagicMock()
    UPDATE_reader_test.is_Rush_BMC(is_Rush_BMC_payload, is_Rush_BMC_offset)

'''test_is_JSON'''

def test_is_JSON():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    UPDATE_reader_test = UPDATE_reader(init_args, init_hsm_manager, init_config)
    is_JSON_contents = mock.MagicMock()
    UPDATE_reader_test.is_JSON(is_JSON_contents)

'''test_skip_JSON'''

def test_skip_JSON():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    UPDATE_reader_test = UPDATE_reader(init_args, init_hsm_manager, init_config)
    skip_JSON_contents = mock.MagicMock()
    UPDATE_reader_test.skip_JSON(skip_JSON_contents)

'''test_already_signed'''

def test_already_signed():
    init_args = mock.MagicMock()
    init_hsm_manager = mock.MagicMock()
    init_config = mock.MagicMock()
    UPDATE_reader_test = UPDATE_reader(init_args, init_hsm_manager, init_config)
    already_signed_contents = mock.MagicMock()
    already_signed_offset = mock.MagicMock()
    UPDATE_reader_test.already_signed(already_signed_contents, already_signed_offset)

