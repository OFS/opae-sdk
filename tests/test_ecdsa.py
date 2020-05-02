import unittest
from unittest import mock
from pacsign.ecdsa import (inverse_mod,
                           is_on_curve,
                           point_neg,
                           point_add,
                           scalar_mult,
                           verify_signature)
'''test_inverse_mod'''

def test_inverse_mod():
    inverse_mod_k = mock.MagicMock()
    inverse_mod_p = mock.MagicMock()
    inverse_mod(inverse_mod_k, inverse_mod_p)

'''test_is_on_curve'''

def test_is_on_curve():
    is_on_curve_point = mock.MagicMock()
    is_on_curve(is_on_curve_point)

'''test_point_neg'''

def test_point_neg():
    point_neg_point = mock.MagicMock()
    point_neg(point_neg_point)

'''test_point_add'''

def test_point_add():
    point_add_point1 = mock.MagicMock()
    point_add_point2 = mock.MagicMock()
    point_add(point_add_point1, point_add_point2)

'''test_scalar_mult'''

def test_scalar_mult():
    scalar_mult_k = mock.MagicMock()
    scalar_mult_point = mock.MagicMock()
    scalar_mult(scalar_mult_k, scalar_mult_point)

'''test_verify_signature'''

def test_verify_signature():
    verify_signature_public_key = mock.MagicMock()
    verify_signature_message = mock.MagicMock()
    verify_signature_signature = mock.MagicMock()
    verify_signature(verify_signature_public_key, verify_signature_message, verify_signature_signature)

