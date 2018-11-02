#! /usr/bin/env python
# Copyright(c) 2018, Intel Corporation
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

"""

"""
import inspect
import math


class Bytes(object):
    def __init__(self, count):
        parents = inspect.getmro(type(self))
        self._count = float(count)
        self._bytes = float(count) * math.pow(1024, len(parents)-2)

    def count(self):
        return self._bytes

    def __int__(self):
        return int(self._bytes)

    def __float__(self):
        return float(self._bytes)

    def __add__(self, other):
        return self._bytes + float(other)

    def __sub__(self, other):
        return self._bytes - float(other)

    def __mul__(self, other):
        return self._bytes * float(other)

    def __div__(self, other):
        return self._bytes / float(other)

    def __le__(self, other):
        return self._bytes <= float(other)

    def __ge__(self, other):
        return self._bytes >= float(other)

    def __lt__(self, other):
        return self._bytes < float(other)

    def __gt__(self, other):
        return self._bytes > float(other)

    def __str__(self):
        return '{} {}'.format(self._count, self.__class__.__name__)


class KiB(Bytes):
    pass


class MiB(KiB):
    pass


class GiB(MiB):
    pass
