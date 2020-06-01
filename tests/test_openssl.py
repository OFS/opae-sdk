import unittest

import secrets
from unittest import mock
from pacsign.hsm_managers.openssl.openssl import (
    CURVE_INFO, get_curve_info_from_name, get_curve_info_from_enum,
    get_curve_info_from_curve_magic_num, get_supported_curve_info_names,
    get_supported_curve_info_curve_magic_nums,
    get_supported_curve_info_sha_magic_nums, get_sha_magic_num_size,
    password_callback, OPENSSL_SIGNATURE, OPENSSL_SHA256, OPENSSL_SHA512_DATA,
    OPENSSL_SHA512, OPENSSL_AES_KEY, openssl)

TEST_DATA= secrets.token_bytes(64)

'''test_get_curve_info_from_name'''


def test_get_curve_info_from_name():
    get_curve_info_from_name_name = mock.MagicMock()
    get_curve_info_from_name(get_curve_info_from_name_name)


'''test_get_curve_info_from_enum'''


def test_get_curve_info_from_enum():
    get_curve_info_from_enum_enum = mock.MagicMock()
    get_curve_info_from_enum(get_curve_info_from_enum_enum)


'''test_get_curve_info_from_curve_magic_num'''


def test_get_curve_info_from_curve_magic_num():
    get_curve_info_from_curve_magic_num_curve_magic_num = mock.MagicMock()
    get_curve_info_from_curve_magic_num(
        get_curve_info_from_curve_magic_num_curve_magic_num)


'''test_get_supported_curve_info_names'''


def test_get_supported_curve_info_names():
    get_supported_curve_info_names()


'''test_get_supported_curve_info_curve_magic_nums'''


def test_get_supported_curve_info_curve_magic_nums():
    get_supported_curve_info_curve_magic_nums()


'''test_get_supported_curve_info_sha_magic_nums'''


def test_get_supported_curve_info_sha_magic_nums():
    get_supported_curve_info_sha_magic_nums()


'''test_get_sha_magic_num_size'''


def test_get_sha_magic_num_size():
    get_sha_magic_num_size_sha_magic_num = mock.MagicMock()
    get_sha_magic_num_size(get_sha_magic_num_size_sha_magic_num)


'''test_password_callback'''

@unittest.skip('May need user interaction?')
def test_password_callback():
    password_callback_buf = mock.MagicMock()
    password_callback_bufsiz = 16
    password_callback_verify = mock.MagicMock()
    password_callback_cb_temp = mock.MagicMock()
    password_callback(
        password_callback_buf,
        password_callback_bufsiz,
        password_callback_verify,
        password_callback_cb_temp)


class test_openssl(unittest.TestCase):
    '''test_close'''

    def setUp(self):
        self._openssl_version = '1.1.1d'

    def test_close(self):
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        openssl_test.close()

    '''test_generate_group'''

    def test_generate_group(self):
        self.skipTest('Is this used?')
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        generate_group_nid = 0
        openssl_test.generate_group(generate_group_nid)

    '''test_generate_key'''

    def test_generate_key(self):
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        generate_key_group = None
        openssl_test.generate_key(generate_key_group)

    '''test_generate_private_pem'''

    def test_generate_private_pem(self):
        self.skipTest('Is this used?')
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        generate_private_pem_pem = mock.MagicMock()
        generate_private_pem_group = mock.MagicMock()
        generate_private_pem_key = mock.MagicMock()
        generate_private_pem_encrypt = mock.MagicMock()
        openssl_test.generate_private_pem(
            generate_private_pem_pem,
            generate_private_pem_group,
            generate_private_pem_key,
            generate_private_pem_encrypt)

    '''test_generate_public_pem'''

    def test_generate_public_pem(self):
        self.skipTest('Is this used?')
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        generate_public_pem_pem = mock.MagicMock()
        generate_public_pem_key = mock.MagicMock()
        openssl_test.generate_public_pem(
            generate_public_pem_pem, generate_public_pem_key)

    '''test_read_private_key'''
    @unittest.skip("Figure our reading pem file")
    def test_read_private_key(self):
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        read_private_key_private_pem = 'private.pem'
        openssl_test.read_private_key(read_private_key_private_pem)

    '''test_read_public_key'''

    @unittest.skip("Figure our reading pem file")
    def test_read_public_key(self):
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        read_public_key_public_pem = 'public.pem'
        openssl_test.read_public_key(read_public_key_public_pem)

    '''test_generate_ec_key_using_xy_and_curve_info'''

    def test_generate_ec_key_using_xy_and_curve_info(self):
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        generate_ec_key_using_xy_and_curve_info_xy = mock.MagicMock()
        generate_ec_key_using_xy_and_curve_info_curve_info = mock.MagicMock(return_value=secrets.token_hex(1))
        openssl_test.generate_ec_key_using_xy_and_curve_info(
            generate_ec_key_using_xy_and_curve_info_xy,
            generate_ec_key_using_xy_and_curve_info_curve_info.return_value)

    '''test_get_bignum_from_byte_array'''

    def test_get_bignum_from_byte_array(self):
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        get_bignum_from_byte_array_byte_array = mock.MagicMock(return_value=TEST_DATA)
        openssl_test.get_bignum_from_byte_array(
            get_bignum_from_byte_array_byte_array.return_value)

    '''test_get_bignums_from_byte_array'''

    def test_get_bignums_from_byte_array(self):
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        get_bignums_from_byte_array_byte_array = mock.MagicMock(return_value=TEST_DATA)
        openssl_test.get_bignums_from_byte_array(
            get_bignums_from_byte_array_byte_array.return_value)

    '''test_convert_byte_array_to_char_pointer'''

    def test_convert_byte_array_to_char_pointer(self):
        self.skipTest('Is this used?')
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        convert_byte_array_to_char_pointer_byte_array = mock.MagicMock()
        openssl_test.convert_byte_array_to_char_pointer(
            convert_byte_array_to_char_pointer_byte_array)

    '''test_get_sha256'''

    def test_get_sha256(self):
        self.skipTest('Is this used?')
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        get_sha256_data = mock.MagicMock()
        get_sha256_size = mock.MagicMock()
        get_sha256_sha = mock.MagicMock()
        openssl_test.get_sha256(
            get_sha256_data,
            get_sha256_size,
            get_sha256_sha)

    '''test_get_sha384'''

    def test_get_sha384(self):
        self.skipTest('Is this used?')
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        get_sha384_data = mock.MagicMock()
        get_sha384_size = mock.MagicMock()
        get_sha384_sha = mock.MagicMock()
        openssl_test.get_sha384(
            get_sha384_data,
            get_sha384_size,
            get_sha384_sha)

    '''test_get_sha512'''

    def test_get_sha512(self):
        self.skipTest('Is this used?')
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        get_sha512_data = mock.MagicMock()
        get_sha512_size = mock.MagicMock()
        get_sha512_sha = mock.MagicMock()
        openssl_test.get_sha512(
            get_sha512_data,
            get_sha512_size,
            get_sha512_sha)

    '''test_get_byte_array_sha'''

    def test_get_byte_array_sha(self):
        self.skipTest('Is this used?')
        init_version = self._openssl_version
        openssl_test = openssl(init_version)
        get_byte_array_sha_type = mock.MagicMock()
        get_byte_array_sha_bytes = mock.MagicMock()
        openssl_test.get_byte_array_sha(
            get_byte_array_sha_type,
            get_byte_array_sha_bytes)
