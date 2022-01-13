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
import ast
import atexit
import io
import json
import jsonschema
import os
import yaml

import umd

from collections import OrderedDict
from contextlib import contextmanager
from pathlib import Path
from tempfile import NamedTemporaryFile

try:
    from yaml import CLoader as YamlLoader
except ImportError:
    from yaml import Loader as YamlLoader


default_schema = Path(__file__).absolute().parent.joinpath('umd-schema.json')

cpp_field_tmpl = '{spaces}{pod} f_{name} : {width};'
cpp_class_tmpl = '''
#define {name}_OFFSET 0x{offset:0x}
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
c_struct_templ = '''
#define {name}_OFFSET 0x{offset:0x}
typedef union _{name} {{
  {pod} value;
  struct {{
{fields}
  }};
}} {name};

'''

driver_struct_templ = '''
typedef struct _{driver} {{
  fpga_handle handle;
{members}
}} {driver};

int {driver}_init({driver} *{var}, fpga_handle h)
{{
  uint8_t *ptr = 0;
  fpga_result res = fpgaMapMMIO(h, 0, (uint64_t**)&ptr);
  if (res) {{
    return res;
  }}
  {var}->handle = h;
{inits}
  return 0;
}}

'''

templates = {'c': c_struct_templ,
             'cpp': cpp_class_tmpl}


class ofs_field(object):
    def __init__(self, name, bits, access, default, description):
        self.name = name
        self.bits = bits
        self.access = access
        self.default = default
        self.description = description

    def max(self):
        return max(self.bits)

    def min(self):
        return min(self.bits)

    def __repr__(self):
        return (f'{self.name}, {self.bits}, {self.access}, {self.default}, '
                f'{self.description}')

    def to_cpp(self, writer):
        writer.write(cpp_field_tmpl.format(**vars(self)))
        writer.write('\n')
        for line in self.description.split('\n'):
            writer.write(f'{self.spaces}// {line.strip()}\n')


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

    def to_structure(self, tmpl,  writer):
        for line in self.description.split('\n'):
            writer.writeline(f'// {line}')
        writer.write(tmpl.lstrip().format(**vars(self)))


def write_temp(local_file: Path) -> Path:
    # TODO: Hack for now until we can formalize schemas in a public URL
    with local_file.open('r') as inp:
        local_data = json.load(inp)
    with NamedTemporaryFile('w',
                            delete=False,
                            prefix=f'{local_file.stem}-',
                            suffix='.json') as out:
        outfile = Path(out.name)
        local_data['$id'] = outfile.as_uri()
        json.dump(local_data, out)
        atexit.register(outfile.unlink)
        return outfile


def use_local_refs(schema):
    # TODO: Hack for now until we can formalize schemas in a public URL
    cwd = Path(__file__).parent.absolute()
    for k, v in schema.items():
        if k in ['$ref', '$id']:
            if v.startswith('#'):
                continue
            comps = jsonschema.validators.urlsplit(v)
            local_file = cwd.joinpath(Path(comps.path).name).absolute()
            if local_file.exists():
                schema[k] = write_temp(local_file).as_uri()
        elif isinstance(v, dict):
            schema[k] = use_local_refs(v)
    return schema


def parse(fp, schemafile=None, local_refs=False):
    data = yaml.load(fp, Loader=YamlLoader)
    if schemafile:
        with open(schemafile, 'r') as schema_fp:
            schema = yaml.safe_load(schema_fp)
            try:
                if local_refs:
                    schema = use_local_refs(schema)
                jsonschema.validate(data, schema)
            except jsonschema.ValidationError as err:
                print(f'input file "{fp.name}"  does not follow schema, error:'
                      f'{err}')
                raise
            except KeyError as err:
                print(f'invalid/incomplete schema({fp.name}): {err}')
                raise
    registers = data.get('registers')
    data['registers'] = [ofs_register(*r[0], r[1] if len(r) > 1 else ())
                         for r in registers]
    return data


class ofs_driver_writer(object):
    def __init__(self, data):
        self.data = data

    def resolve_function(self, fn_name):
        if fn_name in self.fn_names:
            return f'{self.name}_{fn_name}'
        return fn_name

    def resolve_name(self, name):
        if name in self.reg_names:
            return f'drv->r_{name}'
        return name

    @property
    def fn_names(self):
        return [fn.name for fn in self.functions]

    @property
    def reg_names(self):
        return [r.name for r in self.registers]

    def api_functions(self, code):
        functions = OrderedDict()
        try:
            tree = ast.parse(code)
        except SyntaxError as err:
            print(f'sytax error: {err.text}, line: {err.lineno}')
        else:
            lines = code.split('\n')
            for node in tree.body:
                if isinstance(node, ast.FunctionDef):
                    fn = {}
                    args = []
                    body = self.get_body_text(node.body, lines)
                    if body:
                        fn['body'] = body
                    if node.returns:
                        fn['return'] = node.returns.id
                    for a in node.args.args:
                        name = a.arg
                        ann = a.annotation
                        sub = ''
                        ptr = ''
                        if isinstance(ann, ast.Call) and ann.func.id == 'ptr':
                            _type = f'{ann.args[0].id}'
                            ptr = '*'
                        elif isinstance(ann, ast.Subscript):
                            _type = ann.value.id
                            sub = f'[{ann.slice.value.value}]'
                        else:
                            _type = ann.id
                        args.append(f'{_type} {ptr}{name}{sub}')
                    if args:
                        fn['args'] = args
                    functions[node.name] = fn

        return functions

    def write_header(self, output, language='c'):
        self.name = self.data['name']
        self.registers = self.data['registers']
        self.functions = umd.get_functions(self.data.get('api', ''), self)
        if not self.functions:
            return
        filepath = os.path.join(output, f'{self.name}.h')
        # with open(os.path.join(args.output, f'{name}.h'), 'w') as fp:
        with ofs_header_writer.open(filepath, 'w') as writer:
            writer.writeline(
                f'// these structures were auto-generated using {__file__}')
            writer.writeline(
                '// modification of these structures may break the software\n')
            if language == 'c':
                writer.writeline(f'#ifndef __{self.name}__')
                writer.writeline(f'#define __{self.name}__')
            elif language == 'cpp':
                writer.writeline('#pragma once')
            writer.writeline('#include <opae/fpga.h>')
            writer.writeline('#include <ofs/ofs.h>')

            if language == 'c':
                writer.writeline('\n#ifdef __cplusplus')
                writer.writeline('extern "C" {')
                writer.writeline('#endif\n')

            self.write_structures(writer, templates.get(language))
            self.write_driver(writer)
            if language == 'c':
                writer.writeline('\n#ifdef __cplusplus')
                writer.writeline('}')
                writer.writeline('#endif\n')
                writer.writeline(f'#endif //  __{self.name}__')

    def write_structures(self, fp, tmpl):
        for r in self.registers:
            r.width = 64 if max(r.fields, key=ofs_field.max).max() > 32 else 32
            r.pod = f'uint{r.width}_t'
            r.fields = declare_fields(r.pod, r.fields, 4)
            r.to_structure(tmpl, fp)

    def get_body_text(self, body, lines):
        text = io.StringIO()
        first = body[0]
        beg = first.lineno-1
        for line in lines[beg:]:
            if line[:first.col_offset].strip() == '':
                text.write(f'{line[first.col_offset:]}\n')
            else:
                break
        return text.getvalue()

    def write_function(self, fp, function_name, info={}, prototype=False):
        rtype = info.get('return', 'void')
        args = ', '.join([f'{self.name} *drv'] + info.get('args', []))
        fp.write(f'{rtype} {self.name}_{function_name}({args})')
        if prototype:
            fp.write(';\n')
        else:
            fp.write('\n{\n')
            self.write_scoped_body(fp, info['body'])
            fp.write('}\n\n')

    def write_driver(self, fp):
        members = io.StringIO()
        inits = io.StringIO()
        var = 'drv'
        for r in self.registers:
            members.write(f'  volatile {r.name} *r_{r.name};\n')
            inits.write(f'  {var}->r_{r.name} =\n')
            inits.write(f'    (volatile {r.name}*)(ptr+{r.name}_OFFSET);\n')
        fp.write(
            driver_struct_templ.format(driver=self.name,
                                       var=var,
                                       members=members.getvalue().rstrip(),
                                       inits=inits.getvalue().rstrip()))

        fp.write('\n\n// *****  function prototypes ******//\n')
        for fn in self.functions:
            fp.writeline(f'{fn.write_header()};')
            fn.visit_tree()
        fp.write('\n\n// *****  function implementations ******//\n')
        for fn in self.functions:
            if fn.body:
                fp.writeline(f'{fn.write_header()}')
                fp.writeline('{')
                fn.write_body(fp)
                fp.writeline('}\n')


class ofs_header_writer(object):
    def __init__(self, fp):
        self.fp = fp

    def close(self):
        self.fp.close()

    def write(self, text):
        self.fp.write(text)

    def writeline(self, line):
        self.write(f'{line}\n')

    @classmethod
    @contextmanager
    def open(cls, filename, *args, **kwargs):
        writer = cls(open(filename, *args, **kwargs))
        try:
            yield writer
        finally:
            writer.close()


def declare_fields(pod, fields, indent=0):
    fp = io.StringIO()
    for f in sorted(fields, key=ofs_field.min, reverse=False):
        f.spaces = ' '*indent
        f.pod = pod
        f.width = max(f.bits)-min(f.bits)+1
        f.to_cpp(fp)
        fp.write('\n')
    return fp.getvalue().rstrip()


def make_headers(args):
    data = parse(args.input, args.schema, args.use_local_refs)
    for driver in data.get('drivers', []):
        name = driver['name']
        if args.list:
            print(f'{name}.h')
        elif args.driver is None or args.driver == name:
            writer = ofs_driver_writer(driver)
            writer.write_header(args.output, args.language)
    else:
        if 'name' in data and 'registers' in data:
            writer = ofs_driver_writer(data)
            writer.write_header(args.output, args.language)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=argparse.FileType('r'))
    parsers = parser.add_subparsers()
    headers_parser = parsers.add_parser('headers')
    headers_parser.add_argument('language', choices=['c', 'cpp'])
    headers_parser.add_argument('output',
                                nargs='?',
                                default=os.getcwd())
    headers_parser.set_defaults(func=make_headers)
    headers_parser.add_argument('-l', '--list', action='store_true',
                                default=False,
                                help='only list driver names in file')
    headers_parser.add_argument('-d', '--driver',
                                help='process only this driver')
    headers_parser.add_argument('--schema',
                                default=default_schema)
    headers_parser.add_argument('--use-local-refs', action='store_true',
                                default=False,
                                help='Transform refs to local repo files')

    args = parser.parse_args()
    if not hasattr(args, 'func'):
        parser.print_usage()
        return 1
    args.func(args)


if __name__ == '__main__':
    main()
