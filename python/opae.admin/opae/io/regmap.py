# -*- coding: utf-8 -*-
# Copyright(c) 2020, Intel Corporation
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

"""

"""
import argparse
import ctypes
import struct
import sys
import yaml


yaml_seq_tag = u'tag:yaml.org,2002:seq'


class ofs_field(object):
    def __init__(self, name, bits, access, default, description):
        self.name = name
        self.bits = bits
        self.access = access
        self.default = default
        self.description = description
        self._sorted = sorted(bits)

    def __str__(self):
        s = '{:8} {} {} {:x}'.format(
            self.name,
            '-'.join([str(b) for b in self.bits]),
            self.access,
            self.default)
        return s

    def __repr__(self):
        return str(self)

    def lo(self):
        return self._sorted[0]

    def hi(self):
        return self._sorted[-1]

    def width(self):
        return 1 if len(self.bits) == 1 else (self.hi() - self.lo() + 1)

    def to_list(self):
        return [self.name, self.bits, self.access, self.default]

class ofs_register(yaml.YAMLObject):
    yaml_tag = u'tag:intel.com,2020:ofs/register'

    def __init__(self, name, offset, default, description):
        self.name = name
        self.offset = int(offset, 0)
        self.default = int(default, 0)
        self.description = description
        self.fields = []


    def header(self):
        return [self.name,
                '0x{:0x}'.format(self.offset),
                '0x{:0x}'.format(self.default),
                self.description]

    def add_field(self, field):
        self.fields.append(field)

    def __str__(self):
        s = '{:8} 0x{:04x} 0x{:016x}'.format(
            self.name,
            self.offset,
            self.default)
        return s

    @classmethod
    def to_yaml(cls, dumper, data):
        self = data
        return dumper.represent_list(data)
        meta = dumper.represent_list([self.name, '0x{:04x}'.format(self.offset), '0x{:04x}'.format(self.default), self.description])
        return dumper.represent_list([meta,
                                      [f.to_list(dumper) for f in self.fields]])


class ofs_node(yaml.YAMLObject):
    yaml_tag = u'tag:intel.com,2020:ofs/register'

    def __init__(self, r):
        self.value = r.to_list()


    @classmethod
    def to_yaml(cls, dumper, data):
        return dumper.represent_sequence(cls.yaml_tag,
                                        [dumper.represent_object(data.value[0])])


class ofs_safe_loader(yaml.SafeLoader):
    def construct_ofs_register(self, node):
        items = node.value
        reg = ofs_register(*[n.value for n in items[0].value])
        if len(items) > 1:
            for bf in node.value[1].value:
                info = []
                info.append(bf.value[0].value)
                info.append([int(n.value)
                             for n in bf.value[1].value])
                info.append(bf.value[2].value)
                info.append(int(bf.value[3].value, 0))
                info.append(bf.value[4].value)
                reg.add_field(ofs_field(*info))
        return reg

    def construct_ofs_field(self, node):
        return node


ofs_safe_loader.add_constructor(
    u'tag:intel.com,2020:ofs/register',
    ofs_safe_loader.construct_ofs_register)

ofs_safe_loader.add_constructor(
    u'tag:intel.com,2020:ofs/bitfield',
    ofs_safe_loader.construct_ofs_field)


# def ofs_register_dumper(dumper, data):
#     return dumper.represent_sequence(u'!!ofs/register', data, flow_style=False)
# 
# yaml.add_representer(ofs_register, ofs_register_dumper)

class feature(object):
    def __init__(self):
        self._registers = []
        self._name = None

    def append(self, r):
        if not self._registers:
            self._name = r.name
            self._offset = r.offset
        self._registers.append(r)

    @property
    def registers(self):
        return self._registers

    @property
    def name(self):
        return self._name

    @property
    def offset(self):
        return self._offset


def features(data):
    next_dfh = 0x0
    f = feature()
    for r in data:
        if r.offset == next_dfh:
            next_dfh += ((r.default >> 16) & 0x00FFFFFF)
            if r.offset != 0x0:
                yield f
            f = feature()
        f.append(r)
    yield f



def make_struct(name, fields):
    le_fields = sorted(fields, key=ofs_field.lo)
    ct = ctypes.c_uint64 if le_fields[-1].hi() > 31 else ctypes.c_uint32
    stype = type('{}_fields'.format(name),
                 (ctypes.LittleEndianStructure,),
                 dict(_fields_=[(f.name, ct, f.width()) for f in le_fields]))
    utype = type(str(name),
                 (ctypes.Union,),
                 dict(_fields_=[('value', ct),
                                ('bits', stype)]))
    u = utype()
    for f in le_fields:
        setattr(u.bits, f.name, f.default)
    return u


def dump(inp, outp, format='bin', pad=None):
    registers = yaml.load(inp, Loader=ofs_safe_loader)
    data = bytearray()

    offset = 0
    for r in registers:
        if pad is not None:
            delta = r.offset - offset
            data.extend(bytearray([pad]*delta))
            offset = r.offset
        if r.fields:
            st = make_struct(r.name, r.fields)
            value = st.value
            size = ctypes.sizeof(st)
        else:
            value = r.default
            size = 8
        fmt = '<Q' if size > 4 else '<I'
        data.extend(struct.pack(fmt, value))
        offset += size
    outp.write(data)


class field_node(yaml.YAMLObject):
    def __init__(self, fields):
        self.f = [f.to_list() for f in fields]


def gen_fields(protocol):
    if protocol.startswith('dfh'):
        fields =  [ofs_field('feature_type',      [63, 60], 'ro', 0x5, 'Feature Type'),
                   ofs_field('dfh_version',       [59, 52], 'ro', 0x0, 'DFH Version'),
                   ofs_field('feature_minor_rev', [51, 48], 'ro', 0x0, 'Feature Minor Revision'),
                   ofs_field('reserved41',        [47, 41], 'ro', 0x0, 'Reserved'),
                   ofs_field('eol',               [40]    , 'ro', 0x0, 'End of List'),
                   ofs_field('next_offset',       [39, 16], 'ro', 0x1000, 'Next DFH Offset'),
                   ofs_field('feature_major_ver', [15, 12], 'ro', 0x0, 'User defined'),
                   ofs_field('feature_id',        [11,  0], 'ro', 0x0, 'Feature ID')]
        if protocol == 'dfh1':
            fields[1].default = 1
        return fields
    else:
        return []


class flow_list(yaml.YAMLObject):
    yaml_tag = '!!seq'

    def __init__(self, data):
        self.data = data

    @classmethod
    def to_yaml(cls, dumper, data):
        return dumper.represent_sequence(dumper.DEFAULT_SEQUENCE_TAG, data.data, True)


class register_node(yaml.YAMLObject):
    yaml_tag = u'tag:intel.com,2020:ofs/register'

    def __init__(self, data):
        self.data = data

    @classmethod
    def to_yaml(cls, dumper, data):
        return dumper.represent_sequence(cls.yaml_tag, data.data)

def generate(rng, outp, protocol='dfh0'):
    registers = []
    for offset in range(*rng):
        r = ofs_register(protocol, '0x{:04x}'.format(offset), '0x0000', 'header type "{}"'.format(protocol))
        r.fields = gen_fields(protocol)
        bf = make_struct(protocol, r.fields)
        r.default = bf.value
        registers.append(register_node([flow_list(r.header()),
                                        [flow_list(f.to_list()) for f in r.fields]]))
    yaml.dump(registers, outp, indent=2)


def do_dump(args):
    dump(args.input, args.output or sys.stdout, args.format, args.pad)


def do_gen(args):
    generate(args.range, args.output or sys.stdout, args.protocol)


def hex_int(inp):
    try:
        return int(inp, 0)
    except:
        print('error parsing int: {}'.format(inp))


def hrange(inp):
    r = inp.split(':')
    if len(r) < 2:
        raise SystemExit('range must in be "begin:end[:step]" format')

    b = [int(v, 0) for v in r]
    if len(b) < 3:
        b.append(0x1000)
    return tuple(b)


def main(args=None):
    parser = argparse.ArgumentParser()
    subs = parser.add_subparsers()
    dump_parser = subs.add_parser('dump')
    dump_parser.add_argument('input', type=argparse.FileType('r'))
    dump_parser.add_argument('-o', '--output', type=argparse.FileType('wb'))
    dump_parser.add_argument('--pad', type=hex_int)
    dump_parser.add_argument('--format', choices=['bin'], default='bin')
    dump_parser.set_defaults(func=do_dump)

    gen_parser = subs.add_parser('gen')
    gen_parser.add_argument('range', type=hrange)
    gen_parser.add_argument('-o', '--output', type=argparse.FileType('w'))
    gen_parser.add_argument('--protocol', choices=['dfh0', 'dfh1'])
    gen_parser.set_defaults(func=do_gen)

    args = parser.parse_args(args=args)
    args.func(args)



if __name__ == '__main__':
    main()
