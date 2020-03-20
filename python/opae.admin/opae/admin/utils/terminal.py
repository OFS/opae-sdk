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
import sys
import os
import struct
from sys import platform as _platform
from ctypes import (byref, c_short, c_ushort, create_string_buffer,
                    windll, Structure)


class MSG_TYPE:
    INFO = "Info"
    WARNING = "Warning"
    ERROR = "Error"
    PROMPT = "Prompt"
    CMD = "CMD"
    BG_CMD = "BG CMD"
    NULL = ""


if _platform == "win32" or _platform == "win64":
    STD_INPUT_HANDLE = -10
    STD_OUTPUT_HANDLE = -11
    STD_ERROR_HANDLE = -12

    SHORT = c_short
    WORD = c_ushort

    class COORD(Structure):

        _fields_ = [("X", SHORT), ("Y", SHORT)]

    class SMALL_RECT(Structure):

        _fields_ = [
            ("Left", SHORT),
            ("Top", SHORT),
            ("Right", SHORT),
            ("Bottom", SHORT),
        ]

    class CONSOLE_SCREEN_BUFFER_INFO(Structure):

        _fields_ = [
            ("dwSize", COORD),
            ("dwCursorPosition", COORD),
            ("wAttributes", WORD),
            ("srWindow", SMALL_RECT),
            ("dwMaximumWindowSize", COORD),
        ]

    class BCOLORS:
        PROMPT = 0x0009
        INFO = 0x0002
        WARNING = 0x0003
        ERROR = 0x0004
        HELP = 0x0005
        CMD = 0x0007
        FOREGROUND_INTENSITY = 0x0008

    stdout_handle = windll.kernel32.GetStdHandle(STD_OUTPUT_HANDLE)
    stderr_handle = windll.kernel32.GetStdHandle(STD_ERROR_HANDLE)
    SetConsoleTextAttribute = windll.kernel32.SetConsoleTextAttribute
    GetConsoleScreenBufferInfo = windll.kernel32.GetConsoleScreenBufferInfo
    screen_csbi = create_string_buffer(22)

    def get_text_attr():
        csbi = CONSOLE_SCREEN_BUFFER_INFO()
        GetConsoleScreenBufferInfo(stdout_handle, byref(csbi))
        return csbi.wAttributes

    def set_text_attr(color):
        SetConsoleTextAttribute(stdout_handle, color)

    default_colors = get_text_attr()
    default_bg = default_colors & 0x0070
    default_fg = default_colors & 0x0007
else:

    class BCOLORS:

        INFO = "\033[92m"
        WARNING = "\033[96m"
        ERROR = "\033[91m"
        HELP = "\033[95m"
        CMD = "\033[90m"
        # Window not assigned yet
        PROMPT = "\033[94m"
        ENDC = "\033[0m"
        BOLD = "\033[1m"
        UNDERLINE = "\033[4m"


NO_COLOR = False


def printing(string, msg_type, bcolor, space, file, local_no_color=False):

    if msg_type == MSG_TYPE.NULL:

        string = "%s%s" % (" " * space * 4, string)

    else:

        string = "%s%s: %s" % (" " * space * 4, msg_type, string)

    if NO_COLOR or local_no_color:

        print(string)
        if sys.stdout is not None:
            sys.stdout.flush()

    elif _platform == "win32" or _platform == "win64":

        set_text_attr(bcolor | default_bg | BCOLORS.FOREGROUND_INTENSITY)
        print(string)
        if sys.stdout is not None:
            sys.stdout.flush()
        set_text_attr(default_colors)

    else:

        print(bcolor + string + BCOLORS.ENDC)
        if sys.stdout is not None:
            sys.stdout.flush()

    if file is not None:

        file.write(string)
        file.write("\n")


if _platform == "win32" or _platform == "win64":

    def get_size():
        res = windll.kernel32.GetConsoleScreenBufferInfo(stderr_handle,
                                                         screen_csbi)
        if res:
            (
                bufx,
                bufy,
                curx,
                cury,
                wattr,
                left,
                top,
                right,
                bottom,
                maxx,
                maxy,
            ) = struct.unpack("hhhhHhhhhhh", screen_csbi.raw)
            sizex = right - left + 1
            sizey = bottom - top + 1
        else:
            # can't determine actual size - return default values
            sizex, sizey = 80, 25

        return (sizex, sizey)


else:

    def get_size():
        env = os.environ

        def ioctl_gwin_size(fd):
            try:
                import fcntl
                import termios

                cr = struct.unpack("hh", fcntl.ioctl(fd, termios.TIOCGWINSZ,
                                                     "1234"))
            except Exception:
                return
            return cr

        cr = ioctl_gwin_size(0) or ioctl_gwin_size(1) or ioctl_gwin_size(2)
        if not cr:
            try:
                fd = os.open(os.ctermid(), os.O_RDONLY)
                cr = ioctl_gwin_size(fd)
                os.close(fd)
            except Exception:
                pass
        if not cr:
            cr = (env.get("LINES", 25), env.get("COLUMNS", 80))
        return int(cr[1]), int(cr[0])


def set_no_color():

    global NO_COLOR
    NO_COLOR = True
