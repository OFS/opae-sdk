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
import os
import sys

if sys.version_info[0] == 3:
    str_type = str
else:
    str_type = basestring  # noqa pylint: disable=E0602


def _quote(f, basename=False):
    if isinstance(f, str_type):
        return '"{}"'.format(os.path.basename(f) if basename else f)
    return f


def _print(msg):
    print(msg)


def dry_run_func(fn, **cfg):
    """dry_run_func Inspect a callable and return a function that logs its
                    arguments.

        Args:
          fn: The callable object
          **cfg: Configuration of the dry_run
                 log: The 'log' callback function, defaults to 'print'
                 basename: Convert any arguments that are file paths to
                           just the basename (to shorten out the log).
                           Default is False.
    """
    log = cfg.pop('log', _print)
    basename = cfg.pop('basename', False)

    def _fn(*args, **kwargs):
        args = [_quote(a, basename) for a in args]
        kwargs = dict([(k, _quote(v, basename))
                       for k, v in kwargs.values()])
        items = ['{}'.format(a) for a in args]
        items.extend(['{}={}'.format(k, v)
                      for k, v in kwargs.items()])
        msg = '{}({})'.format(fn.__name__, ', '.join(items))
        log(msg)
    return _fn


def dry_run(obj, enabled=True, **kwargs):
    """dry_run Inspects and returns a function that logs its arguments

        Args:
          obj: object to inspect (should be callable, otherwise same object
               is returned).
          enabled: Flag used as an indication of whether or not to enable
                   'dry_run'. If False, the same object is returned.
          **kwargs: Passed on to implementation methods. For now only
                    callables are supported so the only implementation
                    is 'dry_run_func'.
    """
    if enabled:
        if callable(obj):
            return dry_run_func(obj, **kwargs)
    return obj
