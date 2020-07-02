import unittest
from unittest import mock
from pacsign.pacsign import (get_manager_names,
                             add_common_options,
                             answer_y_n)
'''test_get_manager_names'''

def test_get_manager_names():
    get_manager_names_append_manager = mock.MagicMock()
    get_manager_names(get_manager_names_append_manager)

'''test_add_common_options'''

def test_add_common_options():
    add_common_options_parser = mock.MagicMock()
    add_common_options(add_common_options_parser)

'''test_answer_y_n'''

def test_answer_y_n():
    answer_y_n_args = mock.MagicMock()
    answer_y_n_question = mock.MagicMock()
    answer_y_n(answer_y_n_args, answer_y_n_question)

