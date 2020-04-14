# -*- coding: UTF-8 -*-
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
from __future__ import absolute_import
import codecs
import os
import sys
import threading
import time
from datetime import timedelta

from opae.admin.utils.log import loggable

if sys.version_info[0] == 3:  # pragma: no cover
    from io import IOBase as _ftype

    def wrap_stream(stream):
        return stream
else:
    _ftype = file  # noqa pylint: disable=E0602

    def wrap_stream(stream):
        utf_writer = codecs.getwriter('UTF-8')
        return utf_writer(stream)


class progress(loggable):
    """progress is a class to show a progress bar on a stream or callback"""
    BAR = u'\u2588'

    def __init__(self, **kwargs):
        """__init__

        Args:
            **kwargs: keyword arguments used for configuring progress.
                bytes: Total bytes size for reporting progress relative to the
                       given byte size.
                time: Total estimated time for reporting progress relative to
                      the given time.
                bars: Total number of bars (affects the width of the whole
                      progress bar).
                bar: Character to dislplay for each increment.
                     Default is unicode 2588 (█)
                stream: File stream to write progress to.
                        Default is sys.stdout
                log: log function to call. If None, will write to stream.
                null: If set to True, the progress reporting function becomes a
                      'noop' function.
                label: use a label at exit of context. If label is '<thread>',
                       use the current thread name.
        """
        super(progress, self).__init__()
        self._total_size = kwargs.get('bytes')
        self._total_time = kwargs.get('time')
        self._total_bars = kwargs.get('bars', 20)
        self._units = kwargs.get('units', 'bytes')
        self._bar = kwargs.get('bar', self.BAR)
        self._logfn = kwargs.get('log')
        self._start_time = time.time()
        self._last_pct = 0
        self._line_ending = '\r'
        label = kwargs.get('label', '')
        if label == '<thread>':
            self._label = threading.current_thread().name
        else:
            self._label = label

        stream = kwargs.get('stream', sys.stdout)
        if self._total_time == 0:
            self._total_time = 1.0

        if stream is not None:
            if hasattr(stream, 'write'):
                self._stream = wrap_stream(stream)
                if not os.isatty(stream.fileno()):
                    self._line_ending = '\n'
            else:
                self.log.error('stream object is not writable')
                self._stream = wrap_stream(sys.stdout)

        if kwargs.get('null', False):
            self.update_percent = self._noop
        else:
            self.update_percent = self._update_percent

    def _noop(self, *args, **kwargs):
        pass

    def _update_percent(self, pct, ratio=None):
        """_update_percent Update progress bar based on the percent given.

        Args:
            pct (float): A percent ratio [0..1]
            ratio(list or tuple): A two element sequence indicating ratio.
                                  When provided, this will include ratio in
                                  the update. Default is None.

        Returns:
            The update string if not configured with either a stream or a log
            function, otherwise, None.
        """
        percent = int(pct*100)
        num_bars = int(pct*self._total_bars)
        bars = self.BAR*num_bars
        spaces = ' '*(self._total_bars - num_bars)
        text = u'({percent:3}%) [{bars}{spaces}]'.format(**locals())
        if ratio is not None and len(ratio) == 2:
            text += u' [{}/{} {}]'.format(ratio[0], ratio[1], self._units)

        elapsed = timedelta(seconds=time.time() - self._start_time)
        text += u'[Time:{}]'.format(elapsed)
        if self._label:
            text += u'[{}]'.format(self._label)

        if self._logfn is None and self._stream is None:
            return text

        if percent > self._last_pct:
            self._last_pct = percent
            if self._logfn is not None:
                self._logfn(text)
            else:
                self._stream.write(u'{:80}{}'.format(text, self._line_ending))
                self._stream.flush()

    def update(self, n_bytes):
        """update Update the total number of bytes.

        Args:
            n_bytes (int): Total number of bytes processed so far.
        """
        if self._total_size is None:
            raise RuntimeError('progress not configured with "bytes" value')
        pct = float(n_bytes)/self._total_size
        return self.update_percent(pct, ratio=[n_bytes, self._total_size])

    def tick(self):
        """tick Update elapsed time by getting the current time."""
        if self._total_time is None:
            raise RuntimeError('progress not configured with "time" value')
        elapsed = time.time() - self._start_time
        pct = elapsed/self._total_time
        return self.update_percent(pct)

    def begin(self):
        """begin Initialize the progress bar (reset time to zero)"""
        self._start_time = time.time()
        return self.update_percent(0)

    def end(self):
        """end Complete the progress bar (set percent to 1.0)"""
        if self._total_size:
            return self.update(self._total_size)
        return self.update_percent(1)

    def __enter__(self):
        self.begin()
        return self

    def __exit__(self, ex_type, ex_value, ex_tb):
        if ex_type is None:
            self.end()
        self._stream.write(u'\n')
