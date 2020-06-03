import io
import json
import os
import pytest
import unittest
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
    def setUp(self):
        self._dummy_bitstream = mocksign.bitstream.create(database.CONTENT_BMC)
        self._dummy_file = tempfile.NamedTemporaryFile(mode='w+b')
        self._dummy_file.write(self._dummy_bitstream.getbuffer())
        self._dummy_file.seek(0)

    def tearDown(self):
        pass
        # self._dummy_file.close()

    def test_verifier_base(self):
        args = mocksign.args(main_command = 'BMC',
                             root_bitstream = self._dummy_file.name)
        _VERIFIER_BASE_test = _VERIFIER_BASE(args)


    def test_verifier_base_neg(self):
        args = mocksign.args(main_command = 'XYZ',
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
        bio.write(database.DC_PLATFORM_NUM.to_bytes(4, 'little'))
        bio.seek(offset + 12)
        bio.write(database.PR_IDENTIFIER.to_bytes(4, 'little'))

        with tempfile.NamedTemporaryFile(mode='w+b') as bsfile:
            args = mocksign.args(main_command = 'PR',
                                 root_bitstream = bsfile.name)
            bsfile.write(bio.getbuffer())
            bsfile.seek(0)
            base = _VERIFIER_BASE(args)
            self.assertEqual(base.is_Darby_PR(base.reh, offset), True)


'''test_run'''
@mock.patch('pacsign.ecdsa.is_on_curve', return_value=True)
@mock.patch('pacsign.ecdsa.inverse_mod', return_value=1)
@pytest.mark.parametrize('cfg',
                         [('BMC', database.CONTENT_BMC),
                          ('PR', database.CONTENT_PR)])
def test_run(is_on_curve_patch, inverse_mod_patch, cfg):
    command, content_type = cfg[0], cfg[1]
    bs = mocksign.bitstream.create(content_type)
    with tempfile.NamedTemporaryFile(mode='w+b') as tmp:
        tmp.write(bs.getbuffer())
        tmp.seek(0)
        args = mocksign.args(main_command = command,
                             root_bitstream = tmp.name,
                             cert_type = 'RK_256')

        verify_reader_test = verify_reader(args)
        run_fname = tmp.name
        run_file_offset = 0
        run_block0 = mock.MagicMock()
        run_block0.data = bs.block0
        run_block0.size.return_value = len(bs.block0)
        run_block1 = mock.MagicMock()
        run_block1.data = bs.block1
        run_block1.size.return_value = len(bs.block1)
        run_payload = mock.MagicMock()
        run_payload.data = bs.payload
        run_payload.size.return_value = len(bs.payload)
        verify_reader_test.run(run_fname,
                               run_file_offset,
                               run_block0,
                               run_block1,
                               run_payload)

'''test_is_JSON'''
def test_is_JSON():
    bs = mocksign.bitstream.create(database.CONTENT_PR)
    with tempfile.NamedTemporaryFile('w+b') as tmp:
        tmp.write(bs.getbuffer())
        tmp.seek(0)

        args = mocksign.args(main_command = 'PR',
                             root_bitstream = tmp.name,
                             cert_type = 'RK_256')
        verify_reader_test = verify_reader(args)
        metadata_guid = mock.MagicMock()
        metadata_guid.data = ("XeonFPGA" + chr(0xB7) + "GBSv001").encode()
        verify_reader_test.is_JSON(metadata_guid)

'''test_skip_JSON'''
def test_skip_JSON():
    bs = mocksign.bitstream.create(database.CONTENT_PR)
    with tempfile.NamedTemporaryFile('w+b') as tmp:
        tmp.write(bs.getbuffer())
        tmp.seek(0)

        args = mocksign.args(main_command = 'PR',
                             root_bitstream = tmp.name,
                             cert_type = 'RK_256')
        verify_reader_test = verify_reader(args)
        verify_reader_test = verify_reader(args)
        skip_JSON_contents = mock.MagicMock()
        skip_JSON_contents.get_dword.return_value = 16
        assert 36 == verify_reader_test.skip_JSON(skip_JSON_contents)


class test_print_bitstream(test_verifier_base):
    def setUp(self):
        self._dummy_bitstream = mocksign.bitstream.create(database.CONTENT_PR)
        self._dummy_file = tempfile.NamedTemporaryFile(mode='w+b')
        self._dummy_file.write(self._dummy_bitstream.getbuffer())
        self._dummy_file.seek(0)
        self.b0 = mocksign.byte_array(self._dummy_bitstream.block0)
        self.b1 = mocksign.byte_array(self._dummy_bitstream.block1)
        self.payload = mocksign.byte_array(self._dummy_bitstream.payload)


    '''test_print_json'''
    def test_print_json(self):
        args = mocksign.args(main_command = 'PR',
                             root_bitstream = self._dummy_file.name,
                             cert_type = 'RK_256')

        print_bitstream_test = print_bitstream(args,
                                               self.b0, self.b1, self.payload)
        print_bitstream_test.print_json(json.dumps({'test': 0}))

    '''test_print_payload'''

    def test_print_payload(self):
        args = mocksign.args(main_command = 'PR',
                             root_bitstream = self._dummy_file.name,
                             cert_type = 'RK_256')
        print_bitstream_test = print_bitstream(args,
                                               self.b0, self.b1, self.payload)
        print_payload_b0 = mock.MagicMock()
        print_payload_bits = mock.MagicMock()
        print_bitstream_test.print_payload(print_payload_b0, print_payload_bits)



def test_Block_0_print_block():
    bs = mocksign.bitstream.create(database.CONTENT_BMC)
    b0 = Block_0(bs.getbuffer(), bs.payload)
    b0.print_block()

def test_Block_0_dc_print_block():
    bs = mocksign.bitstream.create(database.CONTENT_BMC)
    b0 = Block_0_dc(bs.getbuffer(), bs.payload)
    b0.print_block()

def test_Block_1_print_block():
    bs = mocksign.bitstream.create(database.CONTENT_BMC)
    b1 = Block_1(bs.getbuffer(), bs.block0)
    b1.print_block()

def test_RootEntry_print_block():
    bs = mocksign.bitstream.create(database.CONTENT_BMC)
    re = Root_Entry(bs.block1[16:])

    with mock.patch('sys.stdout', io.StringIO()) as stdout:
        re.print_block()
        assert stdout.getvalue().strip() != 'No root entry'

def test_Block_0_Entry_print_block():
    bs = mocksign.bitstream.create(database.CONTENT_BMC)
    b0entry = Block_0_Entry(bs.b0entry)
    b0entry.print_block()

def test_CSK_print_block():
    bs = mocksign.bitstream.create(database.CONTENT_BMC)
    root = mock.MagicMock()
    root.curve_magic = 0xC7B88C74
    csk = CSK(bs.csk, root)
    csk.print_block()

def test_Block_1_dc_print_block():
    bs = mocksign.bitstream.create(database.CONTENT_PR)
    data = bs.getbuffer()
    b0 = mock.MagicMock()
    b0.sha256 = int.from_bytes(bs.m256be, 'big')
    b0.sha384 = int.from_bytes(bs.m384be, 'big')
    with mock.patch('sys.stdout', new=io.StringIO()) as stdout:
        b1printer = Block_1_dc(data[48:], b0)
        b1printer.print_block()
        lines = stdout.getvalue().split('\n')
        assert lines[0] not in ['SHA-384 mismatch',
                                "Can't find root entry"]
        assert lines[0] == 'Block 1:'
