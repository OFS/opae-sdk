import array
import json
import os
import tempfile
import unittest
import mocksign

from unittest import mock
from pacsign.reader import (_READER_BASE,
                            CANCEL_reader,
                            UPDATE_reader,
                            RHP_reader)

from pacsign import database, hsm_managers, common_util


class test_READER_BASE_SR(unittest.TestCase):
    def setUp(self):
        self._dummy_key_file = tempfile.NamedTemporaryFile(mode='w')
        self._key_path = os.path.dirname(self._dummy_key_file.name)
        self._dummy_cfg = {'key_path': self._key_path,
                           'curve': '',
                           'keys': [{'label': 'root_key',
                                     'permissions': '0xFFFFFFFF',
                                     'is_root': True,
                                     'type': 'SR',
                                     'priv_key': self._dummy_key_file.name,
                                     'pub_key': self._dummy_key_file.name},
                                    {'label': 'csk_key',
                                     'permissions': '0x0',
                                     'is_root': False,
                                     'type': 'SR',
                                     'csk_id': 0,
                                     'priv_key': self._dummy_key_file.name,
                                     'pub_key': self._dummy_key_file.name}]}
        self._dummy_cfg_file = tempfile.NamedTemporaryFile(mode='w')
        json.dump(self._dummy_cfg, self._dummy_cfg_file)
        self._dummy_cfg_file.seek(0)
        self._args = mock.MagicMock()
        self._args.cert_type = 'RK_384'
        self._args.main_command = 'SR'
        self._args.root_key = 'root_key'
        self._args.code_signing_key = 'csk_key'
        self._hsm_manager = hsm_managers.openssl
        self._config = self._dummy_cfg_file.name
        self._bitstream = mocksign.bitstream.create(database.CONTENT_SR)
        self._dummy_key_file.write(self._bitstream.sk.to_pem().decode())
        self._dummy_key_file.seek(0)
        point = self._bitstream.pk.point
        self._pkdata = point.x().to_bytes(64, 'big')
        self._pkdata += point.y().to_bytes(64, 'big')

    '''test_finalize'''

    def test_finalize(self):
        _READER_BASE_test = _READER_BASE(self._args,
                                         self._hsm_manager,
                                         self._config)
        bs = mocksign.bitstream.create(database.CONTENT_SR)
        block0 = common_util.BYTE_ARRAY()
        block0.data = array.array('B', bs.block0)
        block1 = common_util.BYTE_ARRAY()
        block1.data = array.array('B', bs.block0)
        payload = common_util.BYTE_ARRAY()
        payload.data = array.array('B', bs.payload)

        with tempfile.NamedTemporaryFile(mode='w+b') as tmp:
            _READER_BASE_test.finalize(tmp, block0, block1, payload)

    '''test_make_block0'''

    def test_make_block0(self):
        bs = self._bitstream
        _READER_BASE_test = _READER_BASE(self._args,
                                         self._hsm_manager,
                                         self._config)
        payload = common_util.BYTE_ARRAY()
        payload.data = array.array('B', self._bitstream.payload)
        b0 = _READER_BASE_test.make_block0(payload,
                                           len(self._bitstream.payload))
        assert bytearray(b0.data[0:4]) == bs.block0[0:4]

    '''test_make_root_entry'''

    def test_make_root_entry(self):
        rhp = RHP_reader(self._args, self._hsm_manager, self._config)
        ba = common_util.BYTE_ARRAY()
        ba.data = array.array('B', self._pkdata)
        rhp.make_root_entry(ba)

    '''test_make_csk_entry'''

    def test_make_csk_entry(self):
        up = UPDATE_reader(self._args, self._hsm_manager, self._config)
        make_csk_entry_root_key = 'root_key'
        make_csk_entry_CSK_pub_key = common_util.BYTE_ARRAY()

        up.make_csk_entry(
            make_csk_entry_root_key,
            make_csk_entry_CSK_pub_key)

        self._args.csk_id = 0
        can = CANCEL_reader(self._args, self._hsm_manager, self._config)
        can.run()

    '''test_make_block0_entry'''

    def test_make_block0_entry(self):
        bs = self._bitstream
        rhp = RHP_reader(self._args, self._hsm_manager, self._config)
        b0 = mock.MagicMock()
        b0.data = bs.block0
        rhp.make_block0_entry(b0, 'csk_key')

    '''test_make_block1'''

    def test_make_block1(self):
        rhp = RHP_reader(self._args, self._hsm_manager, self._config)
        make_block1_root_entry = mock.MagicMock()
        make_block1_block0_entry = mock.MagicMock()
        make_block1_CSK = mock.MagicMock()
        rhp.make_block1(
            make_block1_root_entry,
            make_block1_block0_entry,
            make_block1_CSK)


class test_READER_BASE_dc(unittest.TestCase):
    def setUp(self):
        self._dummy_key_file = tempfile.NamedTemporaryFile(mode='w')
        self._key_path = os.path.dirname(self._dummy_key_file.name)
        self._dummy_cfg = {'key_path': self._key_path,
                           'curve': '',
                           'keys': [{'label': 'root_key',
                                     'permissions': '0xFFFFFFFF',
                                     'is_root': True,
                                     'type': 'PR',
                                     'priv_key': self._dummy_key_file.name,
                                     'pub_key': self._dummy_key_file.name},
                                    {'label': 'csk_key',
                                     'permissions': '0x0',
                                     'is_root': False,
                                     'type': 'PR',
                                     'csk_id': 0,
                                     'priv_key': self._dummy_key_file.name,
                                     'pub_key': self._dummy_key_file.name}]}
        self._dummy_cfg_file = tempfile.NamedTemporaryFile(mode='w')
        json.dump(self._dummy_cfg, self._dummy_cfg_file)
        self._dummy_cfg_file.seek(0)
        self._args = mock.MagicMock()
        self._args.cert_type = 'RK_384'
        self._args.main_command = 'PR'
        self._args.root_key = 'root_key'
        self._args.code_signing_key = 'csk_key'
        self._hsm_manager = hsm_managers.openssl
        self._config = self._dummy_cfg_file.name
        self._bitstream = mocksign.d5005_pr.create()
        self._dummy_key_file.write(self._bitstream.sk.to_pem().decode())
        self._dummy_key_file.seek(0)
        point = self._bitstream.pk.point
        self._pkdata = point.x().to_bytes(64, 'big')
        self._pkdata += point.y().to_bytes(64, 'big')

    '''test_is_Darby_PR'''

    def test_is_Darby_PR(self):
        _READER_BASE_test = _READER_BASE(self._args,
                                         self._hsm_manager,
                                         self._config)
        with tempfile.NamedTemporaryFile(mode='w+b') as fp:
            fp.write(self._bitstream.payload)
            fp.seek(0)
            ba = common_util.BYTE_ARRAY("FILE", fp.name)
            assert _READER_BASE_test.is_Darby_PR(ba, 0)

    '''test_make_block0_dc'''

    def test_make_block0_dc(self):
        _READER_BASE_test = _READER_BASE(self._args,
                                         self._hsm_manager,
                                         self._config)
        payload = common_util.BYTE_ARRAY()
        bs = self._bitstream
        payload.data = array.array('B', bs.payload)
        b0 = _READER_BASE_test.make_block0_dc(payload, len(bs.payload))
        assert bytearray(b0.data[0:4]) == bs.block0[0:4]

    '''test_make_root_entry_dc'''

    def test_make_root_entry_dc(self):
        rhp = RHP_reader(self._args, self._hsm_manager, self._config)
        make_root_entry_dc_pub_key = common_util.BYTE_ARRAY()
        make_root_entry_dc_pub_key.data = self._pkdata
        rhp.make_root_entry_dc(make_root_entry_dc_pub_key)

    '''test_make_csk_entry_dc'''

    def test_make_csk_entry_dc(self):
        self._args.input_file = self._dummy_key_file.name
        up = UPDATE_reader(self._args, self._hsm_manager, self._config)
        root_key = 'root_key'
        csk_pub_key = common_util.BYTE_ARRAY()
        up.make_csk_entry_dc(root_key, csk_pub_key)
        up.run()

    '''test_make_block0_entry_dc'''

    def test_make_block0_entry_dc(self):
        _READER_BASE_test = _READER_BASE(self._args,
                                         self._hsm_manager,
                                         self._config)
        b0 = mock.MagicMock()
        b0.data = self._bitstream.block0
        _READER_BASE_test.make_block0_entry_dc(b0, 'csk_key')

    '''test_make_block1_dc'''

    def test_make_block1_dc(self):
        rhp = RHP_reader(self._args, self._hsm_manager, self._config)
        b0 = mock.MagicMock()
        b0.data = self._bitstream.block0
        make_block1_dc_root_entry = mock.MagicMock()
        make_block1_dc_block0_entry = mock.MagicMock()
        make_block1_dc_CSK = mock.MagicMock()
        rhp.make_block1_dc(
            b0,
            make_block1_dc_root_entry,
            make_block1_dc_block0_entry,
            make_block1_dc_CSK)

    '''test_msb_calculate_crc32'''

    def test_msb_calculate_crc32(self):
        _READER_BASE_test = _READER_BASE(self._args,
                                         self._hsm_manager,
                                         self._config)
        msb_calculate_crc32_datas = mock.MagicMock()
        msb_calculate_crc32_init = mock.MagicMock()
        msb_calculate_crc32_CRC_POLYNOMIAL = mock.MagicMock()
        msb_calculate_crc32_reverse = mock.MagicMock()
        msb_calculate_crc32_xor = mock.MagicMock()
        _READER_BASE_test.msb_calculate_crc32(
            msb_calculate_crc32_datas,
            msb_calculate_crc32_init,
            msb_calculate_crc32_CRC_POLYNOMIAL,
            msb_calculate_crc32_reverse,
            msb_calculate_crc32_xor)

    '''test_run'''

    def test_run(self):
        rhp = RHP_reader(self._args,
                         self._hsm_manager,
                         self._config)
        rhp.run()

    '''test_is_Rush_BMC'''

    def test_is_Rush_BMC(self):
        UPDATE_reader_test = UPDATE_reader(self._args,
                                           self._hsm_manager,
                                           self._config)

        payload = common_util.BYTE_ARRAY()
        payload.data = self._bitstream.payload

        assert not UPDATE_reader_test.is_Rush_BMC(payload, 0)

    '''test_is_JSON'''

    def test_is_JSON(self):
        UPDATE_reader_test = UPDATE_reader(self._args,
                                           self._hsm_manager,
                                           self._config)
        contents = common_util.BYTE_ARRAY()
        contents.data = self._bitstream.payload
        UPDATE_reader_test.is_JSON(contents)

    '''test_skip_JSON'''

    def test_skip_JSON(self):
        UPDATE_reader_test = UPDATE_reader(
            self._args, self._hsm_manager, self._config)
        skip_JSON_contents = mock.MagicMock()
        UPDATE_reader_test.skip_JSON(skip_JSON_contents)

    '''test_already_signed'''

    def test_already_signed(self):
        UPDATE_reader_test = UPDATE_reader(
            self._args, self._hsm_manager, self._config)
        already_signed_contents = common_util.BYTE_ARRAY()
        already_signed_contents.data = self._bitstream.payload
        already_signed_offset = 0
        UPDATE_reader_test.already_signed(
            already_signed_contents,
            already_signed_offset)
