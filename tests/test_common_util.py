import unittest
from unittest import mock
from pacsign.common_util import (print_new_line,
                                 print_info,
                                 print_warning,
                                 print_error,
                                 print_prompt,
                                 exception_handler,
                                 run_command,
                                 assert_in_error,
                                 change_folder_seperator,
                                 get_filename,
                                 is_windows_os,
                                 check_extension,
                                 check_extensions,
                                 get_unit_size,
                                 get_byte_size,
                                 get_password,
                                 get_standard_hex_string,
                                 get_reversed_hex_string,
                                 BYTE_ARRAY,
                                 CHAR_POINTER)
'''test_print_new_line'''

def test_print_new_line():
    print_new_line()

'''test_print_info'''

def test_print_info():
    print_info_string = mock.MagicMock()
    print_info_space = mock.MagicMock()
    print_info_file = mock.MagicMock()
    print_info_alternate_color = mock.MagicMock()
    print_info(print_info_string, print_info_space, print_info_file, print_info_alternate_color)

'''test_print_warning'''

def test_print_warning():
    print_warning_string = mock.MagicMock()
    print_warning_space = mock.MagicMock()
    print_warning_file = mock.MagicMock()
    print_warning(print_warning_string, print_warning_space, print_warning_file)

'''test_print_error'''

def test_print_error():
    print_error_string = mock.MagicMock()
    print_error_space = mock.MagicMock()
    print_error_file = mock.MagicMock()
    print_error(print_error_string, print_error_space, print_error_file)

'''test_print_prompt'''

def test_print_prompt():
    print_prompt_string = mock.MagicMock()
    print_prompt_space = mock.MagicMock()
    print_prompt_file = mock.MagicMock()
    print_prompt(print_prompt_string, print_prompt_space, print_prompt_file)

'''test_exception_handler'''

def test_exception_handler():
    exception_handler_etype = mock.MagicMock()
    exception_handler_value = mock.MagicMock()
    exception_handler_tb = mock.MagicMock()
    exception_handler(exception_handler_etype, exception_handler_value, exception_handler_tb)

'''test_run_command'''

def test_run_command():
    run_command_command = mock.MagicMock()
    run_command_printed_cmd = mock.MagicMock()
    run_command_return_code = mock.MagicMock()
    run_command_allow_error = mock.MagicMock()
    run_command(run_command_command, run_command_printed_cmd, run_command_return_code, run_command_allow_error)

'''test_assert_in_error'''

def test_assert_in_error():
    assert_in_error_boolean = mock.MagicMock()
    assert_in_error_string = mock.MagicMock()
    assert_in_error(assert_in_error_boolean, assert_in_error_string)

'''test_change_folder_seperator'''

def test_change_folder_seperator():
    change_folder_seperator_fullpath = mock.MagicMock()
    change_folder_seperator(change_folder_seperator_fullpath)

'''test_get_filename'''

def test_get_filename():
    get_filename_fullpath = mock.MagicMock()
    get_filename_space = mock.MagicMock()
    get_filename(str(get_filename_fullpath), int(get_filename_space))

'''test_is_windows_os'''

def test_is_windows_os():
    is_windows_os()

'''test_check_extension'''

def test_check_extension():
    check_extension_file = mock.MagicMock()
    check_extension_extension = mock.MagicMock()
    check_extension(check_extension_file, check_extension_extension)

'''test_check_extensions'''

def test_check_extensions():
    check_extensions_file = mock.MagicMock()
    check_extensions_extensions = mock.MagicMock()
    check_extensions(check_extensions_file, check_extensions_extensions)

'''test_get_unit_size'''

def test_get_unit_size():
    get_unit_size_size = mock.MagicMock()
    get_unit_size_unit_size = mock.MagicMock()
    get_unit_size(int(get_unit_size_size), int(get_unit_size_unit_size))

'''test_get_byte_size'''

def test_get_byte_size():
    get_byte_size_bit = mock.MagicMock()
    get_byte_size(get_byte_size_bit)

'''test_get_password'''
@unittest.skip("skipping password test")
def test_get_password():

    get_password_messages = mock.MagicMock(return_value=["this","test"])
    get_password_MIN = mock.MagicMock()
    get_password_MAX = mock.MagicMock()
    get_password_comment = mock.MagicMock()
    get_password(get_password_messages.return_value, int(get_password_MIN), int(get_password_MAX), str(get_password_comment))

'''test_get_standard_hex_string'''

def test_get_standard_hex_string():
    init_size = mock.MagicMock()
    CHAR_POINTER_test = CHAR_POINTER(int(init_size))
    get_standard_hex_string_offset = mock.MagicMock(return_value=0)
    get_standard_hex_string_size = mock.MagicMock()
    CHAR_POINTER_test.get_standard_hex_string(get_standard_hex_string_offset.return_value, int(get_standard_hex_string_size))

'''test_get_reversed_hex_string'''

def test_get_reversed_hex_string():
    get_reversed_hex_string_data = mock.MagicMock()
    get_reversed_hex_string(get_reversed_hex_string_data)

'''test_clean'''

def test_clean():
    init_size = mock.MagicMock()
    CHAR_POINTER_test = CHAR_POINTER(int(init_size))
    CHAR_POINTER_test.clean()

'''test_size'''

def test_size():
    init_size = mock.MagicMock()
    CHAR_POINTER_test = CHAR_POINTER(int(init_size))
    CHAR_POINTER_test.size()

'''test_append_byte'''

def test_append_byte():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    append_byte_data = mock.MagicMock()
    BYTE_ARRAY_test.append_byte(append_byte_data)

'''test_tofile'''
@unittest.skip("Skipping test to learn how to mock a file")
def test_tofile():
    init_type = mock.MagicMock(return_value="FILE")
    init_arg = mock.MagicMock(spec=file, wraps=StringIO('test'))
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, init_arg)
    tofile_file = mock.MagicMock()
    BYTE_ARRAY_test.tofile(tofile_file)

'''test_append_word'''

def test_append_word():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    append_word_data = mock.MagicMock()
    BYTE_ARRAY_test.append_word(append_word_data)

'''test_append_dword'''

def test_append_dword():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    append_dword_data = mock.MagicMock()
    BYTE_ARRAY_test.append_dword(append_dword_data)

'''test_append_qword'''

def test_append_qword():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    append_qword_data = mock.MagicMock()
    BYTE_ARRAY_test.append_qword(append_qword_data)

'''test_append_data'''

def test_append_data():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    append_data_chars = mock.MagicMock()
    BYTE_ARRAY_test.append_data(append_data_chars)

'''test_append_data_swizzled'''

def test_append_data_swizzled():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    append_data_swizzled_chars = mock.MagicMock()
    BYTE_ARRAY_test.append_data_swizzled(append_data_swizzled_chars)

'''test_assign_word'''

def test_assign_word():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    assign_word_offset = mock.MagicMock()
    assign_word_word = mock.MagicMock()
    BYTE_ARRAY_test.assign_word(int(assign_word_offset), assign_word_word)

'''test_assign_dword'''

def test_assign_dword():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    assign_dword_offset = mock.MagicMock()
    assign_dword_dword = mock.MagicMock()
    BYTE_ARRAY_test.assign_dword(int(assign_dword_offset), assign_dword_dword)

'''test_assign_qword'''

def test_assign_qword():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    assign_qword_offset = mock.MagicMock()
    assign_qword_qword = mock.MagicMock()
    BYTE_ARRAY_test.assign_qword(int(assign_qword_offset), assign_qword_qword)

'''test_assign_data'''

def test_assign_data():
    init_size = mock.MagicMock(return_value=8)
    CHAR_POINTER_test = CHAR_POINTER(init_size.return_value)
    assign_data_chars = mock.MagicMock(return_value= b"01234567")
    CHAR_POINTER_test.assign_data(assign_data_chars.return_value)

'''test_null_data'''

def test_null_data():
    init_size = mock.MagicMock()
    CHAR_POINTER_test = CHAR_POINTER(int(init_size))
    CHAR_POINTER_test.null_data()

'''test_clear_data'''

def test_clear_data():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    BYTE_ARRAY_test.clear_data()

'''test_get_word'''

def test_get_word():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    get_word_offset = mock.MagicMock()
    BYTE_ARRAY_test.get_word(int(get_word_offset))

'''test_get_dword'''

def test_get_dword():
    init_size = mock.MagicMock(return_value=4)
    CHAR_POINTER_test = CHAR_POINTER(init_size.return_value)
    get_dword_offset = mock.MagicMock(return_value=0)
    CHAR_POINTER_test.get_dword(get_dword_offset.return_value)

'''test_get_qword'''

def test_get_qword():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    get_qword_offset = mock.MagicMock()
    BYTE_ARRAY_test.get_qword(int(get_qword_offset))

'''test_get_string'''

def test_get_string():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    get_string_offset = mock.MagicMock()
    get_string_size = mock.MagicMock()
    BYTE_ARRAY_test.get_string(int(get_string_offset), int(get_string_size))

'''test_resize'''

def test_resize():
    init_type = mock.MagicMock(return_value="STRING")
    init_arg = mock.MagicMock()
    BYTE_ARRAY_test = BYTE_ARRAY(init_type.return_value, str(init_arg))
    resize_size = mock.MagicMock()
    BYTE_ARRAY_test.resize(int(resize_size))

'''test_assign_partial_data'''

def test_assign_partial_data():
    init_size = mock.MagicMock()
    CHAR_POINTER_test = CHAR_POINTER(int(init_size))
    assign_partial_data_chars = mock.MagicMock(return_value= b"1234")
    assign_partial_data_source_offset = mock.MagicMock()
    assign_partial_data_dest_offset = mock.MagicMock(return_value=0)
    assign_partial_data_size = mock.MagicMock()
    CHAR_POINTER_test.assign_partial_data(assign_partial_data_chars.return_value, int(assign_partial_data_source_offset), assign_partial_data_dest_offset.return_value, int(assign_partial_data_size))

'''test_compare_data'''
@unittest.skip("need to evaluate assert statement")
def test_compare_data():
    init_size = mock.MagicMock()
    CHAR_POINTER_test = CHAR_POINTER(int(init_size))
    compare_data_chars = mock.MagicMock(return_value= "")
    compare_data_error = mock.MagicMock()
    CHAR_POINTER_test.compare_data(compare_data_chars.return_value, compare_data_error)

