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

import base64
import json
import requests
import struct
import sys

from opae.http.conversion import to_json_obj
from opae.http.fpga_remote_id import fpga_remote_id
import opae.http.constants as constants
from opae.http.properties import properties
from opae.http.shared_buffer import shared_buffer
from opae.http.sysobject import sysobject
from opae.http.metrics import fpga_metric_info, fpga_metric


class handle():
    MAGIC = 0x46504741484e444c

    attrs = {
             'magic':     [],
             'token_id':  ['tokenId'],
             'handle_id': ['handleId'],
            }

    def __init__(self):
        self.__dict__['_mmio_map'] = {}
        self.__dict__['_shared_buffers'] = []

    def __getattr__(self, attr):
        if attr in handle.attrs:
            return self.__dict__[attr]
        raise AttributeError(attr + ' is not a valid handle attr')

    def __setattr__(self, attr, value):
        if attr in handle.attrs:
            self.__dict__[attr] = value
        else:
            raise AttributeError(attr + ' is not a handle attr')

    def to_json_obj(self):
        d = {}
        d['magic'] = str(handle.MAGIC)
        transforms = {'token_id': to_json_obj,
                      'handle_id': to_json_obj,
                     }
        for attr in handle.attrs:
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
        for attr, aliases in handle.attrs.items():
            if attr in c:
                continue
            for a in aliases:
                if a in c:
                    c[attr] = c[a]
                    del c[a]
        c['token_id'] = fpga_remote_id.resolve_aliases(c['token_id'])
        c['handle_id'] = fpga_remote_id.resolve_aliases(c['handle_id'])
        return c

    @staticmethod
    def from_json_obj(jobj):
        h = handle()
        transforms = {'token_id': fpga_remote_id.from_json_obj,
                      'handle_id': fpga_remote_id.from_json_obj,
                     }
        for k, v in handle.resolve_aliases(jobj).items():
            if k in transforms:
                setattr(h, k, transforms[k](v))
            else:
                setattr(h, k, v)
        return h

    def close(self):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/close'

        req = {'handle_id': self.handle_id.to_json_obj()}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaClose Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaClose Response: {resp.text}')

        jobj = resp.json()
        constants.raise_for_error(jobj['result'], 'fpgaClose returned')

    def reset(self):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/afu/reset'

        req = {'handle_id': self.handle_id.to_json_obj()}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaReset Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaReset Response: {resp.text}')

        jobj = resp.json()
        constants.raise_for_error(jobj['result'], 'fpgaReset returned')

    def get_properties(self):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/properties/get'

        req = {'handle_id': self.handle_id.to_json_obj()}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaGetPropertiesFromHandle Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaGetPropertiesFromHandle Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetPropertiesFromHandle returned')

        return properties.from_json_obj(jobj['properties'])

    def map_mmio(self, region_num):
        map_dict = self.__dict__['_mmio_map']
        if region_num in map_dict:
            msg = f'MMIO region {region_num} is already mapped.'
            constants.raise_for_error(constants.FPGA_BUSY, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/map'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaMapMMIO Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaMapMMIO Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaMapMMIO returned')

        map_dict[region_num] = fpga_remote_id.from_json_obj(jobj['mmioId'])

    def unmap_mmio(self, region_num):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/unmap'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_id': map_dict[region_num].to_json_obj(),
               'mmio_num': region_num}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaUnmapMMIO Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaUnmapMMIO Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaUnmapMMIO returned')
        del map_dict[region_num]

    def read32(self, region_num, offset):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/read32'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset)}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaReadMMIO32 Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaReadMMIO32 Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaReadMMIO32 returned')
        return jobj['value']

    def read64(self, region_num, offset):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/read64'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset)}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaReadMMIO64 Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaReadMMIO64 Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaReadMMIO64 returned')
        return int(jobj['value'])

    def write32(self, region_num, offset, value):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/write32'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset),
               'value': value}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaWriteMMIO32 Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaWriteMMIO32 Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaWriteMMIO32 returned')

    def write64(self, region_num, offset, value):
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/write64'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset),
               'value': str(value)}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaWriteMMIO64 Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaWriteMMIO64 Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaWriteMMIO64 returned')

    def write512(self, region_num, offset, values):
        """values is interpreted as a list of 8-byte integers.
        """
        map_dict = self.__dict__['_mmio_map']
        if region_num not in map_dict:
            msg = f'MMIO region {region_num} is not mapped.'
            constants.raise_for_error(constants.FPGA_EXCEPTION, msg)

        if len(values) != 8: # 8 64-bit integers = 512 bits
            msg = 'write512: values should be a list of eight 8-byte integers'
            constants.raise_for_error(constants.FPGA_INVALID_PARAM, msg)

        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/mmio/write512'

        order = '<' if sys.byteorder == 'little' else '>'

        packed = struct.pack(f'{order}8Q', *values)
        encoded = base64.b64encode(packed)

        req = {'handle_id': self.handle_id.to_json_obj(),
               'mmio_num': region_num,
               'offset': str(offset),
               'values': encoded.decode()}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaWriteMMIO512 Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaWriteMMIO512 Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaWriteMMIO512 returned')

    def prepare_buffer(self, length, pre_allocated_addr=0, flags=0):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/buffers/prepare'

        pre = (flags & constants.FPGA_BUF_PREALLOCATED) != 0

        req = {'handle_id': self.handle_id.to_json_obj(),
               'length': str(length),
               'have_buf_addr': True,
               'pre_allocated_addr': str(pre_allocated_addr) if pre else '0',
               'flags': flags}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaPrepareBuffer Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaPrepareBuffer Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaPrepareBuffer returned')

        buf_id = fpga_remote_id.from_json_obj(jobj['bufId'])

        buffer = shared_buffer(self, buf_id, length, self.__dict__['_debug'])
        self.__dict__['_shared_buffers'].append(buffer)

        return buffer

    def get_object(self, name, flags):
        tok = self.__dict__['_token']
        save_url = tok.__dict__['_url']
        url = save_url + '/fpga/v1/handle/sysobject/get'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'name': name,
               'flags': flags}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaHandleGetObject Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaHandleGetObject Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaHandleGetObject returned')

        return sysobject(save_url,
                         name,
                         fpga_remote_id.from_json_obj(jobj['objectId']),
                         self.__dict__['_debug'])

    def set_user_clocks(self, high_clock, low_clock, flags):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/clock/frequency/set'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'high_clk': str(high_clock),
               'low_clk': str(low_clock),
               'flags': flags}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaSetUserClock Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaSetUserClock Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaSetUserClock returned')

    def get_user_clocks(self, flags):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/clock/frequency/get'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'flags': flags}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaGetUserClock Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaGetUserClock Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetUserClock returned')

        return (int(jobj['highClk']), int(jobj['lowClk']))

    def get_metrics_count(self):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/metrics/count'

        req = {'handle_id': self.handle_id.to_json_obj()}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaGetNumMetrics Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaGetNumMetrics Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetNumMetrics returned')

        return int(jobj['numMetrics'])

    def get_metrics_info(self, count):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/metrics/info/get'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'num_metrics': str(count)}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaGetMetricsInfo Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaGetMetricsInfo Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetMetricsInfo returned')

        l = []
        for i in jobj['info']:
            l.append(fpga_metric_info.from_json_obj(i))

        c = int(jobj['numMetrics'])
        if c < count:
            count = c

        if count < len(l):
            l = l[:count]

        return l

    def get_metrics_by_index(self, indexes):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/metrics/get/index'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'metric_num': indexes,
               'num_metric_indexes': str(len(indexes))}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaGetMetricsByIndex Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaGetMetricsByIndex Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetMetricsByIndex returned')

        num_indexes = int(jobj['numMetricIndexes'])
        l = []
        for m in jobj['metrics']:
            l.append(fpga_metric.from_json_obj(m))

        if num_indexes < len(l):
            l = l[:num_indexes]

        return l

    def get_metrics_by_name(self, names):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/metrics/get/name'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'metrics_names': names,
               'num_metric_names': str(len(names))}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaGetMetricsByName Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaGetMetricsByName Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetMetricsByName returned')

        num_names = int(jobj['numMetricNames'])
        l = []
        for m in jobj['metrics']:
            l.append(fpga_metric.from_json_obj(m))

        if num_names < len(l):
            l = l[:num_names]

        return l

    def get_metrics_threshold_info(self, num_thresholds):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/metrics/threshold/get'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'num_thresholds': num_thresholds}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaGetMetricsThresholdInfo Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaGetMetricsThresholdInfo Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaGetMetricsThresholdInfo returned')

        num_thresholds = jobj['numThresholds']

        l = []
        for t in jobj['metricThreshold']:
            l.append(metric_threshold.from_json_obj(t))

        if num_thresholds < len(l):
            l = l[:num_thresholds]

        return l

    def reconfigure_slot_by_name(self, slot, path, flags):
        tok = self.__dict__['_token']
        url = tok.__dict__['_url'] + '/fpga/v1/handle/reconfigure/name'

        req = {'handle_id': self.handle_id.to_json_obj(),
               'slot': slot,
               'path': path,
               'flags': flags}

        if self.__dict__['_debug']:
            print(url)
            print(f'fpgaReconfigureSlotByName Request: {req}')

        resp = requests.post(url, json=req)

        resp.raise_for_status()

        if self.__dict__['_debug']:
            print(f'fpgaReconfigureSlotByName Response: {resp.text}')

        jobj = resp.json()

        constants.raise_for_error(jobj['result'], 'fpgaReconfigureSlotByName returned')
