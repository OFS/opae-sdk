#!/usr/bin/env python3
# Copyright(c) 2023, Intel Corporation
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

import json

import opae.http.constants as constants
from opae.http.version import version
from opae.http.conversion import to_json_obj


class tracks_valid_fields():
    def __init__(self):
        self.__dict__['valid_fields'] = 0

    def field_is_valid(self, f):
        valid_fields = self.__dict__['valid_fields']
        return ((valid_fields >> f) & 1) != 0

    def set_field_valid(self, f):
        self.__dict__['valid_fields'] |= (1 << f)


class fpga_properties(tracks_valid_fields):
    NUM_SLOTS_IDX = 32
    BBSID_IDX = 33
    BBSVERSION_IDX = 34

    attrs = {'num_slots': NUM_SLOTS_IDX,
             'bbs_id': BBSID_IDX,
             'bbs_version': BBSVERSION_IDX,
            }

    def __init__(self, parent):
        super().__init__()
        self.__dict__['_parent'] = parent

    def set_field_valid(self, f):
        super().set_field_valid(f)
        parent = self.__dict__['_parent']
        parent.set_field_valid(f)

    def __getattr__(self, attr):
        if attr in fpga_properties.attrs:
            if self.field_is_valid(fpga_properties.attrs[attr]):
                return self.__dict__[attr]
            else:
                raise AttributeError(attr + ' is not set on fpga_properties')

    def __setattr__(self, attr, value):
        if attr in fpga_properties.attrs:
            self.__dict__[attr] = value
            self.set_field_valid(fpga_properties.attrs[attr])
        else:
            raise AttributeError(attr + ' is not a valid fpga_properties attr')

    def to_json_obj(self):
        d = {}
        transforms = {'bbs_id': str,
                      'bbs_version': to_json_obj}
        for attr, idx in fpga_properties.attrs.items():
            if self.field_is_valid(idx):
                if attr in transforms:
                    d[attr] = transforms[attr](self.__dict__[attr])
                else:
                    d[attr] = self.__dict__[attr]
        return d

    @staticmethod
    def resolve_aliases(d):
        aliases = {'num_slots':   ['numSlots'],
                   'bbs_id':      ['bbsId'],
                   'bbs_version': ['bbsVersion'],
                  }
        c = d.copy()
        for attr, alias_list in aliases.items():
            if attr in c:
                continue
            for a in alias_list:
                if a in c:
                    c[attr] = c[a]
                    del c[a]
        return c

    @staticmethod
    def from_json_obj(jobj, parent):
        props = fpga_properties(parent)
        transforms = {'bbs_version': version.from_json_obj}
        for k, v in fpga_properties.resolve_aliases(jobj).items():
            if k in transforms:
                setattr(props, k, transforms[k](v))
            else:
                setattr(props, k, v)
        return props


class accelerator_properties(tracks_valid_fields):
    ACCELERATOR_STATE_IDX = 32
    NUM_MMIO_IDX = 33
    NUM_INTERRUPTS_IDX = 34

    attrs = {'state': ACCELERATOR_STATE_IDX,
             'num_mmio': NUM_MMIO_IDX,
             'num_interrupts': NUM_INTERRUPTS_IDX,
            }

    def __init__(self, parent):
        super().__init__()
        self.__dict__['_parent'] = parent

    def set_field_valid(self, f):
        super().set_field_valid(f)
        parent = self.__dict__['_parent']
        parent.set_field_valid(f)

    def __getattr__(self, attr):
        if attr in accelerator_properties.attrs:
            if self.field_is_valid(accelerator_properties.attrs[attr]):
                return self.__dict__[attr]
            else:
                raise AttributeError(attr + ' is not set on accelerator_properties')

    def __setattr__(self, attr, value):
        if attr in accelerator_properties.attrs:
            self.__dict__[attr] = value
            self.set_field_valid(accelerator_properties.attrs[attr])
        else:
            raise AttributeError(attr + ' is not a valid accelerator_properties attr')

    def to_json_obj(self):
        d = {}
        transforms = {'state': constants.fpga_accelerator_state_to_str}
        for attr, idx in accelerator_properties.attrs.items():
            if self.field_is_valid(idx):
                if attr in transforms:
                    d[attr] = transforms[attr](self.__dict__[attr])
                else:
                    d[attr] = self.__dict__[attr]
        return d

    @staticmethod
    def resolve_aliases(d):
        aliases = {'num_mmio':       ['numMmio'],
                   'num_interrupts': ['numInterrupts'],
                  }
        c = d.copy()
        for attr, alias_list in aliases.items():
            if attr in c:
                continue
            for a in alias_list:
                if a in c:
                    c[attr] = c[a]
                    del c[a]
        return c

    @staticmethod
    def from_json_obj(jobj, parent):
        props = accelerator_properties(parent)
        transforms = {'state': constants.fpga_accelerator_state_from_str}
        for k, v in accelerator_properties.resolve_aliases(jobj).items():
            if k in transforms:
                setattr(props, k, transforms[k](v))
            else:
                setattr(props, k, v)
        return props


class properties(tracks_valid_fields):
    MAGIC = 0x4650474150524f50

    PARENT_IDX = 0
    OBJTYPE_IDX = 1
    SEGMENT_IDX = 2
    BUS_IDX = 3
    DEVICE_IDX = 4
    FUNCTION_IDX = 5
    SOCKETID_IDX = 6
    VENDORID_IDX = 7
    DEVICEID_IDX = 8
    GUID_IDX = 9
    OBJECTID_IDX = 10
    NUM_ERRORS_IDX = 11
    INTERFACE_IDX = 12
    SUB_VENDORID_IDX = 13
    SUB_DEVICEID_IDX = 14
    HOSTNAME_IDX = 15

    attrs = {'guid': GUID_IDX,
             'parent': PARENT_IDX,
             'objtype': OBJTYPE_IDX,
             'segment': SEGMENT_IDX,
             'bus': BUS_IDX,
             'device': DEVICE_IDX,
             'function': FUNCTION_IDX,
             'socket_id': SOCKETID_IDX,
             'object_id': OBJECTID_IDX,
             'vendor_id': VENDORID_IDX,
             'device_id': DEVICEID_IDX,
             'num_errors': NUM_ERRORS_IDX,
             'interface': INTERFACE_IDX,
             'subsystem_vendor_id': SUB_VENDORID_IDX,
             'subsystem_device_id': SUB_DEVICEID_IDX,
             'hostname': HOSTNAME_IDX,
            }

    def __init__(self):
        super().__init__()
        self.__dict__['fpga'] = fpga_properties(self)
        self.__dict__['accelerator'] = accelerator_properties(self)

    def __getattr__(self, attr):
        objtype = self.__dict__.get('objtype', constants.FPGA_ACCELERATOR + 1)
        if attr in properties.attrs:
            if self.field_is_valid(properties.attrs[attr]):
                return self.__dict__[attr]
            else:
                raise AttributeError(attr + ' is not set')
        elif ((attr == 'fpga') and
              (objtype == constants.FPGA_DEVICE)):
                return self.__dict__['fpga']
        elif ((attr == 'accelerator') and
              (objtype == constants.FPGA_ACCELERATOR)):
                return self.__dict__['accelerator']
        raise AttributeError(attr + ' is not a valid property')

    def __setattr__(self, attr, value):
        objtype = self.__dict__.get('objtype', constants.FPGA_ACCELERATOR + 1)
        valid = False
        index = -1
        if attr in properties.attrs:
            valid = True
            index = properties.attrs[attr]
        elif ((attr == 'fpga') and
              (objtype == constants.FPGA_DEVICE)):
            return setattr(self.__dict__['fpga'], value)
        elif ((attr == 'accelerator') and
              (objtype == constants.FPGA_ACCELERATOR)):
            return setattr(self.__dict__['accelerator'], value)

        if valid:
            self.__dict__[attr] = value
            self.set_field_valid(index)
        else:
            raise AttributeError(attr + ' is not a valid property')

    def to_json_obj(self):
        d = {}
        d['magic'] = str(properties.MAGIC)
        d['valid_fields'] = str(self.__dict__['valid_fields'])
        transforms = {'objtype': constants.fpga_objtype_to_str,
                      'interface': constants.fpga_interface_to_str,
                      'object_id': str,
                     }
        objtype = self.__dict__.get('objtype', constants.FPGA_ACCELERATOR + 1)
        for attr, idx in properties.attrs.items():
            if self.field_is_valid(idx):
                if attr in transforms:
                    d[attr] = transforms[attr](self.__dict__[attr])
                else:
                    d[attr] = self.__dict__[attr]
        if objtype == constants.FPGA_DEVICE:
            fpga = self.__dict__['fpga'].to_json_obj()
            if len(fpga):
                d['fpga'] = fpga
        elif objtype == constants.FPGA_ACCELERATOR:
            accelerator = self.__dict__['accelerator'].to_json_obj()
            if len(accelerator):
                d['accelerator'] = accelerator
        return d

    def to_json_str(self):
        return json.dumps(self.to_json_obj())

    @staticmethod
    def resolve_aliases(d):
        aliases = {'guid':                [],
                   'parent':              [],
                   'objtype':             [],
                   'segment':             [],
                   'bus':                 [],
                   'device':              [],
                   'function':            [],
                   'socket_id':           ['socketId'],
                   'object_id':           ['objectId'],
                   'vendor_id':           ['vendorId'],
                   'device_id':           ['deviceId'],
                   'num_errors':          ['numErrors'],
                   'interface':           [],
                   'subsystem_vendor_id': ['subsystemVendorId'],
                   'subsystem_device_id': ['subsystemDeviceId'],
                   'hostname':            [],
                  }
        c = d.copy()
        for attr, alias_list in aliases.items():
            if attr in c:
                continue
            for a in alias_list:
                if a in c:
                    c[attr] = c[a]
                    del c[a]
        return c

    @staticmethod
    def from_json_obj(jobj):
        props = properties()
        props.__dict__['magic'] = properties.MAGIC

        valid_fields = jobj.get('valid_fields')
        if valid_fields is None:
            valid_fields = jobj.get('validFields')
        props.__dict__['valid_fields'] = int(valid_fields)

        objtype = jobj.get('objtype', constants.FPGA_ACCELERATOR + 1)
        if objtype in [constants.FPGA_DEVICE, constants.FPGA_ACCELERATOR]:
            setattr(props, 'objtype', constants.fpga_objtype_from_str(objtype))

        transforms = {'objtype': constants.fpga_objtype_from_str,
                      'interface': constants.fpga_interface_from_str,
                      'object_id': int,
                     }
        skip = ['magic', 'valid_fields', 'validFields']
        for k, v in properties.resolve_aliases(jobj).items():
            if k in skip:
                continue
            if k == 'fpga':
                props.__dict__['fpga'] = fpga_properties.from_json_obj(v, props)
            elif k == 'accelerator':
                props.__dict__['accelerator'] = accelerator_properties.from_json_obj(v, props)
            else:
                if k in transforms:
                    setattr(props, k, transforms[k](v))
                else:
                    setattr(props, k, v)
        return props
