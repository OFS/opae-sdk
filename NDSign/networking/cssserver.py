#!/usr/bin/env python

import io
import os
from threading import Lock
import unittest

from bitstream.cmf import Cmf
from bitstream.engineering_cert import EngineeringCert
from bitstream.factory import BitstreamFactory
from networking.cmdrspserver import CommandResponseServer, CommandResponseClient, \
    CommandResponseFS
from sign.css import Css
from util.fileutil import FileUtils


class CssServer(CommandResponseServer, Css):

    SIGN_CMF_COMMAND = 'sign_cmf'
    SIGN_EPH_COMMAND = 'sign_eph'
    SIGN_CERT_COMMAND = 'sign_cert'

    ENV_VARS_TO_TRANSFER = ['HOSTNAME', 'PWD', 'ARC_JOB_ID', 'ARC_OS', 'ARC_BROWSE_HOST', 'ARC_SITE']

    css_server_sign_cmf = Lock()

    def __init__(self):
        super(CssServer,self).__init__()
        self.cmd_callback[CssServer.SIGN_CMF_COMMAND] = self.sign_cmf
        self.cmd_callback[CssServer.SIGN_EPH_COMMAND] = self.sign_eph
        self.cmd_callback[CssServer.SIGN_CERT_COMMAND] = self.sign_cert
        self.callback_timeout = 300.0

        self.authorized_users = []

        self.authorized_users_file = None

        # add current username as authorized user
        self.add_user(CommandResponseFS().get_username())

    def is_user_authorized(self, username):
        if username in self.authorized_users:
            return True
        else:
            self.add_users_from_file()
            return username in self.authorized_users

    def add_user(self, username):
        if username not in self.authorized_users:
            self.authorized_users.append(username)

    def add_list_of_users(self, user_list_str):
        username_list = user_list_str.replace(',',' ').split()

        for username in username_list:
            self.add_user(username)

    def add_users_from_file(self):
        if self.authorized_users_file is None or not os.path.isfile(self.authorized_users_file):
            return

        fp = io.open(self.authorized_users_file, 'r')
        user_list_str = fp.read()
        fp.close()

        self.add_list_of_users(user_list_str)

    def msg_init(self, cmdfs):
        super(CssServer,self).msg_init(cmdfs)
        for envvar in ['user', 'version'] + CssServer.ENV_VARS_TO_TRANSFER:
            if envvar in cmdfs.content:
                self.log(envvar + ': ' + cmdfs.content[envvar])

    def sign_eph(self, cmdfs):
        return self._sign_common(cmdfs, 'files.txt', 'files.txt', Css.sign_eph)

    def sign_cmf(self, cmdfs):
        return self._sign_common(cmdfs, 'cmf', 'signed_cmf', Css.sign_cmf)

    def sign_cert(self, cmdfs):
        return self._sign_common(cmdfs, 'cert', 'signed_cert', Css.sign_cert)

    def _sign_common(self, cmdfs, input_field, output_field, sign_func):

        respfs = CommandResponseFS()
        respfs.content['stdout'] = ""
        respfs.command = cmdfs.command
        assert(cmdfs.command == CssServer.SIGN_CMF_COMMAND or cmdfs.command == CssServer.SIGN_CERT_COMMAND or cmdfs.command == CssServer.SIGN_EPH_COMMAND)
        tmp_files_to_delete = []

        # make a copy of this css and use that as your starting point
        css = Css()
        css.copy_settings(self)

        # TODO: Get rid of this overly conservative lock!
        # this lock should not be needed, but it is needed for some unknown reason :(
        CssServer.css_server_sign_cmf.acquire()

        try:
            if cmdfs.content_exists('sign_keychain_file'):
                css.sign_keychain_file = FileUtils.create_tmp_file(prefix="sign_keychain_file")
                cmdfs.get('sign_keychain_file', css.sign_keychain_file)
                tmp_files_to_delete.append(css.sign_keychain_file)
            if cmdfs.content_exists('sign_keypair_file'):
                css.sign_keypair_file = FileUtils.create_tmp_file(prefix="sign_keypair_file")
                cmdfs.get('sign_keypair_file', css.sign_keypair_file)
                tmp_files_to_delete.append(css.sign_keypair_file)

            if cmdfs.content_exists('use_code_sign_server'):
                request_use_code_sign_server = (cmdfs.content['use_code_sign_server'] == "True")
                if request_use_code_sign_server and not css.use_code_sign_server:
                    raise ValueError('Attempting to use css sign with ltsign-only signing server')
                css.use_code_sign_server = request_use_code_sign_server

            if cmdfs.content_exists('css_project'):
                if css.css_project is not None and css.css_project != cmdfs.content['css_project']:
                    raise ValueError('css-project is already hardcoded to: ' + css.css_project + '. This value cannot be changed.')
                css.css_project = cmdfs.content['css_project']

            if cmdfs.content_exists('css_secure_token_cert_keyhash'):
                if css.css_secure_token_cert_keyhash is not None \
                    and css.css_secure_token_cert_keyhash != cmdfs.content['css_secure_token_cert_keyhash']:
                    raise ValueError('css-keyhash is already hardcoded to: ' + css.css_secure_token_cert_keyhash + '. This value cannot be changed.')
                css.css_secure_token_cert_keyhash = cmdfs.content['css_secure_token_cert_keyhash']

            assert(cmdfs.content_exists(input_field))

            if not cmdfs.content_exists('version') or float(cmdfs.content['version']) < 1.9:
                raise ValueError("ndsign client version is too old - version 1.9 or greater is required to use this sign server")

            assert(cmdfs.content_exists('user'))

            if not self.is_user_authorized(cmdfs.content['user']):
                raise ValueError("User '" + cmdfs.content['user'] +"' is not authorized to use this signing server")

            if 'ARC_JOB_ID' in cmdfs.content:
                css.uid = cmdfs.content['ARC_JOB_ID']

            filedata = {}
            if input_field == 'files.txt':
                #get all the files in the 'files' directory
                files = cmdfs.content['files.txt']
                for filename in files.split(','):
                    if filename == '':
                        continue
                    if not cmdfs.content_exists('files/' + filename):
                        print "Could not read '" + filename + "'"
                        continue
                    filedata[filename] = cmdfs.content['files/' + filename]
                data = filedata
            else:
                data_ba = cmdfs.content[input_field]
                data = BitstreamFactory().generate_from_bytearray(data_ba)

            signed_data = sign_func(css, data)

            filenames = ""
            if(output_field == 'files.txt'):
                for filename in signed_data:
                    respfs.content[filename] = signed_data[filename].get_raw_byte_array()
                    filenames += filename + ','
                respfs.content[output_field] = filenames[:-1]
            else:
                respfs.content[output_field] = signed_data.get_raw_byte_array()
            respfs.content['stdout'] += 'Success!'
            respfs.content['return_code'] = "0"

        except ValueError as err:
            print err
            respfs.content['stdout'] += 'Signing Operation Failed!\n'
            respfs.content['stdout'] += str(err)
            respfs.content['return_code'] = "1"
        finally:
            CssServer.css_server_sign_cmf.release()

            for temp_file in tmp_files_to_delete:
                FileUtils.if_exists_delete_file(temp_file)


        return respfs


class CssClient(CommandResponseClient, Css):

    def __init__(self):
        super(CssClient,self).__init__()

    def version(self):
        from ndsign import NadderSign
        return NadderSign.Version


    def update_commandresponse(self, cmd_fs):
        cmd_fs.content['user'] = cmd_fs.get_username()
        cmd_fs.content['version'] = self.version()

        for envvar in CssServer.ENV_VARS_TO_TRANSFER:
            cmd_fs.content[envvar] = os.getenv(envvar)

        if not self.use_code_sign_server:
            assert(os.path.exists(self.get_keypair_file()))
            if self.sign_keypair_file is None:
                self.sign_keypair_file = self.get_keypair_file()

        cmd_fs.add('sign_keychain_file',self.sign_keychain_file)
        cmd_fs.add('sign_keypair_file',self.sign_keypair_file)

        if self.use_code_sign_server:
            cmd_fs.content['use_code_sign_server'] = 'True'
        else:
            cmd_fs.content['use_code_sign_server'] = 'False'
        cmd_fs.content['css_project'] = self.css_project
        cmd_fs.content['css_secure_token_cert_keyhash'] = self.css_secure_token_cert_keyhash

    def get_response(self, resp_fs, output_field):
        if resp_fs.content_exists('return_code') and int(resp_fs.content['return_code']) != 0:
            raise ValueError('Signing Failed')

        if not resp_fs.content_exists(output_field):
            raise ValueError('Signing Failed - Signed data was not returned from server')

        signed_data_ba = resp_fs.content[output_field]
        signed_data = BitstreamFactory().generate_from_bytearray(signed_data_ba)
        return signed_data

    def sign_cmf(self, cmf):
        cmd_fs = CommandResponseFS()
        cmd_fs.command = CssServer.SIGN_CMF_COMMAND
        cmd_fs.content['cmf'] = cmf.get_raw_byte_array()
        self.update_commandresponse(cmd_fs)
        resp_fs = self.execute(cmd_fs)
        return self.get_response(resp_fs, 'signed_cmf')

    def sign_cert(self, cert):
        cmd_fs = CommandResponseFS()
        cmd_fs.command = CssServer.SIGN_CERT_COMMAND
        cmd_fs.content['cert'] = cert.get_raw_byte_array()
        self.update_commandresponse(cmd_fs)
        resp_fs = self.execute(cmd_fs)
        return self.get_response(resp_fs, 'signed_cert')

    def sign_eph(self, filestosign):
        cmd_fs = CommandResponseFS()
        cmd_fs.command = CssServer.SIGN_EPH_COMMAND
        self.update_commandresponse(cmd_fs)
        filenames = ""
        for filename in filestosign:
            filenames += filename + ","
            cmd_fs.content['files/' + filename] = filestosign[filename]
        cmd_fs.content['files.txt'] = filenames[:-1]
        resp_fs = self.execute(cmd_fs)
        # Need to extract files in response

        #resp = self.get_response(resp_fs, 'files.txt')
        resp = resp_fs.content['files.txt']
        returnval = {}
        for filename in resp.split(','):
            returnval[filename] = BitstreamFactory().generate_from_bytearray(resp_fs.content[filename])
        return returnval

class CssClientServerTest(unittest.TestCase):

    def test_sanity(self):
        server = CssServer()
        self.assertEqual(server.use_code_sign_server, False)

        client = CssClient()
        self.assertEqual(client.use_code_sign_server, False)

    def test_sign_cmf(self):
        server = CssServer()
        server.debug = True

        client = CssClient()
        client.debug = True

        listen_thr = server.listen()
        client.add_server(hostname='localhost', port=server.port)
        client.ping()

        cmf = Cmf()
        cmf.load("../test/files/example.cmf")
        client.sign_keypair_file = "../test/keys/public0_p384.kp"
        client.sign_keychain_file = "../test/keys/codesign1.qky"
        client.use_code_sign_server = False

        signed_cmf = client.sign_cmf(cmf)
        signed_cmf.validate()

        client.exit()

        listen_thr.join(timeout=5)

    def test_sign_cert(self):
        server = CssServer()
        server.debug = True

        client = CssClient()
        client.debug = True

        listen_thr = server.listen()
        client.add_server(hostname='localhost', port=server.port)
        client.ping()

        cert = EngineeringCert()
        cert.load("../test/files/example_eng_cert.cert")
        client.sign_keypair_file = "../test/keys/public0_p384.kp"
        client.sign_keychain_file = "../test/keys/codesign1.qky"
        client.use_code_sign_server = False

        signed_cert = client.sign_cert(cert)
        signed_cert.validate()
        print signed_cert

        client.exit()

        listen_thr.join(timeout=5)

    def test_server_userlist(self):
        server = CssServer()
        server.authorized_users = []
        server.authorized_users_file = None
        self.assertEqual(server.is_user_authorized('foo'), False)
        self.assertEquals(len(server.authorized_users), 0)
        server.add_user('foo')
        self.assertEquals(len(server.authorized_users), 1)
        self.assertEqual(server.is_user_authorized('foo'), True)
        self.assertEqual(server.is_user_authorized('bar'), False)
        server.add_user('foo')
        self.assertEquals(len(server.authorized_users), 1)
        server.add_user('bar')
        self.assertEquals(len(server.authorized_users), 2)
        self.assertEqual(server.is_user_authorized('foo'), True)
        self.assertEqual(server.is_user_authorized('bar'), True)

        server.add_list_of_users("jds1 jds2\njds3\njds4,jds5\n  foo bar\n,\n    foobar    \n, jds6")

        #print str(server.authorized_users)
        self.assertEqual(server.is_user_authorized('foo'), True)
        self.assertEqual(server.is_user_authorized('bar'), True)
        self.assertEqual(server.is_user_authorized('foobar'), True)
        self.assertEqual(server.is_user_authorized('jds6'), True)
        self.assertEquals(len(server.authorized_users), 9)

if __name__ == '__main__':
    unittest.main()
