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
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
import argparse
import io
import sys
import yaml

cpp_field_tmpl = '{spaces}{pod} f_{name} : {width};'
cpp_class_tmpl = '''
union {name} {{
  enum {{
    offset = 0x{offset:0x}
  }};
  {name}() {{}}
  {name}({pod} v) : value(v) {{}}
  {pod} value;
  struct {{
{fields}
  }};
}};

'''


class ofs_field(object):
    def __init__(self, name, bits, access, default, description):
        self.name = name
        self.bits = bits
        self.access = access
        self.default = default
        self.description = description

    def max(self):
        return max(self.bits)

    def __repr__(self):
        return (f'{self.name}, {self.bits}, {self.access}, {self.default}, '
                f'{self.description}')

    def to_cpp(self, writer):
        writer.write(cpp_field_tmpl.format(**vars(self)))
        writer.write('\n')
        for line in self.description.split('\n'):
            writer.write(f'{self.spaces}// {line}\n')


class ofs_register(object):
    def __init__(self, name, offset, default, description, fields=[]):
        self.name = name
        self.offset = offset
        self.default = default
        self.description = description
        self.fields = [ofs_field(*f) for f in fields]

    def __repr__(self):
        return (f'{self.name}, 0x{self.offset:0x}, 0x{self.default:0x}, '
                f'{self.description}, {self.fields}')

    def to_cpp(self, writer):
        for line in self.description.split('\n'):
            writer.write(f'// {line}\n')
        writer.write(cpp_class_tmpl.lstrip().format(**vars(self)))


class RegisterTag(yaml.YAMLObject):
    @classmethod
    def from_yaml(cls, loader, node):
        register_meta = loader.construct_sequence(node.value[0])
        register_fields = tuple(map(loader.construct_sequence,
                                    node.value[1].value))
        return ofs_register(*register_meta, register_fields)


yaml.CLoader.add_constructor(u'tag:intel.com,2020:ofs/register',
                             RegisterTag.from_yaml)


def parse(fp):
    return yaml.load(fp, Loader=yaml.CLoader)


def fields_to_cpp(pod, fields, indent=0):
    fp = io.StringIO()
    for f in fields:
        f.spaces = ' '*indent
        f.pod = pod
        f.width = max(f.bits)-min(f.bits)+1
        f.to_cpp(fp)
        fp.write('\n')
    return fp.getvalue()


def to_cpp(args):
    data = parse(args.input)
    fp = args.output
    fp.write('#pragma once\n')
    fp.write(f'// these structures were auto-generated using {__file__}\n')
    fp.write('// modification of these structures may break the software\n\n')
    for r in data.get('registers'):
        r.width = 64 if max(r.fields, key=ofs_field.max).max() > 32 else 32
        r.pod = f'uint{r.width}_t'
        r.fields = fields_to_cpp(r.pod, r.fields, 4)
        r.to_cpp(fp)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=argparse.FileType('r'))
    parsers = parser.add_subparsers()
    cpp_parser = parsers.add_parser('cpp')
    cpp_parser.add_argument('output', type=argparse.FileType('w'),
                            nargs='?',
                            default=sys.stdout)
    cpp_parser.set_defaults(func=to_cpp)
    args = parser.parse_args()
    if not hasattr(args, 'func'):
        parser.print_usage()
        return 1
    args.func(args)


if __name__ == '__main__':
    main()
