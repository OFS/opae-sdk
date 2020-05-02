import unittest
from unittest import mock
from pacsign.database import (FILE_TYPE_DATABASE,
                              FAMILY_DATABASE,
                              CURVE_INFO,
                              get_curve_info_from_name)
'''test_get_type_from_enum'''

def test_get_type_from_enum():
    init_family = mock.MagicMock()
    init_supported_types = mock.MagicMock()
    init_max_csk = mock.MagicMock()
    init_signature_max_size = mock.MagicMock()
    init_supported_cancels = mock.MagicMock()
    init_cert_types = mock.MagicMock()
    FAMILY_DATABASE_test = FAMILY_DATABASE(init_family, init_supported_types, init_max_csk, init_signature_max_size, init_supported_cancels, init_cert_types)
    get_type_from_enum_enum = mock.MagicMock()
    FAMILY_DATABASE_test.get_type_from_enum(get_type_from_enum_enum)

'''test_get_cert_type_from_enum'''

def test_get_cert_type_from_enum():
    init_family = mock.MagicMock()
    init_supported_types = mock.MagicMock()
    init_max_csk = mock.MagicMock()
    init_signature_max_size = mock.MagicMock()
    init_supported_cancels = mock.MagicMock()
    init_cert_types = mock.MagicMock()
    FAMILY_DATABASE_test = FAMILY_DATABASE(init_family, init_supported_types, init_max_csk, init_signature_max_size, init_supported_cancels, init_cert_types)
    get_cert_type_from_enum_enum = mock.MagicMock()
    FAMILY_DATABASE_test.get_cert_type_from_enum(get_cert_type_from_enum_enum)

'''test_get_curve_info_from_name'''

def test_get_curve_info_from_name():
    get_curve_info_from_name_name = mock.MagicMock()
    get_curve_info_from_name(get_curve_info_from_name_name)

