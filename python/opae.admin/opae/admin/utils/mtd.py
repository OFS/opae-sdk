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
import os
import shutil
import struct
import sys

from opae.admin.utils.log import loggable
from opae.admin.utils.progress import progress

if sys.version_info[0] == 3:
    from io import IOBase as _ftype
else:
    _ftype = file  # noqa (Python 3 will report this as an error)


class mtd(loggable):
    """mtd encapsulates an mtd (flash) device."""
    IOCTL_MTD_MEMGETINFO = 0x80204d01
    IOCTL_MTD_MEMERASE = 0x40084d02
    IOCTL_MTD_MEMUNLOCK = 0x40084d06

    def __init__(self, devpath):
        """__init__ Initalizes a new mtd object and holds a reference
                    to the device path which is used in most member methods.

           Args:
               devpath: Path to mtd device (example: '/dev/mtd0')
        """
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

        Args:
            mode: Mode to open device. Defaults to 'rb'.

        Raises:
            IOError: If call to lock failed.

        Note: If the internal file object is currently open, it will log a
              warning and close the current file object.
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
        """fileno Get the file descriptor of the open mtd device.

        Returns: The file descriptor of open device.
                 -1 if the device currently isn't open.
        """
        if self._fp is not None:
            return self._fp.fileno()
        self.log.warn('mtd device (%s) is not open', self._devpath)
        return -1

    @property
    def size(self):
        """size Get the size of the this mtd device by calling the appropriate
                ioctl request.

        Returns:
            The size of the mtd device as reported by the driver.

        Raises:
            IOError: If the ioctl raises an IOError.

        Note:
            This will only call the ioctl request (IOCTL_MTD_MEMGETINFO) once
            and cache the size in an internal member variable.
            The cached value will be returned subsequent times. This assumes
            that the device represented by the devpath hasn't been hot-removed.

        """
        # TODO (rrojo): Determine if it is safer to not cache the size
        #               as there is no guarantee that the same devpath will
        #               be used if device is hot-removed/hot-plugged.
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

        Args:
            start: Offset to start erasing at.
            nbytes: Number of bytes to erase.

        Notes:
            This calls the ioctl request (IOCTL_MTD_MEMERASE).
        """
        self.log.debug('erasing %d bytes starting at 0x%08x', nbytes, start)
        iodata = struct.pack('II', start, nbytes)
        with open(self._devpath, 'wb') as fp:
            fcntl.ioctl(fp.fileno(), self.IOCTL_MTD_MEMERASE, iodata)

    def read(self, size=-1):
        """read Read <size> number of bytes from an open mtd device.

        Args:
            size: Number of bytes to read from device.
                     If size is negative one or omitted, read until EOF.
        """
        return self._fp.read(size)

    def write(self, data):
        """write Write a buffer to an open mtd device.

        Args:
            data: The data to write to the device.
        """
        self._fp.write(data)

    def copy_to(self, dest_fp, nbytes, start=0, **kwargs):
        """copy_to Copy given number of bytes from this mtd device
                   to an open file object.

        Args:
            fp: File object to copy to.
            nbytes: Number of bytes to copy from mtd device to open file.
            start: Offset to start copying from.
            **kwargs: configuration of copy operation

        Raises:
            IOError: if the device is not open or seek operation failed.
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

        self._copy_chunked(self._fp, dest_fp, nbytes,
                           kwargs.get('chunked', nbytes),
                           kwargs.get('progress'))

    def copy_from(self, src_fp, nbytes, start=0, **kwargs):
        """copy_from Copy given number of bytes from an open file to this
                     mtd device.

        Args:
            src_fp: File object to copy from.
            nbytes: Number of bytes to copy.
            start: Offset on mtd device to start copying to. Defaults to 0.

        Raises:
            IOError: if the device is not open or seek operation failed.
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

        self._copy_chunked(src_fp, self._fp, nbytes,
                           kwargs.get('chunked', nbytes),
                           kwargs.get('progress'))

    def _copy_chunked(self, src, dest, size, chunk_size, prg_writer=None):
        offset = 0

        cfg = {'bytes': size}
        if callable(prg_writer):
            cfg['log'] = prg_writer
        elif isinstance(prg_writer, _ftype):
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

    def dump(self, filename, offset=0):
        """dump Dump the contents of the mtd device to a file.

           Args:
               filename: Filename to write the contents to.
               offset: Offset to start transfer from mtd device.
                       Default is 0.

           Note:
               This opens the mtd device as another file object independent of
               its internal file object.

        """
        if not os.path.exists(os.path.basename(filename)):
            msg = 'Directory path does not exist for {}'.format(filename)
            self.log.exception(msg)
            raise OSError(msg)

        self.log.debug('dumping mtd data from %s to %s',
                       self._devpath, filename)
        with open(self._devpath, 'rb') as src:
            src.seek(offset)
            with open(filename, 'wb') as dst:
                shutil.copyfileobj(src, dst)
        self.log.debug('transfer complete')

    def load(self, filename, offset=0):
        """load Load the contents of a file into the mtd device.

        Args:
            filename: Filename to read the contents from.
            offset: Offset to start transfer to mtd device.
                    Default is 0.
        Raises:
            OSError: If the source file does not exist.
            IOError: If the flock call fails.

        Note:
            This opens the mtd device as another file object and tries to
            lock it as exclusive mode.

        """
        if not os.path.exists(filename):
            msg = 'Input file does not exist: {}'.format(filename)
            self.log.exception(msg)
            raise OSError(msg)

        self.log.debug('writing mtd data from %s to %s',
                       filename, self._devpath)
        with open(self._devpath, 'wb') as dst:
            try:
                fcntl.flock(dst.fileno(), fcntl.LOCK_EX)
            except IOError as err:
                self.log.exception('failed to lock device for writing: %s',
                                   err)
                raise
            dst.seek(offset)
            with open(filename, 'rb') as src:
                shutil.copyfileobj(src, dst)
            try:
                fcntl.flock(dst.fileno(), fcntl.LOCK_UN)
            except IOError as err:
                self.log.exception('failed to unlock device after writing: %s',
                                   err)
                raise
        self.log.debug('transfer complete')
