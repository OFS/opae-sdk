#!/usr/bin/env python
import httplib
import io
import os
from random import randint
import socket
from tempfile import mkstemp
from threading import Lock
import time
import unittest
from urllib2 import URLError
import urllib2

from util.convert import Convert
from util.mkdir import Mkdir


class FileUtils(object):
    '''
    Provide a layer of indirection for some common file operaions
    '''

    def __init__(self):
        pass

    def get_file(self, filename):

        if not os.path.isfile(filename) and self.isHttpPath(filename=filename):
            url = filename
            prefix = str(filename).rsplit('/', 2)[-1]
            filename = FileUtils.create_tmp_file(prefix=prefix, suffix=".tmp", text=False)
            self.httpScrape(url=url, filename=filename, text=False)

        return filename

    def read_binary_file_open(self, filename):
        filename = self.get_file(filename)
        return io.open(filename, "rb")

    def write_binary_file_open(self, filename):

        if self.isHttpPath(filename=filename):
            raise ValueError('ERROR: Refusing to write to what appears to be an http path')

        file_dir = os.path.dirname(filename)
        Mkdir().mkdir_p(file_dir)
        return io.open(filename, "wb")

    def close_binary_file(self, fp, _filename):
        fp.close()

    def filenames_are_equal(self, filename1, filename2):
        if os.name == 'nt':
            return filename1.lower() == filename2.lower()
        else:
            return filename1 == filename2

    def isHttpPath(self, filename):
        if filename.startswith("http://") or filename.startswith("https://"):
            return True
        else:
            return False

    def httpScrape(self, url, filename, text=True):
        os.environ["https_proxy"] = ""
        os.environ["http_proxy"] = ""

        scrape_tries = 0
        while True:
            error = False
            try:
                #print url
                request = urllib2.Request(url)
                response = urllib2.urlopen(request)

                if text:
                    file_dir = os.path.dirname(filename)
                    Mkdir().mkdir_p(file_dir)
                    fp = open(filename, 'w')
                else:
                    fp = self.write_binary_file_open(filename=filename)

                fp.write(response.read())
                fp.close()

            except (socket.error, httplib.BadStatusLine, urllib2.HTTPError):
                time.sleep(0.4)
                error = True
                if scrape_tries > 5:
                    raise

            if not error:
                break

            scrape_tries += 1


    create_tmp_file_lock = Lock()
    create_tmp_file_count = 0

    @staticmethod
    def create_tmp_file(prefix="tmp_", suffix=".tmp", text=False, directory=None):
        tmp_filename = None
        FileUtils.create_tmp_file_lock.acquire()
        try:
            if suffix is not None:
                suffix = '.' + str(int(time.time())) + "." + str(randint(0,100000)) + "." + str(FileUtils.create_tmp_file_count) + suffix

            FileUtils.create_tmp_file_count += 1

            fd_tmp_file, tmp_filename = mkstemp(prefix=prefix, suffix=suffix, dir=directory, text=text)
            import atexit
            atexit.register(FileUtils.if_exists_delete_file, tmp_filename)
            os.close(fd_tmp_file)
        finally:
            FileUtils.create_tmp_file_lock.release()

        return tmp_filename

    @staticmethod
    def if_exists_delete_file(filename):

        if filename is None: return

        FileUtils.create_tmp_file_lock.acquire()
        try:
            if filename is not None and os.path.isfile(filename):
                os.remove(filename)
        except WindowsError as e:
            print "Could not delete file: " + filename
            print e
        finally:
            FileUtils.create_tmp_file_lock.release()


class FileUtilsTest(unittest.TestCase):

    def test_fopen(self):
        fp = FileUtils().read_binary_file_open(filename="../test/files/example.cmf")
        peek_bytes = fp.peek(4)
        self.assertEquals(hex(Convert().bytearray_to_integer(peek_bytes,0,4)),"0x62294895")
        read_bytes = fp.read(4)
        self.assertEquals(peek_bytes[0:4], read_bytes[0:4])
        fp.close()

    def test_fopen_write(self):
        fp = FileUtils().write_binary_file_open("../work_dir/fileutiltest.bin")
        fp.write("testing...")
        FileUtils().close_binary_file(fp, "../work_dir/fileutiltest.bin")

    def test_isHttpPath(self):
        self.assertTrue(FileUtils().isHttpPath("http://www.google.com"))
        self.assertTrue(FileUtils().isHttpPath("https://www.google.com"))
        self.assertFalse(FileUtils().isHttpPath("Http://www.google.com"))
        self.assertFalse(FileUtils().isHttpPath("../foo.bin"))
        self.assertFalse(FileUtils().isHttpPath("c:/foo/foo.bin"))
        self.assertFalse(FileUtils().isHttpPath("c:\foo\foo.bin"))

    def test_httpScrape(self):

        try:
            FileUtils().httpScrape(url="http://sj-arc.altera.com", filename="../work_dir/arc.txt")
            FileUtils().httpScrape( \
                url="https://sj-arc.altera.com/tools/acds/17.1/current.linux/linux64/quartus/common/devinfo/programmer/firmware/nadder.zip", \
                filename="../work_dir/web_version_nadder.zip", \
                text=False)
        except URLError:
            print "http://sj-arc is not accessible. Skipping test"


if __name__ == '__main__':
    unittest.main()
