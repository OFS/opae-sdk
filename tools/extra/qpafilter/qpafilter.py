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
import difflib
import io
import json
import logging
import os
import re
import struct
import sys
import zlib
from collections import defaultdict

import yaml

SCRIPT_VERSION = '1.0.1'

CATEGORY_PATTERN = (r'\+-+\+\n;\s*(?P<category>(?:\w+\s?)+)\s*;\n'
                    r'(?P<values>.*?\n)\+-+\+-+\+')
DATA_ITEM_PATTERN = (r';\s*(?P<label>(?:\w+\s?)+)\s*;'
                     r'\s*(?P<value>\d+\.?\d*)\s+(?P<units>.*?)\s*;')

TEMPERATURE_CATEGORY = 'Temperature and Cooling'

DEGREES_C = '\u00b0C'
DEGREES_c = '\u00b0c'

BLOB_HEADER_MAGIC = 0xa7cbef29
BLOB_HEADER_SIZE = 20
BLOB_HEADER_FORMAT = '<IIII'

BLOB_STRUCT_FORMAT = '<LLL'
BLOB_STRUCT_SIZE = 12

VIRTUAL_TEMP_SENSOR0 = 'FPGA Virtual Temperature Sensor 0'

LOG = logging.getLogger()


def CRC32(data, value=0):
    """Find the CRC32 of data, with given start value."""
    return zlib.crc32(data, value) & 0xffffffff


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
    version = 0

    def __init__(self, fname, sensor_map, threshold_map):
        self.fname = fname
        self.sensor_map = sensor_map
        self.threshold_map = threshold_map
        self.payload = None

    def __enter__(self):
        self.payload = io.BytesIO()
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        if not exc_type:
            buf = self.payload.getbuffer()
            length = buf.nbytes
            hdr = struct.pack(BLOB_HEADER_FORMAT,
                              BLOB_HEADER_MAGIC,
                              self.version,
                              length,
                              CRC32(buf))
            with open(self.fname, 'wb') as outfile:
                outfile.write(hdr)
                outfile.write(CRC32(hdr).to_bytes(4, byteorder='little'))
                outfile.write(buf)
            return True
        return False

    def write_sensor(self, sensor):
        """Write the Upper Warning and Upper Fatal blob
           entries for the given sensor."""
        label = sensor['label']

        try:
            id_list = list(self.sensor_map.sensor_ids(label))
        except KeyError:
            LOG.error(f'Sensor "{label}" not found in sensors map.')
            sys.exit(1)

        for sensor_id in id_list:
            warning, fatal = self.sensor_map.values_for(sensor_id)

            upper_warning_id = self.threshold_map['Upper Warning']
            upper_fatal_id = self.threshold_map['Upper Fatal']

            self.payload.write(struct.pack(BLOB_STRUCT_FORMAT,
                                           sensor_id,
                                           upper_warning_id,
                                           warning))
            self.payload.write(struct.pack(BLOB_STRUCT_FORMAT,
                                           sensor_id,
                                           upper_fatal_id,
                                           fatal))


class blob_reader_v0:
    """Given an input file pointer, a sensor map, and a threshold map,
       provide an iterator interface to reading the raw sensor data
       items sequentially."""

    version = 0

    def __init__(self, blobfp, sensor_map, threshold_map):
        self.raw_hdr = blobfp.read(BLOB_HEADER_SIZE)
        self.blob_hdr = struct.unpack('<IIIII', self.raw_hdr)
        self.blob_data = blobfp.read()
        self.sensor_map = sensor_map
        self.threshold_map = threshold_map

    class Iterator:
        """Iterate over an in-memory blob. Iterate and decode each
           sensor entry, changing the raw data to human-readable output."""
        def __init__(self,
                     blob,
                     data_size,
                     sensor_map,
                     threshold_map):
            self.blob = blob
            self.size = data_size
            self.parsed = 0
            self.struct_iter = struct.iter_unpack(BLOB_STRUCT_FORMAT,
                                                  self.blob)
            self.sensor_map = sensor_map
            self.threshold_map = threshold_map

        def __next__(self):
            if self.parsed == self.size:
                raise StopIteration

            sensor_id, threshold_id, raw_value = next(self.struct_iter)

            sensor_label = self.sensor_map.sensor_name(sensor_id)
            threshold_name = self.threshold_map[threshold_id]
            readable_value = raw_value / 2

            self.parsed += BLOB_STRUCT_SIZE

            return (sensor_label, threshold_name, readable_value)

    def __iter__(self):
        return self.Iterator(self.blob_data,
                             self.blob_hdr[2],
                             self.sensor_map,
                             self.threshold_map)

    def __bool__(self):
        """Return indication of whether the input blob is valid."""
        if self.blob_hdr[0] != BLOB_HEADER_MAGIC:
            LOG.error(f'header magic number mismatch. '
                      f'Got 0x{self.blob_hdr[0]:0x} expected '
                      f'0x{BLOB_HEADER_MAGIC:0x}.')
            return False

        if self.blob_hdr[1] != self.version:
            LOG.error(f'blob version mismatch. '
                      f'Got {self.blob_hdr[1]} expected '
                      f'{self.version}.')
            return False

        length = len(self.blob_data)
        if self.blob_hdr[2] != length:
            LOG.error(f'payload length mismatch. '
                      f'Got {length} expected '
                      f'{self.blob_hdr[2]}.')
            return False

        payload_crc = CRC32(self.blob_data)
        if self.blob_hdr[3] != payload_crc:
            LOG.error(f'payload CRC mismatch. '
                      f'Got 0x{payload_crc:0x} expected '
                      f'0x{self.blob_hdr[3]:0x}.')
            return False

        hdr_crc = CRC32(self.raw_hdr[:16])
        if self.blob_hdr[4] != hdr_crc:
            LOG.error(f'header CRC mismatch. '
                      f'Got 0x{hdr_crc:0x} expected '
                      f'0x{self.blob_hdr[4]:0x}.')
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
                         f'{label}: {fatal:.2f} {units}')
                LOG.info(f'is lower than the platform\'s recommended '
                         f'minimum of {self.args.min_temp:.2f} {DEGREES_C}.')
                min_temp_failures += 1
                i['fatal'] = str(self.args.min_temp)

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

    def filter(self, items, sensor_map):
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
            # Associate this fatal value with each sensor ID matching
            # sensor name i['label'], and calculate the warning,
            # filtered_warning, and filtered_fatal values.
            sensor_map.filter(i, self.args)

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


class qpamap:
    """Maps a sensors input database into our memory-resident data
       structure. The input file format is as follows:

       label0:
       - id: ID0
         adjustment: ADJ0
       - id: ID1
         adjustment: ADJ1
       ...
       - id: IDn
         adjustment: ADJn

       Where ID0, ID1 ... IDn are integer IDs and ADJ is a floating-
       point adjustment value. If there is no adjustment value for
       the given id, then adjustment can be omitted.
    """
    def __init__(self, data):
        self.data = data
        self.id_map = {}
        for k, v in self.data.items():
            for i in v:
                adj = i.get('adjustment', 0.0)
                self.id_map[i['id']] = {'label': k, 'adjustment': adj}

    def filter(self, item, args):
        """Calculate the fatal/warning temperature values for item,
           updating self.id_map for each integer identifier matching
           item['label']."""
        for ident in self.sensor_ids(item['label']):
            adjustment = self.id_map[ident]['adjustment']
            fatal = float(item['fatal']) + adjustment

            if fatal < args.min_temp:
                fatal = args.min_temp

            if 'override' in item:
                override = float(item['override'])
                warning = (override * fatal) / 100.0
            else:
                override = 0.0
                warning = ((fatal * args.virt_warn_temp) /
                           args.virt_fatal_temp)

            self.id_map[ident]['units'] = item['units']
            self.id_map[ident]['warning'] = warning
            self.id_map[ident]['fatal'] = fatal
            filtered_warning = int(round(2 * warning))
            filtered_fatal = int(round(2 * fatal))
            self.id_map[ident]['filtered_warning'] = filtered_warning
            self.id_map[ident]['filtered_fatal'] = filtered_fatal

            msg = (f'{item["label"]} warning: {warning:.2f} {item["units"]} '
                   f'fatal: {fatal:.2f} {item["units"]} -> {filtered_warning} '
                   f'{filtered_fatal}')
            if adjustment != 0.0:
                msg += f' (adjustment {adjustment}{item["units"]})'
            if override != 0.0:
                msg += f' (override {override}%)'

            LOG.info(msg)

    def values_for(self, ident):
        """Return a tuple of the filtered warning and filtered fatal values."""
        return (self.id_map[ident]['filtered_warning'],
                self.id_map[ident]['filtered_fatal'])

    def sensor_ids(self, label):
        """Generator that produces the sequence of integer IDs, given
           the string label."""
        for i in self.data[label]:
            yield i['id']

    def sensor_name(self, ident):
        """Return the string label for a given integer ID."""
        return self.id_map[ident]['label']


def read_qpa(in_file, temp_overrides, sensors_map):
    """Read the input file and convert it to our data structure.
       ie a dict keyed by the category name. The value for each
       entry is a list of dictionaries with keys = {'label',
       'fatal', 'warning', 'units', 'override'}.
    """
    override_d = {}
    for override in temp_overrides:
        try:
            olabel, opercentage = override.split(':')
        except ValueError:
            LOG.warning(f'{override} is not a valid temperature '
                        f'override. Skipping')
            continue

        # Allow the override options to specify the numeric sensor
        # ID. If the label portion is an integer, and if that integer
        # matches a sensor ID, then convert the integer to the sensor
        # label using the sensors_map.
        try:
            olabel_as_int = int(olabel)
            override_d[sensors_map[olabel_as_int]] = opercentage
        except (ValueError, KeyError):
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
    data = read_qpa(args.file, args.override_temp, args.sensor_map)

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

            if not filt.filter(key_vals, args.sensor_map):
                LOG.error(f'{category}: filtering failed.')
                sys.exit(1)

            for sensor in key_vals:
                writer.write_sensor(sensor)

            virtual_temperature_sensor_0 = {
                'label': VIRTUAL_TEMP_SENSOR0,
                'fatal': args.virt_fatal_temp,
                'warning': args.virt_warn_temp,
                'units': DEGREES_C,
                'filtered_fatal': int(round(2 * args.virt_fatal_temp)),
                'filtered_warning': int(round(2 * args.virt_warn_temp))}

            if not filt.filter([virtual_temperature_sensor_0],
                               args.sensor_map):
                LOG.error(f'{category}: filtering failed.')
                sys.exit(1)

            writer.write_sensor(virtual_temperature_sensor_0)


def get_blob_reader(blobfp, sensor_map, threshold_map):
    """Examine the version field of the blob header to determine
       the correct version of blob_reader to retrieve."""
    readers = {0: blob_reader_v0}

    raw_hdr = blobfp.read(BLOB_HEADER_SIZE)
    blobfp.seek(0, os.SEEK_SET)

    blob_hdr = struct.unpack('<IIIII', raw_hdr)
    version = blob_hdr[1]

    if version not in readers:
        LOG.error(f'blob version {version} is not '
                  f'supported by this version of the script.')
        return None

    return readers[version](blobfp, sensor_map, threshold_map)


def dump_blob(args):
    """Given the binary from a previous 'create' command, print
       human-readable output."""
    reader = get_blob_reader(args.file, args.sensor_map, args.threshold_map)

    if reader is None:
        sys.exit(1)
    elif not reader:
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


def show_sensors(args):
    """Given the input sensors map (args.sensor_map), produce
       human-readable output for the map."""
    for k, v in args.sensor_map.data.items():
        ids_and_adj = ''
        for index, item in enumerate(v):
            ident = item['id']
            ids_and_adj += (f'0x{ident:0x}' if ident & 0x8000
                            else f'{str(ident)}')
            if item.get('adjustment'):
                ids_and_adj += f' ({item["adjustment"]} {DEGREES_C})'
            if index < len(v) - 1:
                ids_and_adj += ', '
        print(f'{k} {{ {ids_and_adj} }}', file=args.output)


def read_sensors(fname):
    """Given the name of a yaml file containing the sensor
       label to ID mapping, return a qpamap containing
       the data."""
    with open(fname, 'r') as infile:
        return qpamap(yaml.load(infile, Loader=yaml.SafeLoader))


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
                        default='n6000_bmc_sensors.yml',
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
                      default=sys.stdout,
                      help='Output text file (default=stdout)')

    dump.add_argument('-F', '--format',
                      choices=['csv', 'json', 'yaml'],
                      default='csv', help='select output format')

    dump.set_defaults(func=dump_blob)

    show = subparser.add_parser('show',
                                help='Display input sensors map')

    show.add_argument('-o', '--output', type=argparse.FileType('w'),
                      default=sys.stdout,
                      help='Output text file (default=stdout)')

    show.set_defaults(func=show_sensors)

    return parser, parser.parse_args()


def main():
    """The main entry point"""
    parser, args = parse_args()

    if not hasattr(args, 'file') and args.func != show_sensors:
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
