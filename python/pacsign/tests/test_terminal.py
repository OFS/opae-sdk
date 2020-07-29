import sys
import unittest
from unittest import mock


from pacsign.terminal import (printing,
                              set_no_color,
                              get_size)

if sys.platform.startswith('win'):
    '''test_get_text_attr'''


    def test_get_text_attr():
        get_text_attr()


    '''test_set_text_attr'''


    def test_set_text_attr():
        set_text_attr_color = mock.MagicMock()
        set_text_attr(set_text_attr_color)


'''test_printing'''


def test_printing():
    printing_string = mock.MagicMock()
    printing_msg_type = mock.MagicMock()
    printing_bcolor = mock.MagicMock()
    printing_space = mock.MagicMock()
    printing_file = mock.MagicMock()
    printing_local_no_color = mock.MagicMock()
    printing(
        printing_string,
        printing_msg_type,
        printing_bcolor,
        printing_space,
        printing_file,
        printing_local_no_color)


'''test_get_size'''


def test_get_size():
    get_size()


'''test_set_no_color'''


def test_set_no_color():
    set_no_color()
