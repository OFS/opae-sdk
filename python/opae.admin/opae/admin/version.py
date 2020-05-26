# Copyright(c) 2020, Intel Corporation
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
import subprocess

try:
    from pathlib import Path
except ImportError:
    from pathlib2 import Path

version = {
    'major': 1,
    'minor': 4,
    'patch': 1
}


def _git_root():
    for p in reversed(Path(__file__).parents):
        if next(p.glob('.git'), None):
            return p


def _is_export():
    return '$Format:%h$' == git_hash(short=True, check_dirty=False)


def _is_dirty():
    if not _is_export() and _git_root():
        try:
            return subprocess.call(['git', 'diff-index', '--quiet', 'HEAD']) != 0
        except (OSError, IOError):
            pass
    return False


def git_hash(short=False, check_dirty=True):
    h = '$Format:%H$'
    if short:
        h = h[:7]
    return h + '*' if check_dirty and _is_dirty() else ''


def version_info():
    return (version['major'], version['minor'], version['patch'])


def pretty_version():
    ver = '{major}.{minor}.{patch}'.format(**version)
    if _is_dirty():
        ver += '+' + git_hash(short=True, check_dirty=False) + '*'
    return ver
