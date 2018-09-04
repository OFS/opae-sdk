#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright © 2018 rrojo <rrojo@ub-rojo-vm-04.amr.corp.intel.com>
#
# Distributed under terms of the MIT license.

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
