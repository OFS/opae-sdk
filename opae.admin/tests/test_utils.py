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
import unittest
from nose2.tools import params
from opae.admin.utils import version_comparator, parse_timedelta


class test_utils(unittest.TestCase):
    def test_version_comparator_parse(self):
        """test_version_comparator_parse
           When the version comparator is constructed with a valid expression,
           then the parse method returns the re.MatchObject object, and the
           property accessors retrieve the parsed fields.
        """
        c = version_comparator('foo == 1.2.3')
        d = c.parse()

        self.assertIsNotNone(d)
        self.assertEqual(d['label'], 'foo')
        self.assertEqual(d['op'], '==')
        self.assertEqual(d['version'], '1.2.3')

        self.assertEqual(c.label, 'foo')
        self.assertEqual(c.operator, '==')
        self.assertEqual(c.version, '1.2.3')

    def test_version_comparator_compare(self):
        """test_version_comparator_compare
           When a version_comparator has successfully parsed its given
           expression,then calling the compare method executes the described
           comparison, returning a boolean value.
        """
        c = version_comparator('foo == 1.2.3')
        self.assertIsNotNone(c.parse())
        self.assertTrue(c.compare('1.2.3'))

        self.assertFalse(c.compare('1.2.7'))

        c = version_comparator('foo < 1.2.3')
        self.assertIsNotNone(c.parse())

        self.assertTrue(c.compare('1.2.2'))
        self.assertTrue(c.compare('1.1.3'))
        self.assertTrue(c.compare('0.2.3'))

        self.assertFalse(c.compare('1.2.3'))
        self.assertFalse(c.compare('1.2.4'))
        self.assertFalse(c.compare('1.3.3'))
        self.assertFalse(c.compare('2.2.3'))

        c = version_comparator('foo >= 1.2.3')
        self.assertIsNotNone(c.parse())

        self.assertTrue(c.compare('1.2.3'))
        self.assertTrue(c.compare('1.2.10'))
        self.assertFalse(c.compare('1.2.2'))
        self.assertTrue(c.compare('1.10.3'))

    @params(('1d23h59m59s', 172799.0),
           ('1m60s2m', 240.0),
           ('1h', 3600.0),
           ('1h60m1s', 7201.0),
           ('59m1.1s', 3541.1),
           ('0.5h', 1800.0))
    def test_parse_timedelta(self, inp_str, sec):
        '''test_parse_timedelta
        Given a valid timedelta string
        When I call parse_timedelta with it
        I get the total number of seconds represented by it
        '''
        self.assertEqual(parse_timedelta(inp_str), sec)

    def test_parse_timedelta_neg(self):
        '''test_parse_timedelta_neg
        Given an invalid timedelta string
        When I call parse_timedelta with it
        I get 0.0
        '''
        self.assertEqual(parse_timedelta('abc'), 0.0)
