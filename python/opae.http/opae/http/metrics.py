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

from opae.http.conversion import to_json_obj
import opae.http.constants as constants


class fpga_metric_info():

    attrs = {
             'metric_num':      ['metricNum'],
             'metric_guid':     ['metricGuid'],
             'qualifier_name':  ['qualifierName'],
             'group_name':      ['groupName'],
             'metric_name':     ['metricName'],
             'metric_units':    ['metricUnits'],
             'metric_datatype': ['metricDatatype'],
             'metric_type':     ['metricType'],
            }

    def __getattr__(self, attr):
        if attr in fpga_metric_info.attrs:
            return self.__dict__[attr]
        raise AttributeError(attr + ' is not a valid fpga_metric_info attr')

    def __setattr__(self, attr, value):
        if attr in fpga_metric_info.attrs:
            self.__dict__[attr] = value
        else:
            raise AttributeError(attr + ' is not an fpga_metric_info attr')

    def to_json_obj(self):
        d = {}
        transforms = {'metric_num': str,
                      'metric_datatype': constants.fpga_metric_datatype_to_str,
                      'metric_type': constants.fpga_metric_type_to_str,
                     }
        for attr in fpga_metric_info.attrs:
            if attr in transforms:
                d[attr] = transforms[attr](self.__dict__[attr])
            else:
                d[attr] = self.__dict__[attr]
        return d

    def to_json_str(self):
        return json.dumps(self.to_json_obj())

    @staticmethod
    def resolve_aliases(d):
        c = d.copy()
        for attr, aliases in fpga_metric_info.attrs.items():
            if attr in c:
                continue
            for a in aliases:
                if a in c:
                    c[attr] = c[a]
                    del c[a]
        return c

    @staticmethod
    def from_json_obj(jobj):
        info = fpga_metric_info()
        transforms = {'metric_num': int,
                      'metric_datatype': constants.fpga_metric_datatype_from_str,
                      'metric_type': constants.fpga_metric_type_from_str,
                     }
        for k, v in fpga_metric_info.resolve_aliases(jobj).items():
            if k in transforms:
                setattr(info, k, transforms[k](v))
            else:
                setattr(info, k, v)
        return info


class metric_value():
    def __init__(self, dvalue):
        self.dvalue = dvalue

    @staticmethod
    def from_json_obj(jobj):
        return metric_value(jobj['dvalue'])


class fpga_metric():
    def __init__(self, metric_num, value, is_valid):
        self.metric_num = metric_num
        self.value = value
        self.is_valid = is_valid

    @property
    def valid(self):
        return self.is_valid

    @staticmethod
    def from_json_obj(jobj):
        return fpga_metric(int(jobj['metricNum']),
                           metric_value.from_json_obj(jobj['value']),
                           jobj['isvalid'])


class threshold():
    def __init__(self, threshold_name, is_valid, value):
        self.threshold_name = threshold_name
        self.is_valid = is_valid
        self.value = value

    @property
    def valid(self):
        return self.is_valid

    def to_json_obj(self):
        return {'threshold_name': self.threshold_name,
                'is_valid': self.is_valid,
                'value': self.value}

    @staticmethod
    def from_json_obj(jobj):
        return threshold(jobj['thresholdName'],
                         bool(jobj['isvalid']),
                         jobj['value'])


class metric_threshold():

    attrs = {
             'metric_name':        ['metricName'],
             'upper_nr_threshold': ['upperNrThreshold'],
             'upper_c_threshold':  ['upperCThreshold'],
             'upper_nc_threshold': ['upperNcThreshold'],
             'lower_nr_threshold': ['lowerNrThreshold'],
             'lower_c_threshold':  ['lowerCThreshold'],
             'lower_nc_threshold': ['lowerNcThreshold'],
             'hysteresis':         [],
            }

    def __getattr__(self, attr):
        if attr in metric_threshold.attrs:
            return self.__dict__[attr]
        raise AttributeError(attr + ' is not a valid metric_threshold attr')

    def __setattr__(self, attr, value):
        if attr in metric_threshold.attrs:
            self.__dict__[attr] = value
        else:
            raise AttributeError(attr + ' is not an metric_threshold attr')

    def to_json_obj(self):
        d = {}
        transforms = {'upper_nr_threshold': to_json_obj,
                      'upper_c_threshold': to_json_obj,
                      'upper_nc_threshold': to_json_obj,
                      'lower_nr_threshold': to_json_obj,
                      'lower_c_threshold': to_json_obj,
                      'lower_nc_threshold': to_json_obj,
                      'hysteresis': to_json_obj,
                     }
        for attr in metric_threshold.attrs:
            if attr in transforms:
                d[attr] = transforms[attr](self.__dict__[attr])
            else:
                d[attr] = self.__dict__[attr]
        return d

    def to_json_str(self):
        return json.dumps(self.to_json_obj())

    @staticmethod
    def resolve_aliases(d):
        c = d.copy()
        for attr, aliases in metric_threshold.attrs.items():
            if attr in c:
                continue
            for a in aliases:
                if a in c:
                    c[attr] = c[a]
                    del c[a]
        return c

    @staticmethod
    def from_json_obj(jobj):
        thr = metric_threshold()
        transforms = {'upper_nr_threshold': threshold.from_json_obj,
                      'upper_c_threshold': threshold.from_json_obj,
                      'upper_nc_threshold': threshold.from_json_obj,
                      'lower_nr_threshold': threshold.from_json_obj,
                      'lower_c_threshold': threshold.from_json_obj,
                      'lower_nc_threshold': threshold.from_json_obj,
                      'hysteresis': threshold.from_json_obj,
                     }
        for k, v in metric_threshold.resolve_aliases(jobj).items():
            if k in transforms:
                setattr(thr, k, transforms[k](v))
            else:
                setattr(thr, k, v)
        return thr
