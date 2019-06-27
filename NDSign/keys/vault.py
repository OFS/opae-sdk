#!/usr/bin/env python
import os
import unittest


class Vault(object):
    '''
    The Vault is where you store and get your Keys
    '''

    def __init__(self):
        self.initialize()

    def root_dir(self):
        return os.path.dirname(os.path.realpath(__file__)).replace('\\','/')

    def initialize(self):
        self.qky = []
        self.pubkey = []
        self.keypair = []
        self.pem = []

        for root, _dirs, files in os.walk(self.root_dir()):
            for key_file in files:
                if key_file.endswith(".qky"):
                    self.qky.append(os.path.join(root, key_file).replace('\\','/'))
                elif key_file.endswith("_pubkey.bin"):
                    self.pubkey.append(os.path.join(root, key_file).replace('\\','/'))
                elif key_file.endswith(".kp"):
                    self.keypair.append(os.path.join(root, key_file).replace('\\','/'))
                elif key_file.endswith(".pem"):
                    self.pem.append(os.path.join(root, key_file).replace('\\','/'))

    def search_by_filename(self, filename):
        keys_found = []
        key_list = self.qky + self.pubkey + self.keypair + self.pem

        filename_search_string = filename.replace('\\','/')
        filename_search_string = '/' + filename_search_string.lstrip('/')

        for key in key_list:
            if key.endswith(filename_search_string):
                keys_found.append(key)

        return keys_found

class VaultTest(unittest.TestCase):

    def test_constructor(self):
        v = Vault()
        self.assertNotEqual(v, None)
        self.assertEquals(v.root_dir(),os.path.dirname((os.path.realpath(__file__))).replace('\\','/'))
        print "Vault Root Dir is: " + v.root_dir()
        self.assertEqual(len(v.qky), 20)
        self.assertEqual(len(v.keypair), 2)
        self.assertEqual(len(v.pubkey), 18)
        self.assertEqual(len(v.pem), 0)

    def test_search_by_filename(self):
        v = Vault()
        qky = v.search_by_filename(".qky")
        self.assertEqual(len(qky), 0)
        self.assertEqual(len(v.search_by_filename(".foo")),0)

        qky = v.search_by_filename("STRATIX10ROOTKEY.qky")
        print qky

    def test_negative_filename_cases(self):
        v = Vault()
        qky = v.search_by_filename("STRATIX10ROOTKEY")
        self.assertEqual(len(qky), 0)

if __name__ == '__main__':
    unittest.main()
