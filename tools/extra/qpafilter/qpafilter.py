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

""" Convert QPA text file to blob. """

import argparse
from collections import defaultdict
import difflib
import json
import logging
import re
import struct
import sys
import yaml

SCRIPT_VERSION = '1.0.0'

CATEGORY_PATTERN = (r'\+-+\+\n;\s*(?P<category>(?:\w+\s?)+)\s*;\n'
                    r'(?P<values>.*?\n)\+-+\+-+\+')
DATA_ITEM_PATTERN = (r';\s*(?P<label>(?:\w+\s?)+)\s*;'
                     r'\s*(?P<value>\d+\.?\d*)\s+(?P<units>.*?)\s*;')

TEMPERATURE_CATEGORY = 'Temperature and Cooling'

DEGREES_C = '\u00b0C'
DEGREES_c = '\u00b0c'

BLOB_START_MARKER = b'\xff\xa5\x5a\xff\xff\xa5\x5a\xff\xff\xa5\x5a\xff'
BLOB_END_MARKER = b'\xfe\xa5\x5a\xef\xfe\xa5\x5a\xef\xfe\xa5\x5a\xef'

# TODO: endian-ness?
BLOB_STRUCT_FORMAT = '<LLL'
BLOB_STRUCT_SIZE = 12

LOG = logging.getLogger()


class blob_writer:
    """Given a file name, sensors dict, and thresholds dict,
       provide the mechanism for writing the binary blob.
       Implements the with protocol to perform auto file
       cleanup.

       eg
         with blob_writer(fname, sensor_map, threshold_map) as wr:
            <for each sensor>
                wr.writer_sensor(sensor)
    """
    def __init__(self, fname, sensor_map, threshold_map):
        self.fname = fname
        self.sensor_map = sensor_map
        self.threshold_map = threshold_map
        self.outfile = None

    def __enter__(self):
        self.outfile = open(self.fname, 'wb')
        self.write_start_marker()
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        if not exc_type:
            self.write_end_marker()
            self.outfile.close()
            return True
        return False

    def write_start_marker(self):
        """Write the start of data marker."""
        self.outfile.write(BLOB_START_MARKER)

    def write_end_marker(self):
        """Write the end of data marker."""
        self.outfile.write(BLOB_END_MARKER)

    def write_sensor(self, sensor):
        """Write the Upper Warning and Upper Fatal blob
           entries for the given sensor."""
        label = sensor['label']
        fatal = sensor['filtered_fatal']
        warning = sensor['filtered_warning']

        sensor_id = self.sensor_map[label]
        upper_warning_id = self.threshold_map['Upper Warning']
        upper_fatal_id = self.threshold_map['Upper Fatal']

        self.outfile.write(struct.pack(BLOB_STRUCT_FORMAT,
                                       sensor_id,
                                       upper_warning_id,
                                       warning))
        self.outfile.write(struct.pack(BLOB_STRUCT_FORMAT,
                                       sensor_id,
                                       upper_fatal_id,
                                       fatal))


class blob_reader:
    """Given an input file pointer, a sensor map, and a threshold map,
       provide an iterator interface to reading the raw sensor data
       items sequentially."""
    def __init__(self, blobfp, sensor_map, threshold_map):
        self.blob = blobfp.read()
        self.start_marker_index = self.blob.find(BLOB_START_MARKER)
        self.end_marker_index = self.blob.find(BLOB_END_MARKER)
        self.sensor_map = sensor_map
        self.threshold_map = threshold_map

    class Iterator:
        """Iterate over an in-memory blob. Calculate the number of
           sensor entries by subtracting the offset of the first entry
           from the offset of the end marker. Iterate and decode each
           sensor entry, changing the raw data to human-readable output."""
        def __init__(self,
                     blob,
                     start_marker_index,
                     end_marker_index,
                     sensor_map,
                     threshold_map):
            self.blob = blob
            self.start_marker_index = start_marker_index
            self.first_entry_index = self.start_marker_index + BLOB_STRUCT_SIZE
            self.end_marker_index = end_marker_index
            self.num_entries = ((self.end_marker_index -
                                 self.first_entry_index) / BLOB_STRUCT_SIZE)
            self.entry = 0
            self.struct_iter = struct.iter_unpack(
                BLOB_STRUCT_FORMAT,
                self.blob[self.start_marker_index + len(BLOB_START_MARKER):])
            self.sensor_map = sensor_map
            self.threshold_map = threshold_map

        def __next__(self):
            if self.entry == self.num_entries:
                raise StopIteration
            self.entry += 1
            sensor_id, threshold_id, raw_value = next(self.struct_iter)
            return (self.sensor_map[sensor_id],
                    self.threshold_map[threshold_id],
                    raw_value / 2)

    def __iter__(self):
        return self.Iterator(self.blob,
                             self.start_marker_index,
                             self.end_marker_index,
                             self.sensor_map,
                             self.threshold_map)

    def __bool__(self):
        """Return indication of whether the input blob contains
           valid start and end markers."""
        if (self.start_marker_index == -1 or self.end_marker_index == -1):
            return False

        if (self.end_marker_index < (self.start_marker_index +
            len(BLOB_START_MARKER))):
            return False

        first_entry_index = self.start_marker_index + BLOB_STRUCT_SIZE
        if ((self.end_marker_index - first_entry_index) %
            BLOB_STRUCT_SIZE) != 0:
            return False

        return True


class temp_verifier:
    """Verify temperature sensor settings."""
    def __init__(self, args):
        self.args = args

    def verify_units(self, units):
        """Temperature units must be degrees C."""
        return units in [DEGREES_C, DEGREES_c]

    def verify_min_temp(self, value):
        """Each temperature setting must be greater or
           equal to the prescribed minimum value."""
        return value >= self.args.min_temp

    def verify(self, items):
        """Perform all validity checks on the input
           temperature setting."""
        min_temp_failures = 0
        for i in items:
            label = i['label']
            fatal = float(i['fatal'])
            units = i['units']

            if not self.verify_units(units):
                LOG.error(f'Units for {label} not {DEGREES_C}.')
                return False
            if not self.verify_min_temp(fatal):
                LOG.info(f'QPA threshold value for sensor '
                         f'{label}: {fatal} {units}')
                LOG.info(f'is lower than the platform\'s recommended '
                         f'minimum of {self.args.min_temp} {DEGREES_C}.')
                min_temp_failures += 1

        if min_temp_failures == len(items):
            LOG.warning(f'WARNING: All threshold values are below the '
                        f'recommended minimum ({self.args.min_temp} '
                        f'{DEGREES_C}).')
            LOG.warning('Check QPA settings.')
            return False

        return True


def get_verifier(category):
    """Return the verifier class, given a category name."""
    verifiers = {TEMPERATURE_CATEGORY: temp_verifier}
    return verifiers[category]


class temp_filter:
    """Calculate Warning Temperature thresholds, given the Fatal
       value and supporting ratio."""
    def __init__(self, args):
        self.args = args

    def filter(self, items):
        """Perform the necessary translation from temperature
           input value to temperature output. The Upper Fatal
           temperature is given in the input data. Calculate
           the Upper Warning value from the Upper Fatal value,
           by applying the (Virtual Warning Temp) /
           (Virtual Fatal Temp) ratio. If there is an override
           value, then apply it as a percentage of the Upper
           Fatal value instead.
           To form the final output, each temperature is
           multiplied by two, then rounded to the nearest
           integer."""
        for i in items:
            fatal = float(i['fatal'])
            units = i['units']

            if 'override' in i:
                override = float(i['override'])
                warning = (override * fatal) / 100.0
            else:
                override = 0.0
                warning = ((fatal * self.args.virt_warn_temp) /
                           self.args.virt_fatal_temp)

            i['warning'] = str(warning)

            # TODO: is this rounding correct?
            i['filtered_warning'] = int(round(2 * warning))
            i['filtered_fatal'] = int(round(2 * fatal))

            msg = (f'{i["label"]} warning: {i["warning"]} {units} '
                   f'fatal: {i["fatal"]} {units} -> {i["filtered_warning"]} '
                   f'{i["filtered_fatal"]}')
            if override != 0.0:
                msg += f' (override {override}%)'

            # TODO: what to print here?
            print(msg)

        return True


def get_filter(category):
    """Return the filter class, given a category name."""
    filters = {TEMPERATURE_CATEGORY: temp_filter}
    return filters[category]


class two_way_map:
    """Given a dict of strings to integers, provide the ability
       to perform a 2-way translation. That is, given a string
       key, retrieve the integer value. Given an integer key,
       return the string value."""
    def __init__(self, str_to_int):
        self.str_to_int = str_to_int
        self.int_to_str = {v: k for k, v in str_to_int.items()}

    def __getitem__(self, i):
        if isinstance(i, str):
            return self.str_to_int[i]
        if isinstance(i, int):
            return self.int_to_str[i]
        raise TypeError('two_way_map index must be str or int')


def read_qpa(in_file, temp_overrides):
    """Read the input file and convert it to our data structure.
       ie a dict keyed by the category name. The value for each
       entry is a list of dictionaries with keys = {'label',
       'fatal', 'warning', 'units'}.
    """
    override_d = {}
    for override in temp_overrides:
        try:
            olabel, opercentage = override.split(':')
        except ValueError:
            LOG.warning(f'{override} is not a valid temperature '
                        f'override. Skipping')
            continue
        override_d[olabel] = opercentage

    sensors_list = []
    category_re = re.compile(CATEGORY_PATTERN, re.DOTALL | re.UNICODE)
    item_re = re.compile(DATA_ITEM_PATTERN, re.UNICODE)
    outer_d = defaultdict(list)
    for cat_match in category_re.finditer(in_file.read()):
        category = cat_match.group('category').strip()
        for val_match in item_re.finditer(cat_match.group('values')):
            label = val_match.group('label').strip()
            sensors_list.append(label)
            fatal = val_match.group('value').strip()
            units = val_match.group('units').strip()

            inner_d = {'label': label,
                       'fatal': fatal,
                       'units': units,
                       'warning': 0.0}  # placeholder

            if label in override_d:
                inner_d['override'] = override_d.pop(label)
                LOG.info(f'Setting override for \'{label}\' '
                         f'to {inner_d["override"]}%')

            outer_d[category].append(inner_d)

    for override in override_d:
        LOG.warning(f'Temperature override for "{override}" unused.')
        for possible in difflib.get_close_matches(override, sensors_list):
            LOG.warning(f'Did you mean "{possible}"?')

    return outer_d


def create_blob_from_qpa(args):
    """Given the input QPA report, create the binary equivalent."""
    data = read_qpa(args.file, args.override_temp)

    with blob_writer(args.output,
                     args.sensor_map,
                     args.threshold_map) as writer:
        for category, key_vals in data.items():
            try:
                verifier = get_verifier(category)(args)
                filt = get_filter(category)(args)
            except KeyError:
                LOG.error(f'Category {category} is not supported. Skipping')
                continue

            if not verifier.verify(key_vals):
                LOG.error(f'{category}: verification failed.')
                sys.exit(1)

            if not filt.filter(key_vals):
                LOG.error(f'{category}: filtering failed.')
                sys.exit(1)

            for sensor in key_vals:
                writer.write_sensor(sensor)


def dump_blob(args):
    """Given the binary from a previous 'create' command, print
       human-readable output."""
    reader = blob_reader(args.file, args.sensor_map, args.threshold_map)

    if not reader:
        LOG.error(f'{args.file.name} is not a valid blob.')
        sys.exit(1)

    headers = ('sensor', 'threshold', 'value')
    data = [dict(zip(headers, values)) for values in reader]

    if args.format == 'json':
        json.dump(data, args.output, indent=4, sort_keys=True)
    elif args.format == 'yaml':
        args.output.write(yaml.dump(data))
    elif args.format == 'csv':
        for d in data:
            args.output.write(f"{','.join(map(str, d.values()))}\n")


def read_sensors(fname):
    """Given the name of a yaml file containing the sensor
       label to ID mapping, return a two_way_map containing
       the data."""
    with open(fname, 'r') as infile:
        return two_way_map(yaml.load(infile, Loader=yaml.SafeLoader))


def parse_args():
    """Parses command line arguments"""
    parser = argparse.ArgumentParser()

    parser.add_argument('-l', '--log-level',
                        choices=['debug',
                                 'info',
                                 'warning',
                                 'error',
                                 'critical'],
                        default='info', help='log level to use')

    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s {}'.format(SCRIPT_VERSION),
                        help='display version information and exit')

    parser.add_argument('-s', '--sensor-file',
                        type=read_sensors, dest='sensor_map',
                        default='n5010_bmc_sensors.yml',
                        help='BMC sensor to id file')

    subparser = parser.add_subparsers()

    create = subparser.add_parser('create', help='Create blob from QPA')

    create.add_argument('file', type=argparse.FileType('r'), nargs='?',
                        help='Input QPA report to process')

    create.add_argument('-t', '--min-temp', type=float,
                        default=90.0, help='minimum temperature')

    create.add_argument('-f', '--virt-fatal-temp', type=float,
                        default=100.0,
                        help='virtual fatal temperature threshold')

    create.add_argument('-w', '--virt-warn-temp', type=float,
                        default=90.0,
                        help='virtual warning temperature threshold')

    create.add_argument('-O', '--override-temp', action='append',
                        default=[],
                        help='specify a temperature override as '
                        '<label>:<percentage>')

    create.add_argument('-o', '--output', default='qpafilter.blob',
                        help='Output blob file')

    create.set_defaults(func=create_blob_from_qpa)

    dump = subparser.add_parser('dump',
                                help='Convert blob to human-readable format')

    dump.add_argument('file', type=argparse.FileType('rb'), nargs='?',
                      help='Input blob file')

    dump.add_argument('-o', '--output', type=argparse.FileType('w'),
                      default=sys.stdout, nargs='?',
                      help='Output text file (default=stdout)')

    dump.add_argument('-F', '--format',
                      choices=['csv', 'json', 'yaml'],
                      default='csv', help='select output format')

    dump.set_defaults(func=dump_blob)

    return parser, parser.parse_args()


def main():
    """The main entry point"""
    parser, args = parse_args()

    if not hasattr(args, 'file'):
        print('Error: file is a required argument.\n')
        parser.print_help(sys.stderr)
        sys.exit(1)

    LOG.setLevel(logging.NOTSET)
    log_fmt = '[%(levelname)-8s] %(message)s'
    log_hndlr = logging.StreamHandler(sys.stdout)
    log_hndlr.setFormatter(logging.Formatter(log_fmt))
    log_hndlr.setLevel(logging.getLevelName(args.log_level.upper()))
    LOG.addHandler(log_hndlr)

    setattr(args, 'threshold_map',
            two_way_map({'Upper Warning': 0,
                         'Upper Critical': 1,
                         'Upper Fatal': 2,
                         'Lower Warning': 3,
                         'Lower Critical': 4,
                         'Lower Fatal': 5}))

    if hasattr(args, 'func'):
        args.func(args)
    else:
        LOG.error('You must specify a sub-command.')
        parser.print_help(sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
