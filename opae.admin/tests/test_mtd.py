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
import mock
import struct
import sys
import tempfile
import unittest
from opae_mocks import open_string, mock_open, mock_ioctl
from opae.admin.utils.mtd import mtd


def get_buffer(bio):
    pos = bio.tell()
    bio.seek(0)
    value = bio.read()
    bio.seek(pos)
    return value


class test_mtd(unittest.TestCase):
    TEST_SIZE = 4096*100

    def setUp(self):
        self.ioctl_handler = mock_ioctl()
        self.empty_io = tempfile.NamedTemporaryFile(prefix='empty_mock_')
        self.empty_io.seek(0)
        self.random_io = tempfile.NamedTemporaryFile(prefix='random_mock_')
        self.random_io.seek(0)
        with open('/dev/urandom', 'rb') as random:
            self.random_io.write(random.read(self.TEST_SIZE))

    def tearDown(self):
        self.empty_io.close()
        self.random_io.close()

    @mock.patch(open_string, new=mock_open('/dev/mtd0'))
    def test_open(self):
        with mtd('/dev/mtd0').open('rb') as dev:
            self.assertIsNotNone(dev)
        with self.assertRaises(IOError):
            with mtd('/dev/mtd1').open('rb') as dev:
                pass

    @mock.patch(open_string, new=mock_open('/dev/mtd0'))
    def test_size(self):
        dummy_size = 0x101011

        def mtd_info(fd, req, buf, *args):
            return struct.pack('BIQIIQ', 0, 0, 0x101011, 0, 0, 0)

        with self.ioctl_handler.register(mtd.IOCTL_MTD_MEMGETINFO,
                                         mtd_info) as ioctl:
            with mock.patch('fcntl.ioctl', new=ioctl):
                dev = mtd('/dev/mtd0')
                dev.open('rb')
                self.assertEquals(dev.size, dummy_size)

    @mock.patch(open_string, new=mock_open('/dev/mtd0'))
    def test_erase(self):

        def mtd_info(fd, req, buf, *args):
            return struct.pack('BIQIIQ', 0, 0, 0x101011, 0, 0, 0)

        with self.ioctl_handler.register(mtd.IOCTL_MTD_MEMGETINFO,
                                         mtd_info) as ioctl:
            with mock.patch('fcntl.ioctl', new=ioctl):
                with self.assertRaises(KeyError):
                    dev = mtd('/dev/mtd0')
                    dev.open('wb')
                    dev.erase(0, self.TEST_SIZE) 

    def test_copy_from(self):
        self.random_io.seek(0)
        self.assertNotEqual(get_buffer(self.empty_io),
                            get_buffer(self.random_io))
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                dev.copy_from(self.random_io, self.TEST_SIZE, chunked=32,
                              progress=sys.stdout)
                self.assertEqual(get_buffer(self.empty_io),
                                 get_buffer(self.random_io))
            with self.assertRaises(ValueError):
                self.empty_io.read(1)

    def test_copy_from_opened(self):
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                pass
            with self.assertRaises(IOError):
                dev.copy_from(self.random_io, self.TEST_SIZE, chunked=32,
                            progress=sys.stdout)

    def test_copy_to(self):
        self.assertNotEqual(get_buffer(self.empty_io),
                            get_buffer(self.random_io))
        with mock.patch(open_string, return_value=self.random_io):
            with mtd('/dev/mtd0').open('wb') as dev:
                dev.copy_to(self.empty_io, self.TEST_SIZE, chunked=32,
                            progress=sys.stdout)
                self.assertEqual(get_buffer(self.random_io),
                                 get_buffer(self.empty_io))
            with self.assertRaises(ValueError):
                self.random_io.read(1)

    def test_copy_to_opened(self):
        with mock.patch(open_string, return_value=self.random_io):
            with mtd('/dev/mtd0').open('wb') as dev:
                pass
            with self.assertRaises(IOError):
                dev.copy_to(self.empty_io, self.TEST_SIZE, chunked=32,
                            progress=sys.stdout)

    def test_dump(self):
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                with tempfile.NamedTemporaryFile() as tmp_file:
                    dev.dump(tmp_file.name)
                    pass

    def test_dump_no_file(self):
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                with self.assertRaises(OSError):
                    dev.dump("invalid_file")
                    pass

    def test_load(self):
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                with tempfile.NamedTemporaryFile() as tmp_file:
                    with self.assertRaises(ValueError):
                        dev.load(tmp_file.name)
                        pass

    def test_load_no_file(self):
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                with self.assertRaises(OSError):
                    dev.load("invalid_file")
                    pass

    def test_fileno(self):
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                fno = dev.fileno()
                self.assertNotEqual(fno, -1)

    def test_fileno_opened(self):
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0') as dev:
                pass
            fno = dev.fileno()
            self.assertEqual(fno, -1)
