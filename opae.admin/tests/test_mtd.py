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
        """test_open
           Given a valid mtd device to be open
           When trying to open an invalid mtd device 
           It returns an IOError as expected
        """
        with mtd('/dev/mtd0').open('rb') as dev:
            self.assertIsNotNone(dev)
        with self.assertRaises(IOError):
            with mtd('/dev/mtd1').open('rb') as dev:
                pass

    @mock.patch(open_string, new=mock_open('/dev/mtd0'))
    def test_size(self):
        """test_size
           Given a valid registered ioctl and mtd open of the device
           The size of the device is the same as what was registered
        """
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
        """test_erase
           Given a valid registered ioctl and mtd open of the device
           When trying to open and erase some bytes from the mtd device
           It fails to erase and raises KeyError.
        """
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
        """test_copy_from
           Given two files with different buffer sizes
           Try copying bytes from open file to the valid mtd device
           It copies successfully with the same buffer size
           But fails to read bytes and exit with ValueError
        """
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

    def test_copy_from_closed_fd(self):
        """test_copy_from_closed_fd
           Given a valid mtd open and close of mtd device
           When trying to access a closed mtd device to copy from
           It returns IOError as expected
        """
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                pass
            with self.assertRaises(IOError):
                dev.copy_from(self.random_io, self.TEST_SIZE, chunked=32,
                            progress=sys.stdout)

    def test_copy_to(self):
        """test_copy_to
           Given two files with different buffer sizes
           Try copying bytes from open mtd device to a valid open file
           It copies successfully with the same buffer size
           But fails to read bytes and exit with ValueError
        """

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

    def test_copy_to_closed_fd(self):
        """test_copy_to_closed_fd
           Given a valid mtd open and close of mtd device
           When trying to access a closed device fd to copy to
           It returns IOError as expected
        """
        with mock.patch(open_string, return_value=self.random_io):
            with mtd('/dev/mtd0').open('wb') as dev:
                pass
            with self.assertRaises(IOError):
                dev.copy_to(self.empty_io, self.TEST_SIZE, chunked=32,
                            progress=sys.stdout)

    def test_dump(self):
        """test_dump
           Given a valid mtd open and creation of tempfile
           It reads the mtd device and dumps content to tempfile under /tmp/XXX
        """
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                with tempfile.NamedTemporaryFile() as tmp_file:
                    dev.dump(tmp_file.name)
                    pass

    def test_dump_no_file(self):
        """test_dump_no_file
           Given a valid mtd open and try to dump content to unknown file
           It fails to locate file and raises OSError as expected
        """
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                with self.assertRaises(OSError):
                    dev.dump("invalid_file")
                    pass

    def test_load(self):
        """test_load
           Given a valid mtd open and try to load empty content to the mtd device
           It raises ValueEroor as expected
        """
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                with tempfile.NamedTemporaryFile() as tmp_file:
                    with self.assertRaises(ValueError):
                        dev.load(tmp_file.name)
                        pass

    def test_load_no_file(self):
        """test_load_no_file
           Given a valid mtd open and try to load unknown file to mtd device
           It fails to locate file and raises OSError as expected
        """
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                with self.assertRaises(OSError):
                    dev.load("invalid_file")
                    pass

    def test_fileno(self):
        """test_fileno
           Given a valid mtd open and try to retrieve the fd
           It returns a valid (non -1) fd as expected
        """
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0').open('rb') as dev:
                fno = dev.fileno()
                self.assertNotEqual(fno, -1)

    def test_fileno_closed(self):
        """test_fileno_closed
           Given a valid mtd open and close of the mtd device
           When trying to access a closed device fd
           It returns -1 as expected
        """
        with mock.patch(open_string, return_value=self.empty_io):
            with mtd('/dev/mtd0') as dev:
                pass
            fno = dev.fileno()
            self.assertEqual(fno, -1)
