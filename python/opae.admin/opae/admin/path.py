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
import os
import re

MOUNT_PATTERN = (r'(?P<device>\w+)\s+(?P<mountdir>[\w\/]+)\s+'
                 r'(?P<fstype>\w+)\s+(?P<opts>[\w\d=,]+)\s(.*)')
MOUNT_RE = re.compile(MOUNT_PATTERN)


def get_mounts():
    mounts = {'sysfs': '/sys', 'udev': '/dev'}
    mocks = []
    with open('/proc/mounts', 'r') as fp:
        for l in fp:
            m = MOUNT_RE.match(l)
            if m:
                if m.group('device') in mounts:
                    mounts[m.group('device')] = m.group('mountdir')
                elif m.group('device') == 'mock_sysfs':
                    mocks.append(m.group('mountdir'))
    mounts['mocks'] = mocks
    return mounts


_mounts = get_mounts()

sysfs = _mounts.get('sysfs', '/sys')
udev = _mounts.get('udev', '/dev')


def sysfs_path(path, *paths):
    if path.startswith('/sys'):
        relpath = os.path.relpath(path, '/sys')
    else:
        relpath = path
    return os.path.join(sysfs, relpath, *paths)


def device_path(path, *paths):
    if path.startswith('/dev'):
        relpath = os.path.relpath(path, '/dev')
    else:
        relpath = path
    return os.path.join(udev, relpath, *paths)
