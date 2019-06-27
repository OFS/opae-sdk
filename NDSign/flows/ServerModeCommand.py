#!/usr/bin/env python

import os
import unittest

from flows.SignCommand import SignCommand
from networking.cssserver import CssServer, CssClient


class ServerModeCommand(SignCommand):
    '''
    the servermode command
    '''
    DEFAULT_PORT = 1750

    def __init__(self):
        super(ServerModeCommand,self).__init__()
        self.name = 'servermode'
        self.css = CssServer()

    def add_parser(self, parser):
        super(ServerModeCommand,self).add_parser(parser=parser)
        self.add_css_project_argument()
        self.add_css_keyhash_argument()
        self.parser.add_argument('--port', default=ServerModeCommand.DEFAULT_PORT, \
                            type=int, help='TCP Port for Signing Server to listen on (default is ' +
                            str(ServerModeCommand.DEFAULT_PORT) + ')')

        self.parser.add_argument('--authorized_users', default=None, \
                            type=str, help='A file that stores a list of authorized usernames')

    def execute(self, args):
        super(ServerModeCommand, self).execute(args=args)

        self.execute_cmd(signtool=args.signtool, port=args.port,
                         css_project=args.css_project, css_keyhash=args.css_keyhash, authorized_users_file=args.authorized_users)

    def execute_cmd(self, signtool, port, css_project, css_keyhash, authorized_users_file):
        super(ServerModeCommand,self).execute_cmd(signtool=signtool)

        self.set_css_project(css_project)
        self.set_css_keyhash(css_keyhash)

        if self.css.use_code_sign_server:
            self.css.passwd()

        self.css.port = port

        if authorized_users_file is not None:
            if not os.path.isfile(authorized_users_file):
                raise ValueError('Authorized User File does not exist - ' + authorized_users_file)
            self.css.authorized_users_file = authorized_users_file

        listen_thread = self.css.listen()
        print "CSS Server is Listening on Port " + str(self.css.port) + ". Type 'exit' to exit."

        while True:
            cmd = raw_input('> ')
            if cmd == 'exit' or cmd == 'quit':
                if self.css.port is not None and self.css.port != 0 and self.css.listening:
                    client = CssClient()
                    client.add_server(hostname='localhost', port=self.css.port)
                    client.exit()
                break
            elif cmd == 'ping':
                if self.css.listening:
                    client = CssClient()
                    client.add_server(hostname='localhost', port=self.css.port)
                    client.ping()
                else:
                    print "Server is no longer listening"
            elif cmd == '':
                pass
            else:
                print "Invalid Command: " + cmd + "\n"

        listen_thread.join()

class ServerModeCommandTest(unittest.TestCase):

    def testSignCmf(self):
        pass

if __name__ == '__main__':
    unittest.main()