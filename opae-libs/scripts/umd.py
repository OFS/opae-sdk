#! /usr/bin/env python3
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
import sys

from ofs_parse import parse as ofs_parse


class file_writer:
    pass


class fn_visitor(ast.NodeVisitor):
    def __init__(self, driver):
        self.driver = driver

    def visit_FunctionDef(self, node):
        return c_func(self.driver, node)


class c_visitor(ast.NodeVisitor):
    _aliases = {}

    @classmethod
    def c_alias(cls, fn):
        def wrapper(self, *args, **kwargs):
            return fn(self, *args, **kwargs)

        cls._aliases[fn.__name__] = wrapper
        return wrapper

    def get_alias(self, name):
        return self._aliases.get(name)

    def get_type(self, name):
        if name.endswith('_ptr'):
            return f'{name.replace("_ptr", "*")}'
        return name

    def visit_Add(self, node):
        return '+'

    def visit_Sub(self, node):
        return '-'

    def visit_Mult(self, node):
        return '*'

    def visit_Div(self, node):
        return '/'

    def visit_Not(self, node):
        return '!'

    def visit_Lt(self, node):
        return '<'

    def visit_Gt(self, node):
        return '>'

    def visit_Eq(self, node):
        return '=='

    def visit_Num(self, node):
        return str(node.n)

    def visit_Str(self, node):
        return f'"{node.s}"'

    def visit_Index(self, node):
        return self.visit(node.value)

    def visit_Constant(self, node):
        if isinstance(node.value, str):
            return f'"{node.value}"'
        return str(node.value)

    def visit_Name(self, node):
        return node.id


class decl_visitor(c_visitor):
    def __init__(self, target):
        self.target = target

    def visit_Name(self, node):
        _type = self.get_type(node.id)
        return f'{_type} {self.target}'

    def visit_Call(self, node):
        if node.func.id == 'ptr':
            return f'{node.args[0].id} *{self.target}'

    def visit_Subscript(self, node):
        return f'{node.value.id} {self.target}[{self.visit(node.slice)}]'


class scope_visitor(c_visitor):
    def __init__(self, driver):
        self.driver = driver

    @c_visitor.c_alias
    def ref(self, name, cast=None):
        if cast:
            return f'({self.get_type(cast)})&({name})'
        return f'&{name}'

    def visit_arguments(self, args):
        return [decl_visitor(a.arg).visit(a.annotation) for a in args.args]

    def visit_If(self, node):
        new_node = c_if(node, self.driver, self)
        for s in node.body:
            new_node.append(self.visit(s))
        return new_node

    def visit_While(self, node):
        new_node = c_while(node, self.driver, self)
        for s in node.body:
            new_node.append(self.visit(s))
        return new_node

    def visit_For(self, node):
        new_node = c_for(node, self.driver, self)
        for s in node.body:
            new_node.append(self.visit(s))
        return new_node

    def visit_Expr(self, node):
        return self.visit(node.value)

    def visit_Assign(self, node):
        lhs = self.visit(node.targets[0])
        rhs = self.visit(node.value)
        return c_code(f'{lhs} = {rhs}')

    def visit_AugAssign(self, node):
        lhs = self.visit(node.target)
        op = self.visit(node.op)
        rhs = self.visit(node.value)
        return c_code(f'{lhs} {op}= {rhs}')

    def visit_AnnAssign(self, node):
        dv = decl_visitor(node.target.id)
        decl = dv.visit(node.annotation)
        if node.value:
            return c_code(f'{decl} = {self.visit(node.value)}')
        return c_code(decl)

    def visit_Return(self, node):
        return c_code(f'return {self.visit(node.value)}')

    def visit_Call(self, node):
        args = [self.visit(a) for a in node.args]
        alias = self.get_alias(node.func.id)
        if alias:
            return alias(self, *args)
        fn = self.driver.resolve_function(node.func.id)
        if fn.startswith(self.driver.name):
            args.insert(0, 'drv')
        args = ', '.join(args)
        return c_code(f'{fn}({args})')

    def visit_BinOp(self, node):
        lhs = self.visit(node.left)
        op = self.visit(node.op)
        rhs = self.visit(node.right)
        return f'{lhs}{op}{rhs}'

    def visit_UnaryOp(self, node):
        return f'{self.visit(node.op)}{self.visit(node.operand)}'

    def visit_BoolOp(self, node):
        raise NotImplementedError("help")

    def visit_Compare(self, node):
        lhs = self.visit(node.left)
        op = self.visit(node.ops[0])
        rhs = self.visit(node.comparators[0])
        return f'{lhs} {op} {rhs}'

    def visit_Attribute(self, node):
        if node.value.id in self.driver.reg_names:
            return f'drv->r_{node.value.id}->f_{node.attr}'
        return f'{node.value.id}.{node.attr}'

    def visit_Name(self, node):
        if node.id in self.driver.reg_names:
            return f'drv->r_{node.id}->value'
        return node.id

    def visit_Starred(self, node):
        return f'*{self.visit(node.value)}'

    def visit_USub(self, node):
        return '-'

    def visit_Subscript(self, node):
        return f'{node.value.id}[{self.visit(node.slice)}]'


class c_node(object):
    def __init__(self, node):
        self.node = node


class c_code(c_node):
    def __init__(self, code):
        self.code = code

    def write_code(self):
        return self.code

    def __str__(self):
        return self.code


class c_block(c_node):
    def __init__(self, node, driver, sv: scope_visitor = None):
        super().__init__(node)
        self.driver = driver
        self.body = []
        self.sv = sv

    def append(self, n):
        self.body.append(n)

    def write_header(self):
        raise NotImplementedError(f'{self} must implment write_header')

    def write_body(self, writer: file_writer, indent=1):
        spaces = '\t'*indent
        for s in self.body:
            if isinstance(s, c_block):
                writer.writeline(f'{spaces}{s.write_header()} {{')
                s.write_body(writer, indent+1)
                writer.writeline(f'{spaces}}}')
            else:
                writer.writeline(f'{spaces}{s.write_code()};')


class c_conditional_block(c_block):
    def __init__(self, node, driver, sv: scope_visitor = None):
        super().__init__(node, driver)
        self.sv = sv
        self.cond = self.sv.visit(node.test)

    def write_header(self):
        return f'{self.keyword} ({self.cond})'


class c_if(c_conditional_block):
    keyword = 'if'


class c_while(c_conditional_block):
    keyword = 'while'


class c_for(c_block):
    def write_header(self):
        target = self.sv.visit(self.node.target)
        if self.node.iter.func.id in ['range', 'arange']:
            args = self.node.iter.args
            if len(args) == 1:
                start = 0
                end = self.sv.visit(args[0])
            else:
                start = self.sv.visit(args[0])
                end = self.sv.visit(args[1])
            if len(args) < 3:
                step = f'{target}++'
            else:
                step = self.sv.visit(args[2])
                if step > 0:
                    step = f'+={step}'
                else:
                    step = f'-={step}'
                step = f'{target}{step}'
            for_range = f'{target} = {start}; i < {end}; {step}'
        elif self.node.iter.func.id == 'c':
            for_range = self.node.iter.args[0].s
        return f'for ({for_range})'


class c_func(c_block):
    def __init__(self, driver, node):
        super().__init__(node, driver)
        self.driver = driver
        self.name = node.name
        self.returns = node.returns.id if node.returns else 'void'
        self.sv = scope_visitor(driver)

    def write_header(self):
        params = [f'{self.driver.name} *drv'] + self.sv.visit(self.node.args)
        params = ', '.join(params)
        return f'{self.returns} {self.driver.name}_{self.name}({params})'

    def visit_tree(self):
        for s in self.node.body:
            if isinstance(s, ast.Pass):
                break
            self.body.append(self.sv.visit(s))


class driver_writer(file_writer):
    def __init__(self, name, fp, registers=[], functions=[]):
        self.name = name
        self.fp = fp
        self.set_context(registers, functions)

    def set_context(self, registers, functions):
        self.registers = registers
        self.functions = functions
        self.reg_names = [r.name for r in registers]
        self.fn_names = [f.name for f in functions]

    def resolve_function(self, fn_name):
        if fn_name in self.fn_names:
            return f'{self.name}_{fn_name}'
        return fn_name

    def resolve_name(self, name):
        if name in self.reg_names:
            return f'drv->r_{name}'
        return name

    def writefile(self):
        for fn in self.functions:
            self.writeline(f'{fn.write_header()};')

        for fn in self.functions:
            fn.visit_tree()
            self.writeline(f'{fn.write_header()}')
            self.writeline('{')
            fn.write_body(self)
            self.writeline('}')

    def writeline(self, text):
        self.fp.write(f'{text}\n')


def get_functions(code, driver):
    functions = []
    try:
        tree = ast.parse(code)
    except SyntaxError as err:
        raise SystemExit(f'syntax err: {err.text}')
    else:
        fv = fn_visitor(driver)
        for node in tree.body:
            functions.append(fv.visit(node))
    return functions


def writefile(args):
    data = ofs_parse(args.file)
    name = data['name']
    registers = data['registers']
    try:
        tree = ast.parse(data.get('api', ''))
    except SyntaxError as err:
        raise SystemExit(f'syntax err: {err.text}')

    wr = driver_writer(name, args.output)
    functions = []
    for f in tree.body:
        functions.append(fn_visitor(wr).visit(f))
    wr.set_context(registers, functions)
    wr.writefile()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('file', type=argparse.FileType('r'))
    parser.add_argument('output', type=argparse.FileType('w'),
                        nargs='?', default=sys.stdout)
    args = parser.parse_args()
    writefile(args)


if __name__ == '__main__':
    main()
