# Copyright(c) 2019-2020, Intel Corporation
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
import tempfile
import time
import sys
import unittest
import io
import re

from opae.admin.utils.progress import progress

if sys.version_info[0] == 3:
    temp_cfg = {'encoding': sys.getdefaultencoding()}
else:
    temp_cfg = {}

def get_percent(line):
    """
       Args:
          line: A progress string containing percent complete

       Return:
          integer of current percent value if found, otherwise None
    """
    m = re.search(r'\d+', line)
    if m:
        try:
            return int(m.group())
        except ValueError:
            return None

class test_progress(unittest.TestCase):
    TEST_SIZE = 4096*100

    def test_notty(self):
        """test_notty
           Given a valid progress object
           When the time is set to a valid integer and stream is set to a file pointer
           Then it outputs progress bar stream to file
           When I read the file by line
           And get the current percentage
           Then the current percentage is greater than the previous percentage
           When the current pertagtage exceeds 100
           Then it stops reading and exits the file
        """
        with tempfile.NamedTemporaryFile(mode='w+',
                                         **temp_cfg) as stream:
            with progress(time=10, stream=stream) as prg:
                for _ in range(100):
                    prg.tick()
                    time.sleep(0.1)

            stream.seek(0)
            line = stream.readline().strip('\n')
            p_percent = 0
            while line:
                c_percent = get_percent(line)
                if c_percent is not None:
                    self.assertGreater(c_percent, p_percent)
                    p_percent = c_percent
                    line = stream.readline()
                else:
                    break

    def test_time0(self):
        """test_time0
           Given a valid progress object
           When time is set to zero
           Then the progress bar is streamed to stdout
           And the test succeeds
        """
        with progress(time=0) as prg:
            for _ in range(10):
                prg.tick()
                time.sleep(0.1)

    def test_label(self):
        """test_label
           Given a valid progress object with a label
           When time is set to zero
           Then the progress bar is streamed to stdout
           And the label is shown
           And the test succeeds
        """
        with progress(time=0, label='Writing') as prg:
            for _ in range(10):
                prg.tick()
                time.sleep(0.1)

    def test_noop(self):
        """test_noop
           Given a valid progress object
           When time is set to a valid integer and null is set to True
           Then progress reporting tool becomes a 'noop' method
        """
        with progress(time=0, null=True) as prg:
            for _ in range(10):
                prg.tick()
                time.sleep(0.1)

    def test_stream_readonly(self):
        """test_stream_readonly
           Given a valid progress object
           When time is set to a valid integer
           And stream is set with read-only object
           Then it prints Error and redirects stream to stdout
        """
        stream = io.IOBase()
        with progress(time=10, stream=stream) as prg:
            for _ in range(100):
                prg.tick()
                time.sleep(0.1)

    def test_time_set_none(self):
        """test_time_set_none
           Given a valid progress object
           When I set time to none
           Then it exits with RunTimeError exception
        """
        with progress(time=None) as prg:
            for _ in range(10):
                with self.assertRaises(RuntimeError):
                    prg.tick()
                    time.sleep(0.1)

    def test_bytes_set_none(self):
        """test_bytes_set_none
           Given a valid progress object
           When I set bytes to none
           Then it exits with RunTimeError exception
        """
        with progress(bytes=None) as prg:
            for _ in range(10):
                with self.assertRaises(RuntimeError):
                    prg.update(self.TEST_SIZE)
                    time.sleep(0.1)
