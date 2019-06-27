#!/usr/bin/env python

import datetime
import getpass
import os
import unittest
from zipfile import ZipFile
import zipfile

from networking.server import Server, Client
from util.fileutil import FileUtils


class CommandResponseFS(object):

    password = "password"

    def get_password(self):
        return CommandResponseFS.password

    def get_username(self):
        username = os.popen('whoami').read()
        username = username.rstrip()
        username = username.split('\\')[-1]
        return username

    def __init__(self):
        super(CommandResponseFS,self).__init__()
        self.password = self.get_password()
        self.data = None

    def add(self, name, filename):
        if filename is None:
            self.content[name] = None
        else:
            data = None
            with FileUtils().read_binary_file_open(filename=filename) as fp:
                data = bytearray(fp.read())
            fp.close()
            self.content[name] = data

    def content_exists(self, name):
        if name not in self.content or self.content[name] is None:
            return False
        else:
            return True

    def get(self, name, filename):
        if not self.content_exists(name):
            return None
        else:
            with FileUtils().write_binary_file_open(filename) as fp:
                fp.write(self.content[name])
            fp.close()

    @property
    def data(self):
        zipfilename = None
        zipfp = None
        self._data = []

        try:
            zipfilename = FileUtils.create_tmp_file(suffix=".zip")
            zfile = ZipFile(file=zipfilename, mode="w", compression=zipfile.ZIP_DEFLATED)

            for f in self.content:
                if self.content[f] is None:
                    continue
                filename = None
                try:
                    filename = FileUtils.create_tmp_file()
                    with FileUtils().write_binary_file_open(filename) as fp:
                        fp.write(self.content[f])
                    fp.close()
                    zfile.write(filename=filename, arcname=f)
                finally:
                    FileUtils.if_exists_delete_file(filename)

            zfile.close()
            assert(zipfile.is_zipfile(zipfilename))

            with FileUtils().read_binary_file_open(filename=zipfilename) as zipfp:
                self._data = bytearray(zipfp.read())

            zipfp.close()

        finally:
            if zipfp is not None: zipfp.close()
            FileUtils.if_exists_delete_file(zipfilename)

        return self._data

    @data.setter
    def data(self, value):
        self.content = {}
        self._data = value
        if self._data is None:
            return

        zipfilename = None
        zfile = None

        if value is None:
            return

        try:
            zipfilename = FileUtils.create_tmp_file(suffix=".zip")
            with FileUtils().write_binary_file_open(filename=zipfilename) as zipfp:
                zipfp.write(self._data)
            zipfp.close()

            zfile = ZipFile(file=zipfilename, mode='r', compression=zipfile.ZIP_DEFLATED)

            for f in zfile.namelist():
                if f.startswith("..") or f.startswith('/') or ':' in f:
                    raise ValueError('Invalid Zip: File within zip ' + zipfilename + ' is illegal: ' + f)

                fp = zfile.open(name=f, mode='r')
                self.content[f] = fp.read()
                fp.close()
            zfile.close()
        finally:
            if zfile is not None: zfile.close()
            FileUtils.if_exists_delete_file(zipfilename)

    @property
    def command(self):
        if 'cmd' in self.content:
            return self.content['cmd']
        else:
            return None

    @command.setter
    def command(self, value):
        self.content['cmd'] = value

    @property
    def stdout(self):
        if 'stdout' in self.content and self.content['stdout'] is not None:
            return self.content['stdout']
        else:
            return ""

    @stdout.setter
    def stdout(self, value):
        self.content['stdout'] = value

class CommandResponseServer(Server):

    def __init__(self):
        super(CommandResponseServer,self).__init__()
        self.callback = self.execute

        self.cmd_callback = {}
        self.cmd_callback['ping'] = self.ping
        self.cmd_callback['exit'] = self.exit

    def ping(self, cmdfs):
        assert(cmdfs.command == 'ping')
        cmdfs.content['stdout'] = 'Reply from Server Received'
        return cmdfs

    def exit(self, cmdfs):
        assert(cmdfs.command == 'exit')
        self.keep_listening = False
        cmdfs.content['stdout'] = 'Server is Shutting Down Now!'
        return cmdfs

    def msg_init(self, cmdfs):
        self.log('----------------------------------')
        self.log('Command: ' + cmdfs.command)

    def msg_fini(self, respfs):
        self.log('----------------------------------')

    def execute(self, clientsocket):
        try:
            data = self.recv(clientsocket)
            cmd_resp_fs = CommandResponseFS()
            cmd_resp_fs.data = data

            if cmd_resp_fs.command in self.cmd_callback:
                self.msg_init(cmd_resp_fs)
                cmd_resp_fs = self.cmd_callback[cmd_resp_fs.command](cmd_resp_fs)
                self.msg_fini(cmd_resp_fs)
            else:
                cmd_resp_fs.content['stdout'] = 'ERROR: Invalid Command - ' + cmd_resp_fs.command

            self.send(clientsocket,cmd_resp_fs.data)
        finally:
            clientsocket.close()

class CommandResponseClient(Client):

    def __init__(self):
        super(CommandResponseClient,self).__init__()

    def execute(self, cmd_fs, silent=False):
        self.connect()
        if not self.connected:
            raise ValueError('Could not connect to any sign server')

        self.send(cmd_fs.data)
        resp_fs = CommandResponseFS()
        resp_fs.data = self.recv()
        self.disconnect()
        assert(resp_fs.command == cmd_fs.command)
        if not silent: print resp_fs.stdout
        return resp_fs

    def ping(self, silent=False):
        cmd_fs = CommandResponseFS()
        cmd_fs.command = 'ping'
        ti = datetime.datetime.now()
        _resp_fs = self.execute(cmd_fs, silent=silent)
        tf = datetime.datetime.now()
        td = tf - ti
        if not silent:
            print str(td.total_seconds() * 1000) + "ms"

    def exit(self):
        cmd_fs = CommandResponseFS()
        cmd_fs.command = 'exit'
        self.execute(cmd_fs)
        try:
            self.ping(silent=True)
        except:
            pass

class CommandResponseClientServerTest(unittest.TestCase):

    def test_ping(self):

        server = CommandResponseServer()
        server.debug = True

        client = CommandResponseClient()
        client.debug = True
        client.connect_timeout = 0.1

        listen_thr = server.listen()
        client.add_server(hostname='localhost', port=server.port)

        server.keep_listening = False
        client.ping()

        listen_thr.join(timeout=5)

    def test_exit(self):
        server = CommandResponseServer()
        client = CommandResponseClient()
        listen_thr = server.listen()
        client.add_server(hostname='localhost', port=server.port)
        for _i in range(0,5):
            client.ping()
        client.exit()
        listen_thr.join()

class CommandResponseFSTest(unittest.TestCase):

    def test_very_simple(self):
        fs = CommandResponseFS()
        self.assertNotEqual(fs, None)
        self.assertGreater(len(fs.data), 10)
        size_pre = len(fs.data)
        fs.content['subdir/cmd'] = "this_is_test"
        size_post = len(fs.data)
        self.assertGreater(size_post, size_pre)

        fs_copy = CommandResponseFS()
        fs_copy.data = fs.data
        self.assertEqual(fs_copy.content['subdir/cmd'],"this_is_test")
        self.assertEqual(fs_copy.data,fs.data)

    def test_command(self):
        fs = CommandResponseFS()
        self.assertEqual(fs.command,None)
        fs.command = "foo"
        self.assertEqual(fs.command,"foo")
        self.assertEqual(fs.content['cmd'],"foo")

    def test_get_username(self):
        fs = CommandResponseFS()
        self.assertEqual(fs.get_username(),getpass.getuser())
