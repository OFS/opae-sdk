# Copyright(c) 2017, Intel Corporation
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
import ctypes
import logging


class f_value(object):
    def __init__(self, key, value):
        self._key = key
        self._value = value

    def name(self):
        return self._key

    def value(self):
        return self._value

    def __str__(self):
        return self._key

    def __repr__(self):
        return self._key

    def __int__(self):
        return int(self._value)


class f_list(object):
    def __init__(self, *args, **kwargs):
        self._list = list(*args, **kwargs)

    def __repr__(self):
        return repr(self._list)

    def __str__(self):
        return str(self._list)

    def __contains__(self, fv):
        return str(fv) in self._list

    def __iter__(self):
        return iter(self._list)


class f_enum(object):
    def __init__(self, *args, **kwargs):
        self._name = kwargs.pop('name', 'csr')
        self._dict = dict()
        for a in args:
            if isinstance(a, dict):
                self._dict.update(a)
        self._dict.update(kwargs)

    def name(self):
        return self._name

    def __contains__(self, fv):
        return fv.name() in self._dict

    def __call__(self, fname):
        value = self._dict.get(
            fname, self._dict.get(fname.replace('_', '-'), 0))
        return f_value(fname, value)

    def keys(self):
        return f_list(sorted(self._dict.keys()))

    def __getitem__(self, key):
        return self._dict[key]


class csr(object):
    _bits_ = []
    _types_ = {}
    _offset_ = 0x0000
    _enums_ = []

    def __init__(self, value=0, offset=None, **kwargs):
        self._offset = self._offset_ if offset is None else offset
        type_name = self.__class__.__name__
        if type_name in self._types_:
            union_t = self._types_[type_name]
        else:
            if self._bits_ and sorted(
                    self._bits_, key=lambda b: b[1])[-1][1][0] >= 32:
                c_inttype = ctypes.c_ulong
                self._width = 64
            else:
                c_inttype = ctypes.c_uint
                self._width = 32

            fields = []
            next_bit = 0
            for b in sorted(self._bits_, key=lambda b: b[1]):
                field_name = b[0]
                field_bits = b[1]
                hi = field_bits[0]
                lo = field_bits[1]
                if lo > next_bit:
                    reserved_name = '{}_reserved{}'.format(type_name, next_bit)
                    reserved_width = lo - next_bit
                    fields.append((reserved_name, c_inttype, reserved_width))
                next_bit = hi + 1
                field_width = hi - lo + 1
                fields.append((field_name, c_inttype, field_width))

            bit_structure_t = type(
                '{}_bits_t'.format(type_name),
                (ctypes.LittleEndianStructure,),
                {'_fields_': fields})
            union_t = type(
                type_name, (ctypes.Union,), {
                    '_fields_': [
                        ('bits', bit_structure_t),
                        ('value', c_inttype)]})
            self._types_[type_name] = union_t
        self._union = union_t()
        width = kwargs.pop('width', None)
        if width == 32:
            self._c_inttype = ctypes.c_uint
            self._width = 32
        elif width == 64:
            self._c_inttype = ctypes.c_ulong
            self._width = 64
        else:
            if union_t.value.size == 4:
                self._c_inttype = ctypes.c_uint
            else:
                self._c_inttype = ctypes.c_ulong
        self.reset_value(value)

    def __getitem__(self, key):
        return getattr(self._union.bits, key)

    def __setitem__(self, key, value):
        setattr(self._union.bits, key, value)

    def __or__(self, value):
        self._union.value |= value
        return self

    def __and__(self, value):
        self._union.value &= value
        return self

    def offset(self):
        return self._offset

    def width(self):
        return self._width

    def value(self):
        return self._union.value

    def set_value(self, value):
        self._union.value = value

    def reset_value(self, value=0):
        self._union.value = value

    def fields(self):
        return self._union.bits._fields_

    def to_c(self):
        def c_name(v):
            return 'uint64_t' if v is ctypes.c_ulong else 'uint32_t'
        code = 'struct {}{{\n'.format(self.__class__.__name__)

        for f in self.fields():
            code += '    {type} {name} : {width};\n'.format(
                type=c_name(f[0]), name=f[0], width=f[2])
        code += '};\n'
        return code

    def __repr__(self):
        r = "Raw Value: {}".format("0x{:08x}".format(self.value()))
        for bitfield in self._bits_:
            if self[bitfield[0]]:
                r += "\n\t{:32} : 0x{:02x}".format(
                    bitfield[0], self[bitfield[0]])
        return r

    def to_dict(self):
        data = dict([(b[0], self[b[0]]) for b in self._bits_ if self[b[0]]])
        return data

    def write(self, handle, **kwargs):
        if kwargs:
            value = kwargs.pop('value', 0)
            self.reset_value(value)
            for k, v in kwargs.iteritems():
                try:
                    self[k] = int(v)
                except BaseException:
                    logging.warn("Unrecognized field %s in CSR", k)
        if self._c_inttype is ctypes.c_uint:
            write_method = handle.write_csr32
        else:
            write_method = handle.write_csr64
        write_method(self.offset(), self.value())
