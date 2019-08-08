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
import fcntl
import shutil
import struct

from opae.admin.utils.log import loggable
from opae.admin.utils.progress import progress


class mtd(loggable):
    """mtd encapsulates an mtd (flash) device"""
    IOCTL_MTD_MEMGETINFO = 0x80204d01
    IOCTL_MTD_MEMERASE = 0x40084d02
    IOCTL_MTD_MEMUNLOCK = 0x40084d06

    def __init__(self, devpath):
        super(mtd, self).__init__()
        self._devpath = devpath
        self._fp = None
        self._size = None

    def __enter__(self):
        if self._fp is None:
            self.open()
        return self

    def __exit__(self, ex_type, ex_val, ex_traceback):
        self.close()

    def open(self, mode='rb'):
        """open Open an mtd device.

        :param mode: Mode to open device. Defaults to 'rb'
        """
        if self._fp is not None:
            self.log.warn('device is currently open, closing')
            self._fp.close()
        self._fp = open(self._devpath, mode)
        return self

    def close(self):
        """close Close an open mtd device."""
        if self._fp is not None:
            self._fp.close()
        self._fp = None

    def fileno(self):
        """fileno Get the file descriptor of the open mtd device."""
        if self._fp is not None:
            return self._fp.fileno()
        self.log.warn('mtd device (%s) is not open', self._devpath)
        return -1

    @property
    def size(self):
        """size Get the size of the this mtd device"""

        if self._size is None:
            iodata = struct.pack('BIIIIIQ', 0, 0, 0, 0, 0, 0, 0)
            with open(self._devpath, 'rb') as fp:
                try:
                    res = fcntl.ioctl(fp.fileno(),
                                      self.IOCTL_MTD_MEMGETINFO, iodata)
                except IOError as err:
                    self.log.exception('error getting size: %s', err)
                    raise
                else:
                    self._size = struct.unpack_from('BIIIIIQ', res)[2]
        return self._size

    def erase(self, start, nbytes):
        """erase Erase parts of the mtd device.

        :param start: Offset to start erasing at.
        :param nbytes: Number of bytes to erase.
        """
        self.log.debug('erasing %d bytes starting at 0x%08x', nbytes, start)
        iodata = struct.pack('II', start, nbytes)
        with open(self._devpath, 'wb') as fp:
            fcntl.ioctl(fp.fileno(), self.IOCTL_MTD_MEMERASE, iodata)

    def read(self, size=-1):
        """read Read <size> number of bytes from an open mtd device.

        :param size: Number of bytes to read from device.
                     If size is negative one or omitted, read until EOF.
        """
        return self._fp.read(size)

    def write(self, data):
        """write Write a buffer to an open mtd device.

        :param data: The data to write to the device.
        """
        self._fp.write(data)

    def copy_to(self, dest_fp, nbytes, start=0, **kwargs):
        """copy_to Copy given number of bytes from this mtd device
                   to an open file object.

        :param fp: File object to copy to.
        :param nbytes: Number of bytes to copy from mtd device.
        :param start: Offset to start copying from.
        :param **kwargs: configuration of copy operation
        """
        if self._fp is None:
            self.log.exception('mtd device is not open: %s', self._devpath)
            raise IOError('mtd device is not open')

        self._fp.seek(start)
        if self._fp.tell() != start:
            msg = 'could not seek to {} in mtd device:{}'.format(start,
                                                                 self._fp.name)
            self.log.exception(msg)
            raise IOError(msg)
        self.log.debug('copying %d bytes from mtd device: %s', nbytes,
                       self._fp.name)

        chunked = kwargs.get('chunked')

        if chunked:
            self._copy_chunked(self._fp, dest_fp,
                               nbytes, chunked, kwargs.get('progress'))
        else:
            shutil.copyfileobj(self._fp, dest_fp, nbytes)

    def copy_from(self, src_fp, nbytes, start=0, **kwargs):
        """copy_from Copy given number of bytes from an file object to this
                     mtd device.

        :param src_fp: File object to copy from.
        :param nbytes: Number of bytes to copy.
        :param start: Offset to start copying to. Defaults to 0.
        """
        if self._fp is None:
            self.log.exception('mtd device is not open: %s', self._devpath)
            raise IOError('mtd device is not open')

        self._fp.seek(start)
        if self._fp.tell() != start:
            msg = 'could not seek to {} in mtd device:{}'.format(start,
                                                                 self._fp.name)
            self.log.exception(msg)
            raise IOError(msg)

        chunked = kwargs.get('chunked')

        if chunked:
            self._copy_chunked(src_fp, self._fp,
                               nbytes, chunked, kwargs.get('progress'))
        else:
            shutil.copyfileobj(src_fp, self._fp, nbytes)

    def _copy_chunked(self, src, dest, size, chunk_size, prg_writer=None):
        offset = 0

        cfg = {'bytes':size}
        if callable(prg_writer):
            cfg['log'] = prg_writer
        elif type(prg_writer) is file:
            cfg['stream'] = prg_writer
        else:
            cfg['null'] = True

        prg = progress(**cfg)

        with prg:
            while offset < size:
                n_bytes = min(chunk_size, size-offset)
                chunk = src.read(n_bytes)
                dest.write(chunk)
                n_read = len(chunk)
                if n_read < n_bytes:
                    self.log.warn('read %d less bytes than requested',
                                  n_bytes - n_read)
                offset += n_read
                prg.update(offset)
