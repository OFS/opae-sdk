#!/usr/bin/env python

import os
import socket
from threading import Thread
import time
import unittest

from util.convert import Convert
from util.mkdir import Mkdir


class Server(object):

    MAGIC = 0x3053444A

    def __init__(self):
        super(Server,self).__init__()
        #self.port = 1433
        self.port = 0
        self.debug = False
        self.callback = self.print_msg
        self.callback_timeout = 15.0
        self.keep_listening = True
        self.listening = False
        self.send = Server.send
        self.recv = Server.recv

    @staticmethod
    def send(socket, data):
        numOfBytesSent = socket.send(Convert().integer_to_bytes(n=Server.MAGIC, length=4))
        if numOfBytesSent != 4: raise RuntimeError('could not send magic')

        numOfBytesSent = socket.send(Convert().integer_to_bytes(n=len(data), length=8))
        if numOfBytesSent != 8: raise RuntimeError('could not send header')

        #print "Sent Message Length: " + str(len(data)) + '\n'

        totalsent = 0
        while totalsent < len(data):
            sent = socket.send(data[totalsent:])
            if sent == 0: raise RuntimeError("socket connection broken")
            totalsent = totalsent + sent

    @staticmethod
    def recv(socket):
        magic_bytearray = socket.recv(4)
        magic = Convert().bytearray_to_integer(magic_bytearray, num_of_bytes=4)
        if magic != Server.MAGIC: raise RuntimeError('server did not receive expected magic value - ignoring ' + hex(magic))

        msglen_bytearray = socket.recv(8)
        msglen = Convert().bytearray_to_integer(msglen_bytearray, num_of_bytes=8)

        #print "Recv Message Length: " + str(msglen) + '\n'

        chunks = []
        bytes_recd = 0
        while bytes_recd < msglen:
            chunk = socket.recv(min(msglen - bytes_recd, 4096))
            if chunk == '': raise RuntimeError("socket connection broken")
            chunks.append(chunk)
            bytes_recd = bytes_recd + len(chunk)
        return ''.join(chunks)

    def log(self,msg):
        if self.debug: print msg

    def print_msg(self, clientsocket):
        try:
            print self.recv(clientsocket)
        finally:
            clientsocket.close()

    def loopback(self, clientsocket):
        try:
            data = self.recv(clientsocket)
            self.send(clientsocket,data)
        finally:
            clientsocket.close()

    def recv_file(self, clientsocket, filename):
        filepathdir = os.path.dirname(filename)
        Mkdir().mkdir_p(filepathdir)
        fp = open(self.recvfile, 'wb')

        try:
            data = self.recv(clientsocket)
            fp.write(data)
        finally:
            clientsocket.close()
            fp.close()

        self.log('Server got %s at %s' % (filename, time.asctime()))

    def listen_blocking(self):
        assert(not self.listening)

        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind(('',self.port))

        self.log("Listening on Port: " + str(server_socket.getsockname()[1]) + "\n")

        port_on_entry = self.port
        self.port = server_socket.getsockname()[1]

        max_connections = 64
        server_socket.listen(max_connections)
        thread_stack = []
        self.listening = True

        try:
            while True:
                (clientsock, clientaddr) = server_socket.accept()
                self.log('Server connected by %s at %s' % (clientaddr, time.asctime()))

                t = Thread(target=self.callback, args=(clientsock,))
                self.log("Starting thread: " + t.name + "\n")
                t.start()
                thread_stack.append(t)

                if not self.keep_listening: break

                for t in thread_stack:
                    if not t.isAlive():
                        thread_stack.remove(t)

            # Give any outstanding thread "callback_timeout" seconds to terminate
            for t in thread_stack:
                if t.isAlive():
                    self.log("Waiting for thread: " + t.name + "\n")
                    #ToDo: Test and handle the timeout usecase here
                    t.join(timeout=self.callback_timeout)
                    thread_stack.remove(t)

        except ValueError as err:
            print str(err)
        except RuntimeError as err:
            print str(err)

        finally:
            server_socket.close()
            self.port = port_on_entry
            self.listening = False

        return None

    def listen_nonblocking(self):
        listen_thread = Thread(target=self.listen_blocking, args=())
        listen_thread.start()

        timeout_counter = 0
        while not self.listening:
            time.sleep(0.1)
            timeout_counter += 0.1
            if timeout_counter > 15.0: raise RuntimeError("Something went wrong with the listen() thread spawned here")

        assert(self.port != 0)

        return listen_thread

    def listen(self):
        return self.listen_nonblocking()

class Client(object):

    def __init__(self):
        super(Client,self).__init__()
        self.debug = False
        self.sock = None
        self.connected = False
        self.serveraddr = []
        self.connect_timeout = 3

    def log(self,msg):
        if self.debug: print msg

    def add_server(self, hostname, port):
        self.serveraddr.append((hostname,port))

    def connect(self):
        assert(not self.connected)

        for s in self.serveraddr:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            old_timeout = self.sock.gettimeout()

            # extra sanity
            assert(old_timeout is None)

            try:
                self.sock.settimeout(self.connect_timeout)
                self.sock.connect(s)
                self.connected = True
                assert(self.sock is not None)
                break
            except Exception:
                print "WARNING: Failed to connect to " + s[0] + ":" + str(s[1])
            finally:
                self.sock.settimeout(old_timeout)

    def disconnect(self):
        assert(self.connected)
        self.sock.close()
        self.sock = None
        self.connected = False

    def send(self,data):
        assert(self.connected)
        Server.send(self.sock,data)

    def recv(self):
        assert(self.connected)
        return Server.recv(self.sock)


class ClientServerTest(unittest.TestCase):

    def test_server_simple(self):
        server = Server()
        server.debug = True
        server.keep_listening = False

        listen_thread = server.listen()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', server.port))
        sock.settimeout(None)
        server.send(sock,'foobar\n')
        sock.close()
        listen_thread.join(timeout=15.0)

    def test_client_server_simple(self):
        server = Server()
        server.debug = True
        server.keep_listening = False
        server.callback = server.loopback

        client = Client()
        client.debug = True
        client.connect_timeout = 0.1

        listen_t = server.listen()
        self.assertTrue(server.listening)
        client.add_server(hostname='wrong_server_name', port=server.port)
        client.add_server(hostname='localhost', port=0)
        client.add_server(hostname='localhost', port=server.port)
        client.connect()

        # ToDo: This assertion fails on Linux, debug why this is...
        self.assertTrue(client.connected)

        data_send = bytearray(range(256)) * 100000
        client.send(data_send)
        data_recv = client.recv()
        client.disconnect()
        self.assertFalse(client.connected)
        listen_t.join(timeout=1.0)
        self.assertFalse(server.listening)
        self.assertEqual(data_send, data_recv)
        self.assertEqual(len(data_recv),25600000)

if __name__ == '__main__':
    unittest.main()
