import os
import unittest
import struct
import tempfile
from unittest import mock

import mocksign
from pacsign import database
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


class test_verifier_base(unittest.TestCase):
    def mock_args(self, **kwargs):
        m = mock.MagicMock()
        for k, v in kwargs.items():
            setattr(m, k, v)
        return m

    def setUp(self):
        self._dummy_bitstream = mocksign.bitstream.create(database.CONTENT_BMC)
        self._dummy_file = tempfile.NamedTemporaryFile(mode='w+b')
        self._dummy_file.write(self._dummy_bitstream.getbuffer())
        self._dummy_file.seek(0)

    def tearDown(self):
        pass
        # self._dummy_file.close()

    def test_verifier_base(self):
        args = self.mock_args(main_command = 'BMC',
                              root_bitstream = self._dummy_file.name)
        _VERIFIER_BASE_test = _VERIFIER_BASE(args)


    def test_verifier_base_neg(self):
        args = self.mock_args(main_command = 'XYZ',
                              root_bitstream = self._dummy_file.name)
        with self.assertRaises(KeyError):
            _VERIFIER_BASE_test = _VERIFIER_BASE(args)

        args.main_command = 'BMC'
        self._dummy_file.seek(0)
        self._dummy_file.write(bytes(0xffff))
        self._dummy_file.seek(0)
        with self.assertRaises(AssertionError) as err:
            _VERIFIER_BASE_test = _VERIFIER_BASE(args)
            self.assertIn(err.msg, 'is not a root entry hash')



    '''test_is_Darby_PR'''

    def test_is_Darby_PR(self):
        bs = mocksign.bitstream.create(database.CONTENT_PR)
        bio = bs.bytes_io
        offset = bio.tell()
        bio.write(struct.pack('<I', database.DC_PLATFORM_NUM))
        bio.seek(offset + 12)
        bio.write(struct.pack('<I', database.PR_IDENTIFIER))

        with tempfile.NamedTemporaryFile(mode='w+b') as bsfile:
            args = self.mock_args(main_command = 'PR',
                                  root_bitstream = bsfile.name)
            bsfile.write(bio.getbuffer())
            bsfile.seek(0)
            base = _VERIFIER_BASE(args)
            self.assertEqual(base.is_Darby_PR(base.reh, offset), True)

class test_verifier_reader(test_verifier_base):

    '''test_run'''

    def test_run(self):
        args = self.mock_args(main_command = 'BMC',
                              root_bitstream = self._dummy_file.name,
                              cert_type = 'RK_256')

        verify_reader_test = verify_reader(args)
        run_fname = self._dummy_file.name
        run_file_offset = 0
        run_block0 = mock.MagicMock()
        run_block0.data = self._dummy_bitstream.block0
        run_block0.size.return_value = len(self._dummy_bitstream.block0)
        run_block1 = mock.MagicMock()
        run_block1.data = self._dummy_bitstream.block1
        run_block1.size.return_value = len(self._dummy_bitstream.block1)
        run_payload = mock.MagicMock()
        run_payload.data = self._dummy_bitstream.payload
        run_payload.size.return_value = len(self._dummy_bitstream.payload)
        verify_reader_test.run(run_fname,
                               run_file_offset,
                               run_block0,
                               run_block1,
                               run_payload)

# '''test_is_JSON'''
#
# def test_is_JSON():
#     init_args = mock.MagicMock()
#     verify_reader_test = verify_reader(init_args)
#     is_JSON_contents = mock.MagicMock()
#     verify_reader_test.is_JSON(is_JSON_contents)
#
# '''test_skip_JSON'''
#
# def test_skip_JSON():
#     init_args = mock.MagicMock()
#     verify_reader_test = verify_reader(init_args)
#     skip_JSON_contents = mock.MagicMock()
#     verify_reader_test.skip_JSON(skip_JSON_contents)
#
# '''test_print_json'''
#
# def test_print_json():
#     init_args = mock.MagicMock()
#     init_b0 = mock.MagicMock()
#     init_b1 = mock.MagicMock()
#     init_payload = mock.MagicMock()
#     init_json_str = mock.MagicMock()
#     print_bitstream_test = print_bitstream(init_args, init_b0, init_b1, init_payload, init_json_str)
#     print_json_json_str = mock.MagicMock()
#     print_bitstream_test.print_json(print_json_json_str)
#
# '''test_print_payload'''
#
# def test_print_payload():
#     init_args = mock.MagicMock()
#     init_b0 = mock.MagicMock()
#     init_b1 = mock.MagicMock()
#     init_payload = mock.MagicMock()
#     init_json_str = mock.MagicMock()
#     print_bitstream_test = print_bitstream(init_args, init_b0, init_b1, init_payload, init_json_str)
#     print_payload_b0 = mock.MagicMock()
#     print_payload_bits = mock.MagicMock()
#     print_bitstream_test.print_payload(print_payload_b0, print_payload_bits)
#
# '''test_print_block'''
#
# def test_print_block():
#     init_bits = mock.MagicMock()
#     DC_B0_Entry_test = DC_B0_Entry(init_bits)
#     DC_B0_Entry_test.print_block()

