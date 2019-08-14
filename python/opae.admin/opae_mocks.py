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
import contextlib
import tempfile
import sys

if sys.version_info[0] == 3:
    open_string = 'builtins.open'
else:
    open_string = '__builtin__.open'


class mock_open(object):
    real_open = open

    def __init__(self, path):
        self._path = path

    def __call__(self, path, *args, **kwargs):
        if path == self._path:
            return tempfile.NamedTemporaryFile(prefix='opae_mock_')
        else:
            return self.real_open(path, *args, **kwargs)


class mock_ioctl(object):
    def __init__(self):
        self._requests = {}

    @contextlib.contextmanager
    def register(self, req, cb):
        self._requests[req] = cb
        try:
            yield self
        finally:
            del(self._requests[req])

    def __call__(self, fd, opt, arg=0, mutate=False):
        if opt in self._requests:
            return self._requests[opt](fd, opt, arg, mutate)
        else:
            raise KeyError('request not registered: {}'.format(opt))
