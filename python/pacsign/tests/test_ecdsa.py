import unittest
from unittest import mock
from pacsign.ecdsa import (inverse_mod,
                           is_on_curve,
                           point_neg,
                           point_add,
                           scalar_mult,
                           verify_signature)
BASE_PT = (
        0x6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296,
        0x4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5,
        )
TEST_SIGNATURE = (
        121110987654321,
        123456789101112
        )

'''test_inverse_mod'''

def test_inverse_mod():
    inverse_mod_k = mock.MagicMock(return_value=10)
    inverse_mod_p = mock.MagicMock(return_value=17)
    inverse_mod(inverse_mod_k.return_value, inverse_mod_p.return_value)

'''test_is_on_curve'''

def test_is_on_curve():
    is_on_curve_point = mock.MagicMock(return_value = BASE_PT)
    is_on_curve(is_on_curve_point.return_value)

'''test_point_neg'''

def test_point_neg():
    point_neg_point = mock.MagicMock(return_value = BASE_PT)
    point_neg(point_neg_point.return_value)

'''test_point_add'''

def test_point_add():
    point_add_point1 = mock.MagicMock(return_value = BASE_PT)
    point_add_point2 = mock.MagicMock(return_value = BASE_PT)
    point_add(point_add_point1.return_value, point_add_point2.return_value)

'''test_scalar_mult'''

def test_scalar_mult():
    scalar_mult_k = mock.MagicMock()
    scalar_mult_point = mock.MagicMock(return_value = BASE_PT)
    scalar_mult(int(scalar_mult_k), scalar_mult_point.return_value)

'''test_verify_signature'''

def test_verify_signature():
    verify_signature_public_key = mock.MagicMock()
    verify_signature_message = mock.MagicMock()
    verify_signature_signature = mock.MagicMock()
    verify_signature(verify_signature_public_key, verify_signature_message, verify_signature_signature)

