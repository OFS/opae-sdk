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
import io
import os
import re
import yaml

from contextlib import contextmanager

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
  uint64_t *ptr = 0;
  fpga_result res = fpgaMapMMIO(h, 0, &ptr);
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


class RegisterTag(yaml.YAMLObject):
    @classmethod
    def from_yaml(cls, loader, node):
        register_meta = loader.construct_sequence(node.value[0])
        fields = node.value[1].value if len(node.value) > 1 else ()
        register_fields = tuple(map(loader.construct_sequence, fields))
        return ofs_register(*register_meta, register_fields)


yaml.CLoader.add_constructor(u'tag:intel.com,2020:ofs/register',
                             RegisterTag.from_yaml)


def parse(fp):
    return yaml.load(fp, Loader=yaml.CLoader)


class ofs_driver_writer(object):
    def __init__(self, data):
        self.data = data

    def write_header(self, output, language='c'):
        self.name = self.data['name']
        self.registers = self.data['registers']
        self.functions = self.data.get('functions', {})
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
            writer.writeline('#include "ofs_primitives.h"')

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

    def find_call(self, node):
        if isinstance(node, ast.Call):
            return [node]
        if isinstance(node, ast.Return) or isinstance(node, ast.Expr):
            return self.find_call(node.value)
        if isinstance(node, ast.Compare) or isinstance(node, ast.BinOp):
            calls = self.find_call(node.left)
            if isinstance(node, ast.Compare):
                for c in node.comparators:
                    calls.extend(self.find_call(c))
            else:
                calls.extend(self.find_call(node.right))
            return calls
        return []

    def annotation_to_c(self, node):
        if isinstance(node, ast.Subscript):
            return node.value.id, node.slice.value.value
        return node.id, None

    def registers_names(self):
        return [r.name for r in self.registers]

    def c_convert(self, body, node):
        def arg_repl(m):
            fn = m.group(1)
            args = m.group(2)
            if m.group(2) and m.group(2).strip():
                return f'{self.name}_{fn}(drv, {args})'
            return f'{self.name}_{fn}(drv)'
        # lines = body.split('\n')
        line = ast.get_source_segment(body, node)

        for call_node in self.find_call(node):
            fn_name = call_node.func.id
            if fn_name in self.functions:
                line = re.sub(f'({fn_name})\\((.*?)\\)', arg_repl, line)
        return self.ofs_resolve(node, self._c_convert(line))

    def ofs_resolve(self, node, line):
        if node is None:
            return line
        if isinstance(node, ast.Call) and node.func.id == 'ofs_addr':
            func = node.func.id
            arg0 = node.args[0].id
            if func == 'ofs_addr' and arg0 in self.registers_names():
                arg1 = node.args[1].id
                line = re.sub(f'ofs_addr\\(\\s*({arg0})\\s*,\\s*({arg1})\\s*\\)', # noqa
                              r'(\2*)drv->r_\1', line)
                return line
        if isinstance(node, ast.AnnAssign) or isinstance(node, ast.Assign):
            # line = self.ofs_resolve(node.target, line)
            line = self.ofs_resolve(node.value, line)
            return line
        if isinstance(node, ast.BinOp):
            line = self.ofs_resolve(node.left, line)
            line = self.ofs_resolve(node.right, line)
            return line
        if isinstance(node, ast.Name) and node.id in self.registers_names():
            line = line.replace(node.id, f'drv->r_{node.id}->value')
        return line

    def find_registers(self, node):
        if node is None:
            return []
        if isinstance(node, ast.BinOp):
            lhs = self.find_registers(node.left)
            rhs = self.find_registers(node.right)
            return lhs + rhs
        value_id = getattr(node, 'id', None)
        if value_id and value_id in self.registers_names():
            return [value_id]
        return []

    def _c_convert(self, line):
        p = re.compile(r'(.*?)(?:(?P<r>\w+)\.(?P<f>\w+))(.*)')
        m = p.match(line)
        if m:
            line = m.expand(r'\1drv->r_\2->f_\3\4')
        f = re.compile(r'(\w+)\((.*?)\)')
        m = f.match(line)
        if m and m.group(1) in self.functions:
            line = f.sub(f'{self.name}_\\1(drv, \\2)', line)
        for py_syntax, c_syntax in [('not ', '!'),
                                    (' and ', ' && '),
                                    ('True', 'true'),
                                    ('False', 'false')]:
            line = line.replace(py_syntax, c_syntax)
        return line

    def write_scoped_body(self, fp, body, indent=1):
        spaces = '  '*indent
        body = re.sub(r'#(.*)', r'__cmt__("\1")', body)
        parsed = ast.parse(body)
        lines = body.strip().split('\n')
        for s in parsed.body:
            if isinstance(s, ast.Expr) and isinstance(s.value, ast.Call):
                line = self.c_convert(body, s)
                if s.value.func.id == '__cmt__':
                    line = re.sub(r'__cmt__\("(.*?)"\)', r'//\1', line)
                    fp.writeline(f'{spaces}{line}')
            elif type(s) in [ast.If, ast.While, ast.For]:
                if isinstance(s, ast.For) and s.iter.func.id.endswith('range'):
                    i = s.target.id
                    _min = 0 if len(s.iter.args) == 1 else s.iter.args[0].id
                    _max = s.iter.args[-1].id
                    test = f'{i} = {_min}; i < {_max}; ++i'
                    keyword = 'for'
                else:
                    test = self.c_convert(body, s.test)
                    keyword = 'if' if isinstance(s, ast.If) else 'while'
                fp.write(f'{spaces}{keyword} ({test}) {{\n')
                inner = io.StringIO()
                first = s.body[0]
                beg = first.lineno-1
                for line in lines[beg:]:
                    if line[:first.col_offset].strip() == '':
                        inner.write(f'{line[first.col_offset:]}\n')
                    else:
                        break
                self.write_scoped_body(fp, inner.getvalue(), indent+1)
                fp.write(f'{spaces}}}\n')
            else:
                line = self.c_convert(body, s)
                if isinstance(s, ast.AnnAssign):
                    _type, size = self.annotation_to_c(s.annotation)
                    _var = s.target.id
                    if _type.endswith('_ptr'):
                        _type = _type.replace('_ptr', '')
                        line = re.sub(
                            f'({_var}):\\s*({_type})(?:_ptr)([{size}])?',
                            r'\2 *\1\3', line
                        )
                    else:
                        line = re.sub(
                            f'({_var}):\\s*({_type})([{size}])?',
                            r'\2 \1\3', line
                        )
                    # if _type.endswith('_ptr'):
                    #     _type = _type.rstrip('_ptr')
                    #     _var = f'\\*{_var}'
                fp.write(f'{spaces}{line};\n')

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

        impls = []
        fp.write('\n\n// *****  function prototypes ******//\n')
        for k, v in self.functions.items():
            if v and 'body' in v:
                impls.append((k, v))
            self.write_function(fp, k, v or {}, prototype=True)
        fp.write('\n\n// *****  function implementations ******//\n')
        for k, v in sorted(impls):
            self.write_function(fp, k, v or {})


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
    data = parse(args.input)
    for driver in data.get('drivers', []):
        name = driver['name']
        if args.list:
            print(f'{name}.h')
        elif args.driver is None or args.driver == name:
            writer = ofs_driver_writer(driver)
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

    args = parser.parse_args()
    if not hasattr(args, 'func'):
        parser.print_usage()
        return 1
    args.func(args)


if __name__ == '__main__':
    main()
