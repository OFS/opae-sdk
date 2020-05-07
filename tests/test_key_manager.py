import json
import os
import tempfile
import unittest
from unittest import mock
from pacsign.hsm_managers.openssl.key_manager import (HSM_MANAGER,
                                                      _KEY,
                                                      _PRIVATE_KEY,
                                                      _PUBLIC_KEY)

class test_openssl_keymanager(unittest.TestCase):
    def setUp(self):
        cfg = {'cryptoki_version': [2, 40],
               'library_version': [2,5],
               'platform-name': "DCP"
               }
        self._dummy_files = []
        self._dummy_files.append(tempfile.NamedTemporaryFile(mode='w',
                                                             delete=False))
        json.dump(cfg, self._dummy_files[-1])
        self._dummy_files[-1].close()

        cfg['key_path'] = '/tmp/key'

        self._dummy_files.append(tempfile.NamedTemporaryFile(mode='w',
                                                             delete=False))
        json.dump(cfg, self._dummy_files[-1])
        self._dummy_files[-1].close()

    def tearDown(self):
        for f in self._dummy_files:
            os.unlink(f.name)

    '''test_clean'''

    def test_clean(self):
        init_file = mock.MagicMock()
        init_openssl = mock.MagicMock()
        init_key_info = mock.MagicMock()
        _KEY_test = _KEY(init_file, init_openssl, init_key_info)
        _KEY_test.clean()

    '''test_get_public_key'''

    def test_get_public_key(self):
        with self.assertRaises(AssertionError) as err:
            HSM_MANAGER_test = HSM_MANAGER(self._dummy_files[0].name)
            self.assertIn("key_path", err.msg)

        with self.assertRaises(AssertionError):
            HSM_MANAGER_test = HSM_MANAGER(self._dummy_files[1].name)

        get_public_key_public_pem = "test_key.pem"
        HSM_MANAGER_test.get_public_key(get_public_key_public_pem)

'''test_sign'''

def test_sign():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _PRIVATE_KEY_test = _PRIVATE_KEY(init_file, init_openssl, init_key_info)
    sign_sha = mock.MagicMock()
    sign_data = mock.MagicMock()
    sign_fixed_RS_size = mock.MagicMock()
    _PRIVATE_KEY_test.sign(sign_sha, sign_data, sign_fixed_RS_size)

'''test_get_private_key'''

def test_get_private_key():
    init_cfg_file = mock.MagicMock()
    HSM_MANAGER_test = HSM_MANAGER(init_cfg_file)
    get_private_key_private_pem = mock.MagicMock()
    HSM_MANAGER_test.get_private_key(get_private_key_private_pem)

'''test_retrive_key_info'''

def test_retrive_key_info():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _KEY_test = _KEY(init_file, init_openssl, init_key_info)
    _KEY_test.retrive_key_info()

'''test_check_pem_file'''

def test_check_pem_file():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _KEY_test = _KEY(init_file, init_openssl, init_key_info)
    _KEY_test.check_pem_file()

'''test_get_char_point_from_bignum'''

def test_get_char_point_from_bignum():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _KEY_test = _KEY(init_file, init_openssl, init_key_info)
    get_char_point_from_bignum_bignum = mock.MagicMock()
    get_char_point_from_bignum_size = mock.MagicMock()
    _KEY_test.get_char_point_from_bignum(get_char_point_from_bignum_bignum, get_char_point_from_bignum_size)

'''test_match_xy'''

def test_match_xy():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _KEY_test = _KEY(init_file, init_openssl, init_key_info)
    match_xy_public_key = mock.MagicMock()
    _KEY_test.match_xy(match_xy_public_key)

'''test_verify_signature'''

def test_verify_signature():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _KEY_test = _KEY(init_file, init_openssl, init_key_info)
    verify_signature_sha = mock.MagicMock()
    verify_signature_rs = mock.MagicMock()
    _KEY_test.verify_signature(verify_signature_sha, verify_signature_rs)

'''test_get_content_type'''

def test_get_content_type():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _KEY_test = _KEY(init_file, init_openssl, init_key_info)
    _KEY_test.get_content_type()

'''test_get_X_Y'''

def test_get_X_Y():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _PUBLIC_KEY_test = _PUBLIC_KEY(init_file, init_openssl, init_key_info)
    _PUBLIC_KEY_test.get_X_Y()

'''test_get_permission'''

def test_get_permission():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _PUBLIC_KEY_test = _PUBLIC_KEY(init_file, init_openssl, init_key_info)
    _PUBLIC_KEY_test.get_permission()

'''test_get_ID'''

def test_get_ID():
    init_file = mock.MagicMock()
    init_openssl = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _PUBLIC_KEY_test = _PUBLIC_KEY(init_file, init_openssl, init_key_info)
    _PUBLIC_KEY_test.get_ID()

