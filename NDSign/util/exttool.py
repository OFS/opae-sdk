#!/usr/bin/env python

import subprocess
import unittest


class ExtTool(object):

    def __init__(self):
        pass

    def shell_out(self, cmd, stdin=None, stdout=None, stderr=None, shell=False):
        p = subprocess.Popen(cmd, shell=shell, stdin=stdin, stdout=stdout, stderr=stderr)
        return p

    def execute(self, cmd, verbose=True, cmd_echo=None, expected_return_code=0):

        if verbose:
            if cmd_echo is None:
                print (str(cmd))
                print str.join(" ",cmd)
            else:
                print cmd_echo

        p = self.shell_out(cmd=cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        out, err = p.communicate()
        return_code = p.wait()
        if verbose:
            if out is not None: print out
            if err is not None: print err
            print "Return code: " + str(return_code)

        if expected_return_code is not None:
            if return_code != expected_return_code:
                raise ValueError(cmd[0] + ' command returned unexpected return code. Expected a return code of ' \
                                 + str(expected_return_code) + ' and received a return code of ' \
                                 + str(return_code) + '.')
        return (return_code, out)

class ExtToolTest(unittest.TestCase):

    def test_constructor(self):
        exe = ExtTool()
        self.assertNotEqual(exe, None)
        exe.execute(['echo', 'hello world'])
        exe.execute(['echo', 'hello world but do not tell me about it'], verbose=False)

    def test_false(self):
        exe = ExtTool()
        exe.execute(cmd=['false'],expected_return_code=1)

    def test_capture(self):
        exe = ExtTool()
        rc, out = exe.execute(['echo', 'hello world'])
        self.assertEqual(rc, 0)
        self.assertEqual(out, 'hello world\n')
