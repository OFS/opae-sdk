# Copyright(c) 2019, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

##########################
#
# Common Util API
#
##########################
import os
import sys
from opae.admin.utils import terminal
import array
import inspect
from ctypes import *

def answer_y_n(args, question):
    if args.yes:
        return True
    ans = False
    while True:
        overwrite = input('{}? Y = yes, N = no: '.format(question))
        if overwrite.lower() == 'y':
            ans = True
            break
        if overwrite.lower() == 'n':
            break
    return ans


def print_new_line():

    terminal.printing("", terminal.MSG_TYPE.NULL, terminal.BCOLORS.INFO, 0, None, False)


def print_info(string, space=0, file=None, alternate_color=False):

    terminal.printing(
        string,
        terminal.MSG_TYPE.INFO,
        terminal.BCOLORS.INFO,
        space,
        file,
        alternate_color,
    )


def print_warning(string, space=0, file=None):

    terminal.printing(
        string, terminal.MSG_TYPE.WARNING, terminal.BCOLORS.WARNING, space, file
    )


def print_error(string, space=0, file=None):

    terminal.printing(
        string, terminal.MSG_TYPE.ERROR, terminal.BCOLORS.ERROR, space, file
    )


def print_prompt(string, space=0, file=None):

    terminal.printing(
        string, terminal.MSG_TYPE.NULL, terminal.BCOLORS.PROMPT, space, file
    )


def exception_handler(etype, value, tb):

    pass


def assert_in_error(boolean, string, *arg):

    if boolean:

        pass

    elif len(string):

        if len(arg):
            string = string % (arg)

        # contruct message
        caller = inspect.stack()[1]
        module = os.path.basename(caller[1])
        info = "Module: %s, Function: %s, Line: %d" % (module, caller[3], caller[2])
        msg = "%s\n       %s" % (info, string)
        print_error(msg, space=0)
        sys.excepthook = exception_handler
        assert False, msg

    else:

        # contruct message
        caller = inspect.stack()[1]
        module = os.path.basename(caller[1])
        msg = "Module: %s, Function: %s, Line: %d" % (module, caller[3], caller[2])
        print_error(msg, space=0)
        sys.excepthook = exception_handler
        assert False, msg


def change_folder_seperator(fullpath):

    fullpath = fullpath.replace("\\", "/")
    fullpath = fullpath.replace("//", "/")
    return fullpath


def get_filename(fullpath, space=0):

    fullpath = change_folder_seperator(fullpath)
    filename = fullpath
    if len(fullpath):
        index = fullpath.rfind("/")
        if index != 0:
            filename = fullpath[index + 1 :]
    else:
        print_error("Input is NULL function get_filename()", space=space)
        sys.exit(-1)
    return filename


def check_extension(file, extension):

    status = True
    file_char_count = len(file)
    extension_char_count = len(extension)
    if (
        file_char_count == 0
        or extension_char_count == 0
        or file_char_count < (extension_char_count + 1)
        or extension != file[(file_char_count - extension_char_count) :]
    ):
        status = False
    return status


def check_extensions(file, extensions):

    final_index = 0
    index = 0
    for extension in extensions:
        status = check_extension(file, extension)
        if status:
            final_index |= 1 << index
            break
        index += 1
    return final_index


def get_unit_size(size, unit_size):

    assert unit_size > 0
    return int((size + unit_size - 1) / unit_size)


def get_byte_size(bit):

    return get_unit_size(bit, 8)


def get_standard_hex_string(data):

    string = ""
    try:
        for d in data:
            string = "%s%02X" % (string, ord(d) & 0xFF)
    except TypeError:
        for d in data:
            string = "%s%02X" % (string, d & 0xFF)
    return string


def get_reversed_hex_string(data):

    string = ""
    try:
        for d in data:
            string = "%02X%s" % (ord(d) & 0xFF, string)
    except TypeError:
        for d in data:
            string = "%02X%s" % (d & 0xFF, string)
    return string


class BYTE_ARRAY:
    def __init__(self, type=None, arg=None):

        self.data = array.array("B")
        if type == "FILE":
            assert arg is not None
            statinfo = os.stat(arg)
            file = open(arg, "rb")
            self.data.fromfile(file, statinfo.st_size)
            file.close()
        elif type == "STRING":
            assert arg is not None
            self.data.fromstring(arg)
        elif type == "HEXSTRING":
            assert arg is not None
            assert (
                len(arg) % 2 == 0
            ), "Hex String must be multiple of 2 hexadecimal character"
            while len(arg):
                self.data.append(int(arg[:2], 16))
                arg = arg[2:]
        elif type == "BITSTREAM":
            assert arg is not None and len(arg)
            for a in arg:
                self.data.append(a)
        else:
            assert type is None
            assert arg is None

    def __enter__(self):

        return self

    def __del__(self):

        self.clean()

    def __exit__(self, exception_type, exception_value, traceback):

        self.clean()

    def clean(self):

        if self.data is not None:
            self.null_data()
            del self.data
            self.data = None

    def size(self):

        return len(self.data)

    def append_byte(self, data):

        self.data.append(data & 0xFF)

    def tofile(self, file):

        self.data.tofile(file)

    def append_word(self, data):

        for i in range(2):

            self.data.append(data & 0xFF)
            data >>= 8

    def append_dword(self, data):

        for i in range(4):

            self.data.append(data & 0xFF)
            data >>= 8

    def append_qword(self, data):

        for i in range(8):

            self.data.append(data & 0xFF)
            data >>= 8

    def append_data(self, chars):

        try:
            for char in chars:
                self.data.append(char)
        except TypeError:
            for char in chars:
                self.data.append(ord(char))

    def append_data_swizzled(self, chars):

        try:
            for char in chars:
                self.data.append(int("{:08b}".format(char)[::-1], 2))
        except TypeError:
            for char in chars:
                self.data.append(ord(int("{:08b}".format(char)[::-1], 2)))

    def assign_word(self, offset, word):

        for i in range(2):
            assert offset < self.size()
            self.data[offset] = word & 0xFF
            word >>= 8
            offset += 1

    def assign_dword(self, offset, dword):

        for i in range(4):
            assert offset < self.size()
            self.data[offset] = dword & 0xFF
            dword >>= 8
            offset += 1

    def assign_qword(self, offset, qword):

        for i in range(8):
            assert offset < self.size()
            self.data[offset] = qword & 0xFF
            qword >>= 8
            offset += 1

    def assign_data(self, offset, chars):

        assert (offset + len(chars)) <= self.size()
        try:
            for char in chars:
                self.data[offset] = char
                offset += 1
        except TypeError:
            for char in chars:
                self.data[offset] = ord(char)
                offset += 1

    def null_data(self):

        for i in range(self.size()):
            self.data[i] = 0

    def clear_data(self):

        self.null_data()
        while self.size():
            self.data.pop()

    def get_word(self, offset):

        word = 0
        shift = 0
        for i in range(2):
            assert offset < self.size()
            word |= (self.data[offset] & 0xFF) << shift
            offset += 1
            shift += 8
        return word

    def get_dword(self, offset):

        dword = 0
        shift = 0
        for i in range(4):
            assert (
                offset < self.size()
            ), "Data size is %d, attemp to access index %d" % (self.size(), offset)
            dword |= (self.data[offset] & 0xFF) << shift
            offset += 1
            shift += 8
        return dword

    def get_qword(self, offset):

        qword = 0
        shift = 0
        for i in range(8):
            assert (
                offset < self.size()
            ), "Data size is %d, attemp to access index %d" % (self.size(), offset)
            qword |= (self.data[offset] & 0xFF) << shift
            offset += 1
            shift += 8
        return qword

    def get_string(self, offset, size):

        assert size
        string = ""
        null = False
        for i in range(size):
            assert offset < self.size()
            if self.data[offset] == 0:
                null = True
            else:
                assert not null, "Fail to read string at Byte %d" % offset
                string = "%s%c" % (string, chr(self.data[offset]))
            offset += 1
        return string

    def resize(self, size):

        assert size
        while len(self.data) > size:
            self.data[len(self.data) - 1] = 0
            self.data.pop()
        while len(self.data) < size:
            self.data.append(0)


class CHAR_POINTER:
    def __init__(self, size):

        assert size > 0
        c_chars = c_char * size
        self.data = c_chars()
        self.null_data()

    def __del__(self):

        self.clean()

    def __exit__(self, exception_type, exception_value, traceback):

        self.clean()

    def clean(self):

        if self.data is not None:
            self.null_data()
            del self.data
            self.data = None

    def size(self):

        return len(self.data)

    def null_data(self):

        for i in range(self.size()):
            try:
                self.data[i] = chr(0)
            except TypeError:
                self.data[i] = 0

    def assign_data(self, chars):

        assert self.size() == len(chars)
        for i in range(self.size()):
            try:
                self.data[i] = chr(chars[i])
            except TypeError:
                self.data[i] = chars[i]

    def assign_partial_data(self, chars, source_offset, dest_offset, size):

        assert source_offset < len(chars)
        assert (source_offset + size) <= len(chars)
        assert dest_offset < self.size()
        assert (dest_offset + size) <= self.size()
        for i in range(size):
            self.data[dest_offset + i] = chars[source_offset + i]

    def compare_data(self, chars, error):

        assert self.size() == len(chars)
        if id(self.data) == id(chars):
            for i in range(self.size()):
                assert_in_error(self.data[i] == chars[i], error)
        else:
            for i in range(self.size()):
                assert_in_error(ord(self.data[i]) == chars[i], error)

    def get_dword(self, offset):

        dword = 0
        shift = 0
        for i in range(4):
            assert (
                offset < self.size()
            ), "Data size is %d, attemp to access index %d" % (self.size(), offset)
            dword |= (ord(self.data[offset]) & 0xFF) << shift
            offset += 1
            shift += 8
        return dword

    def get_standard_hex_string(self, offset, size):

        assert size
        string = ""
        for i in range(size):
            assert offset < self.size()
            string = "%s%02X" % (string, ord(self.data[offset]))
            offset += 1
        return string
