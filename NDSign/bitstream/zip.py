#!/usr/bin/env python

import os
from tempfile import mkstemp
import unittest
from zipfile import ZipFile
import zipfile

from bitstream import IBitstream
from util.fileutil import FileUtils
from util.mkdir import Mkdir


class BitstreamZip(IBitstream):

    def __init__(self):
        self.filename = None
        self.fragments = {}

    def initialize(self):
        return self

    def size(self):
        size = 0
        for frag in self.fragments:
            size += self.fragments[frag].size()
        return size

    def append(self, filename, fragment):
        self.fragments[filename] = fragment

    def validate(self):
        for frag in self.fragments:
            self.fragments[frag].validate()
        return True

    def update(self):
        for frag in self.fragments:
            self.fragments[frag].update()

    def get_value(self, offset, size=4, endianness='little'):
        raise NotImplementedError('method not implemented')

    def load(self, filename=None):
        if filename is not None:
            self.filename = filename

        assert(self.filename is not None)
        assert(zipfile.is_zipfile(self.filename))

        sha256_chk = {}

        z = ZipFile(file=self.filename, mode='r', compression=zipfile.ZIP_STORED)
        for name in z.namelist():
            if name.startswith("..") or name.startswith('/') or ':' in name:
                raise ValueError('Invalid Zip: File within zip ' + self.filename + ' is illegal: ' + name)

            if name.endswith('_sha256.chk'):
                chk_fp = z.open(name=name, mode='r')
                for chk_line in chk_fp.readlines():
                    filehash,filename =  chk_line.rstrip('\n').split('  ')
                    sha256_chk[filename] = filehash

                chk_fp.close()
                continue

            if name.endswith('.chk'):
                raise ValueError('Invalid Zip: Chk file within zip ' + self.filename + ' is unexpected: ' + name)

            fp = z.open(name=name, mode='r')
            from factory import BitstreamFactory
            frag = BitstreamFactory().generate(stream=fp, filename=name)
            self.fragments[name] = frag
            fp.close()
        z.close()

        if len(self.fragments) != len(sha256_chk):
            raise ValueError('Invalid Zip: number of file entries in *.chk file does not equal number of files in zip file')

        for f in sha256_chk:
            if f not in self.fragments:
                raise ValueError('Invalid Zip: File found in check that is missing from zip: ' + f)
            if int(sha256_chk[f],16) != int(self.fragments[f].sha256sum(),16):
                raise ValueError('Invalid Zip: Hash Mismatch for file "' + f + '".' + \
                                 " Expected: " + sha256_chk[f] + \
                                 " Actual: " + self.fragments[f].sha256sum())

        self.validate()
        return self

    def save(self, filename=None):
        if filename is not None:
            self.filename = filename

        assert(self.filename is not None)
        self.validate()

        file_dir = os.path.dirname(self.filename)
        Mkdir().mkdir_p(file_dir)

        with ZipFile(file=self.filename, mode='w', compression=zipfile.ZIP_STORED) as z:
            for frag in self.fragments:
                try:
                    temp_filename_prefix = "ndsign_" + os.path.basename(frag).replace('.','_')
                    FileUtils.create_tmp_file_lock.acquire()
                    try:
                        temp_fragment_fp, temp_fragment_filename = mkstemp(prefix=temp_filename_prefix, suffix=".bin", dir=None, text=False)
                    finally:
                        FileUtils.create_tmp_file_lock.release()
                    self.fragments[frag].save(filename=temp_fragment_filename)
                    os.close(temp_fragment_fp)
                    z.write(filename=temp_fragment_filename, arcname=frag)
                finally:
                    FileUtils().if_exists_delete_file(temp_fragment_filename)

            sha256_chk_filename = os.path.basename(self.filename).replace('.','_') + '_sha256.chk'
            z.writestr(sha256_chk_filename,bytes=self.sha256sumAll())

        z.close()

    def get_raw_byte_array(self):
        data = None
        filename = None
        try:
            filename = FileUtils().create_tmp_file(prefix="tmp_", suffix=".zip")
            self.save(filename)
            with FileUtils().read_binary_file_open(filename=filename) as fp:
                data = bytearray(fp.read())
            fp.close()
        finally:
            FileUtils().if_exists_delete_file(filename)

        return data

    def sha256sumAll(self):
        return_string = ""
        for frag in self.fragments:
            return_string += self.fragments[frag].sha256sum()
            return_string += '  '
            return_string += frag
            return_string += '\n'
        return return_string

    def __str__(self):
        return_string = " " + self.__class__.__name__ + " [Size=" + str(self.size()) + "] " + "\n"
        for frag in self.fragments:
            return_string += "   " + frag + ": " + self.fragments[frag].__class__.__name__ + \
                " [Size=" + str(self.fragments[frag].size()) + "]" + \
                " [SHA256=" + self.fragments[frag].sha256sum() + "]\n"
        return return_string

class BitstreamZipTest(unittest.TestCase):

    def test_constructor(self):
        bz = BitstreamZip()
        bz.initialize()
        bz.validate()

    def test_load_zip(self):
        bz = BitstreamZip().load("../test/files/example.zip")
        bz.validate()
        self.assertGreater(bz.size(),10000)

    def test_save_zip(self):
        bz = BitstreamZip().load("../test/files/example.zip")
        bz_size_orig = bz.size()
        from cmf import Cmf
        bz.append(filename="bar/empty.cmf", fragment=Cmf().initialize())
        bz.save(filename="../work_dir/example_modifiled.zip")
        bz.load()
        bz.validate()
        self.assertGreater(bz.size(),bz_size_orig)


