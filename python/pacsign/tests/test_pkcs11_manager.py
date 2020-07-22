import unittest
from unittest import mock
from pacsign.hsm_managers.pkcs11.pkcs11_manager import (HSM_MANAGER,
                                                        _PUBLIC_KEY)
'''test_clean'''

def test_clean():
    init_cfg_file = mock.MagicMock()
    HSM_MANAGER_test = HSM_MANAGER(init_cfg_file)
    HSM_MANAGER_test.clean()

'''test_get_key'''

def test_get_key():
    init_cfg_file = mock.MagicMock()
    HSM_MANAGER_test = HSM_MANAGER(init_cfg_file)
    get_key_key = mock.MagicMock()
    get_key_attrib = mock.MagicMock()
    HSM_MANAGER_test.get_key(get_key_key, get_key_attrib)

'''test_get_public_key'''

def test_get_public_key():
    init_cfg_file = mock.MagicMock()
    HSM_MANAGER_test = HSM_MANAGER(init_cfg_file)
    get_public_key_public_key = mock.MagicMock()
    HSM_MANAGER_test.get_public_key(get_public_key_public_key)

'''test_sign'''

def test_sign():
    init_cfg_file = mock.MagicMock()
    HSM_MANAGER_test = HSM_MANAGER(init_cfg_file)
    sign_sha = mock.MagicMock()
    sign_key = mock.MagicMock()
    HSM_MANAGER_test.sign(sign_sha, sign_key)

'''test_get_X_Y'''

def test_get_X_Y():
    init_xy = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _PUBLIC_KEY_test = _PUBLIC_KEY(init_xy, init_key_info)
    _PUBLIC_KEY_test.get_X_Y()

'''test_get_permission'''

def test_get_permission():
    init_xy = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _PUBLIC_KEY_test = _PUBLIC_KEY(init_xy, init_key_info)
    _PUBLIC_KEY_test.get_permission()

'''test_get_ID'''

def test_get_ID():
    init_xy = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _PUBLIC_KEY_test = _PUBLIC_KEY(init_xy, init_key_info)
    _PUBLIC_KEY_test.get_ID()

'''test_get_content_type'''

def test_get_content_type():
    init_xy = mock.MagicMock()
    init_key_info = mock.MagicMock()
    _PUBLIC_KEY_test = _PUBLIC_KEY(init_xy, init_key_info)
    _PUBLIC_KEY_test.get_content_type()

