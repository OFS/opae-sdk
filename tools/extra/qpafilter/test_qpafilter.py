import pytest
import qpafilter
import unittest
import io
import struct
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
 
    bw = qpafilter.blob_writer('sample_blob.bin', sensor_m, threshold_m)
    data = io.BytesIO()
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
        
        

