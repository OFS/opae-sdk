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

import io
import pytest
import qpafilter
import struct
import unittest

from unittest import mock


def test_two_way_map():
    twp = qpafilter.two_way_map({'sensor_one':1})

    with pytest.raises(KeyError):
        _ = twp[900]

    with pytest.raises(TypeError):
        _ = twp[3.1]

    assert twp[1] == 'sensor_one'
    assert twp['sensor_one'] ==  1


def test_temp_verifier_min_temp():
    m = mock.MagicMock()
    m.min_temp = 80
    tp = qpafilter.temp_verifier(m)

    assert not tp.verify_min_temp(6)

def test_temp_verifier_verify():
    m = mock.MagicMock()
    m.min_temp = 80

    tp = qpafilter.temp_verifier(m)
    items = [{'label': 'test_sensor_0', 'fatal':100, 'units':'°C'}]
    bad_items = [{'label': 'test_sensor_0', 'fatal':100, 'units':'°K'}]

    assert tp.verify(items)
    assert not tp.verify(bad_items)

    items[0]['fatal'] = 6
    assert not tp.verify(items)

def test_blob_writer():
    sensor_m = {'sensor_test_0':88}
    threshold_m = {'Upper Warning':100, 'Upper Fatal':120}
 
    data = io.BytesIO()

    bw = qpafilter.blob_writer('sample_blob.bin', sensor_m, threshold_m)
    bw.outfile = data 
    bw.write_start_marker()

    pos = data.tell()
    data.seek(0) 
    start_marker = data.read()

    assert start_marker == qpafilter.BLOB_START_MARKER

    sample_sensor = {'label':'sensor_test_0', 'filtered_fatal':120, 'filtered_warning':80}
    bw.write_sensor(sample_sensor)

    data.seek(pos)
    sensor_data_bin = data.read()
    fmt = '<LLL'
    sensor_i = struct.iter_unpack(fmt, sensor_data_bin)

    assert next(sensor_i) == (88, 100, 80) 
    assert next(sensor_i) == (88, 120, 120) 

    pos = data.tell()
    bw.write_end_marker()
    data.seek(pos)
    end_marker = data.read()

    assert end_marker == qpafilter.BLOB_END_MARKER 

    with qpafilter.blob_writer('/tmp/blob_written.bin', sensor_m, threshold_m):
        pass

    with open('/tmp/blob_written.bin','rb') as fIn:
        data = fIn.read()
        assert data == qpafilter.BLOB_START_MARKER + qpafilter.BLOB_END_MARKER

def test_get_filter():
    category = "Temperature and Cooling"
    actual = qpafilter.get_filter(category)
    assert actual == qpafilter.temp_filter
    # TODO negative case: pass in non existing category... check for key error/exception?
       
def test_get_verifier(): 
    category = "Temperature and Cooling"
    actual = qpafilter.get_verifier(category)
    assert actual == qpafilter.temp_verifier
    # TODO negative case: pass in non existing category... check for key error/exception?

def test_read_sensors():
    mock_open = mock.mock_open(read_data='Sample sensor 1: 8')
    with mock.patch('builtins.open', mock_open):
        result = qpafilter.read_sensors("Sample_thermal.ini")
        assert result.str_to_int == {"Sample sensor 1": 8}
        assert result.int_to_str == {8:"Sample sensor 1"}

def test_read_qpa():
   valid_temp_overrides = ["Sample Sensor 1 Temperature:50", "Sample Sensor 2 Temperature:80"]
   invalid_temp_overrides = ["Sample Sensor 13 Temp:80", "50"]
  
   data = '''
+-----------------------------------------------------------+
; Temperature and Cooling                                   ;
+-----------------------------------------------+-----------+
; Sample Sensor 1 Temperature                   ; 55.0 °C   ;
; Sample Sensor 2 Temperature                   ; 54.5 °F   ;
; Sample Sensor 3 Temp                          ; 100  °c   ;
+-----------------------------------------------+-----------+
''' 
   fileIn = io.StringIO(data)

   valid_actual = qpafilter.read_qpa(fileIn, valid_temp_overrides)
   assert valid_actual["Temperature and Cooling"] == [{'label': 'Sample Sensor 1 Temperature', 'fatal': '55.0', 'units': '°C', 'warning': 0.0,  'override': '50'}, {'label': 'Sample Sensor 2 Temperature', 'fatal': '54.5', 'units': '°F', 'warning': 0.0, 'override':'80'}, {'label': 'Sample Sensor 3 Temp', 'fatal': '100', 'units': '°c', 'warning': 0.0}]

   # TODO:  Anyway to check this?
   _ = qpafilter.read_qpa(fileIn, invalid_temp_overrides)
    
