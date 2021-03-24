#!/usr/bin/env python3
# Copyright(c) 2021, Intel Corporation
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

import argparse
import re
import sys
from collections import defaultdict

SCRIPT_VERSION = '1.0.0'

CATEGORY_PATTERN = r'\+-+\+\n;\s*(?P<category>(?:\w+\s?)+)\s*;\n(?P<values>.*?\n)\+-+\+-+\+'
DATA_ITEM_PATTERN = r';\s*(?P<label>(?:\w+\s?)+)\s*;\s*(?P<value>.*?)\s;'

TEMPERATURE_CATEGORY = 'Temperature and Cooling'

DEGREES_C = '\u00b0C'
DEGREES_c = '\u00b0c'


class temp_verifier:
    def __init__(self, args):
        self.args = args

    def verify_units(self, units):
        return units in [DEGREES_C, DEGREES_c]

    def verify_min_temp(self, value):
        return value >= self.args.min_temp

    def verify(self, items):
        min_temp_failures = 0
        for label, value_units in items:
            value, units = value_units.split(' ')
            value = float(value)

            if not self.verify_units(units):
                print('Error: units not {}'.format(DEGREES_C))
                return False
            if not self.verify_min_temp(value):
                print('INFO: QPA threshold value for sensor {} : {}\n'
                      'is lower than the platform\'s recommended '
                      'minimum of {} \u00b0C'.format(label,
                          value_units, self.args.min_temp))
                min_temp_failures += 1

            print('{} {} {}'.format(label, value, units))

        if min_temp_failures == len(items):
            print('WARNING: All threshold values are below '
                  'the recommended minimum ({} \u00b0C).\n'
                  'Check QPA settings.'.format(self.args.min_temp))
            return False

        return True


def get_verifier(category):
    """Return the verifier class, given a category name."""
    verifiers = { TEMPERATURE_CATEGORY: temp_verifier
    }
    return verifiers[category]


def parse_args():
    """Parses command line arguments"""
    parser = argparse.ArgumentParser()

    parser.add_argument('file', type=argparse.FileType('r'), nargs='?',
                        help='Input QPA report to process')

    parser.add_argument('-t', '--min-temp', type=float,
                        default=90.0, help='minimum temperature')

    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s {}'.format(SCRIPT_VERSION),
                        help='display version information and exit')

    return parser, parser.parse_args()


def read_qpa(in_file):
    """Read the input file and convert it to our data structure.
       ie a dict keyed by the category name. The value for each
       entry is a list of 2-tuples of (label, value_and_units).
    """
    category_re = re.compile(CATEGORY_PATTERN, re.DOTALL|re.UNICODE)
    item_re = re.compile(DATA_ITEM_PATTERN, re.UNICODE)
    d = defaultdict(list)
    for mc in category_re.finditer(in_file.read()):
        for mv in item_re.finditer(mc.group('values')):
            label = mv.group('label').strip()
            value = mv.group('value').strip()
            d[mc.group('category').strip()].append((label, value))
    return d


def main():
    """The main entry point"""
    parser, args = parse_args()

    if args.file is None:
        print('Error: file is a required argument\n')
        parser.print_help(sys.stderr)
        sys.exit(1)

    data = read_qpa(args.file)
    for category, key_vals in data.items():
        try:
            verifier = get_verifier(category)(args)
        except KeyError:
            print('Error: category {} is not supported.'.format(category))
            continue

        if not verifier.verify(key_vals):
            print('{}: verification failed.'.format(category))
            sys.exit(1)




if __name__ == '__main__':
    main()
