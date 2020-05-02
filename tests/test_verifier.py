import unittest
from unittest import mock
from pacsign.verifier import (_VERIFIER_BASE,
                              verify_reader,
                              print_bitstream,
                              Block_0,
                              Block_0_dc,
                              Block_1,
                              Root_Entry,
                              CSK,
                              Block_0_Entry,
                              Block_1_dc,
                              DC_Root_Entry,
                              DC_CSK_Entry,
                              DC_B0_Entry)
'''test_is_Darby_PR'''

def test_is_Darby_PR():
    init_args = mock.MagicMock()
    _VERIFIER_BASE_test = _VERIFIER_BASE(init_args)
    is_Darby_PR_contents = mock.MagicMock()
    is_Darby_PR_offset = mock.MagicMock()
    _VERIFIER_BASE_test.is_Darby_PR(is_Darby_PR_contents, is_Darby_PR_offset)

'''test_run'''

def test_run():
    init_args = mock.MagicMock()
    verify_reader_test = verify_reader(init_args)
    run_fname = mock.MagicMock()
    run_file_offset = mock.MagicMock()
    run_block0 = mock.MagicMock()
    run_block1 = mock.MagicMock()
    run_payload = mock.MagicMock()
    verify_reader_test.run(run_fname, run_file_offset, run_block0, run_block1, run_payload)

'''test_is_JSON'''

def test_is_JSON():
    init_args = mock.MagicMock()
    verify_reader_test = verify_reader(init_args)
    is_JSON_contents = mock.MagicMock()
    verify_reader_test.is_JSON(is_JSON_contents)

'''test_skip_JSON'''

def test_skip_JSON():
    init_args = mock.MagicMock()
    verify_reader_test = verify_reader(init_args)
    skip_JSON_contents = mock.MagicMock()
    verify_reader_test.skip_JSON(skip_JSON_contents)

'''test_print_json'''

def test_print_json():
    init_args = mock.MagicMock()
    init_b0 = mock.MagicMock()
    init_b1 = mock.MagicMock()
    init_payload = mock.MagicMock()
    init_json_str = mock.MagicMock()
    print_bitstream_test = print_bitstream(init_args, init_b0, init_b1, init_payload, init_json_str)
    print_json_json_str = mock.MagicMock()
    print_bitstream_test.print_json(print_json_json_str)

'''test_print_payload'''

def test_print_payload():
    init_args = mock.MagicMock()
    init_b0 = mock.MagicMock()
    init_b1 = mock.MagicMock()
    init_payload = mock.MagicMock()
    init_json_str = mock.MagicMock()
    print_bitstream_test = print_bitstream(init_args, init_b0, init_b1, init_payload, init_json_str)
    print_payload_b0 = mock.MagicMock()
    print_payload_bits = mock.MagicMock()
    print_bitstream_test.print_payload(print_payload_b0, print_payload_bits)

'''test_print_block'''

def test_print_block():
    init_bits = mock.MagicMock()
    DC_B0_Entry_test = DC_B0_Entry(init_bits)
    DC_B0_Entry_test.print_block()

