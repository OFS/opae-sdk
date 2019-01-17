# Copyright(c) 2018, Intel Corporation
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

# pylint : skip-file
import argparse
import math
import mock
import itertools
import json
import numpy as np
import random
import logging
import re
import sys
import unittest
from StringIO import StringIO
from nose2.tools import params

from opae.tools.fpgadiag import nlb
from opae.tools.fpgadiag.nlb0 import nlb0
from opae.tools.fpgadiag.nlb3 import nlb3
from opae.tools.fpgadiag.fpgamux import fpgamux

import opae.fpga

NUM_LINES = 0x0130
SRC_ADDR = 0x0120
DST_ADDR = 0x0128
DSM_ADDR = 0x0110
CTL_REG = 0x0138
CFG_REG = 0x0140

DSM_NUM_CLOCKS = 0x0048
DSM_NUM_READS = 0x0050
DSM_NUM_WRITES = 0x0054
DSM_START_OVERHEAD = 0x0058
DSM_END_OVERHEAD = 0x005c

if sys.version_info[0] == 3:
    long = int


class mock_sysobject(mock.Mock):
    def __getitem__(self, name):
        return mock_sysobject(name)

    def read64(self, *args):
        return 0


class mock_fmehandle(mock.Mock):
    def find(self, name, *args, **kwargs):
        return mock_sysobject(name)


class mock_nlb(object):
    MMIO_SIZE = 256*1024

    def __init__(self, freq):
        self._freq = freq
        self._on = True
        self._clock = 0
        self._clocks = 0
        self._mmio = np.zeros(self.MMIO_SIZE/8, dtype=np.uint64)
        self._delay = 1.0/freq
        self._lines = 0
        self._done = False
        self._buffers = []

    def handle(self):
        return mock_handle(self, self._mmio)

    def find_buffer(self, addr):
        for b in self._buffers:
            if addr >= b.io_address()-64 and addr < b.io_address()+b.size():
                return b

    def _write_dsm(self):
        assert self._mmio[DSM_ADDR]
        dsm_addr = self._mmio[DSM_ADDR]
        dbuf = self.find_buffer(dsm_addr)
        if dbuf:
            mode = (int(self._mmio[CFG_REG]) >> 2) & 3
            if mode == 0 or mode == 3:  # lpbk1 or trput
                dbuf._buffer[DSM_NUM_READS/4] = self._lines
                dbuf._buffer[DSM_NUM_WRITES/4] = self._lines
            elif mode == 1:  # read
                dbuf._buffer[DSM_NUM_READS/4] = self._lines
            elif mode == 2:  # write
                dbuf._buffer[DSM_NUM_WRITES/4] = self._lines
            dbuf._buffer[DSM_NUM_CLOCKS/4] = self._clocks
            if self._lines == self._mmio[NUM_LINES]:
                dbuf._buffer[0x40/4] = 1
                self._on = False

    def tick(self):
        if (int(self._mmio[CTL_REG]) & 0x3) == 0x3:
            cfg = int(self._mmio[CFG_REG])
            cfg_mode = (cfg >> 2) & 7
            if cfg_mode != 2:  # if mode is not write, assert we have a src
                assert self._mmio[SRC_ADDR]
            if cfg_mode != 1:  # if mode is not read, assert we have a dst
                assert self._mmio[DST_ADDR]
            self._lines = self._mmio[NUM_LINES]
            self._clocks = 180 + self._lines*random.randint(60, 75)/10
            self._write_dsm()


class mock_handle(object):
    def __init__(self, parent, mmio, *args, **kwargs):
        self._parent = parent
        self._mmio = mmio

    def write_csr32(self, offset, value):
        self._mmio[offset] = value
        self._parent.tick()

    def write_csr64(self, offset, value):
        self._mmio[offset] = value
        self._parent.tick()

    def read_csr32(self, offset):
        self._parent.tick()
        return int(self._mmio[offset])

    def read_csr64(self, offset):
        self._parent.tick()
        return long(self._mmio[offset])


class mock_shared_buffer(object):
    def __init__(self, parent, size):
        self._parent = parent
        self._nlb = parent._parent
        self._size = size

    def allocate(self):
        self._buffer = np.zeros(self._size/4, dtype=np.uint32)
        self._io_addr = long(id(self._buffer.data))

    def read32(self, offset):
        return self._buffer[offset/4]

    def io_address(self):
        return self._io_addr

    def poll(self, offset, value, **kwargs):
        return True

    def poll32(self, offset, value, **kwargs):
        return self.poll(offset, value, **kwargs)

    def poll64(self, offset, value, **kwargs):
        return self.poll(offset, value, **kwargs)

    def fill(self, value):
        self._buffer.fill(value)

    def compare(self, other, size=0):
        if size == 0:
            return np.array_equal(self._buffer, other._buffer)
        else:
            return np.array_equal(
                self._buffer[: size / 4],
                other._buffer[: size / 4])

    def size(self):
        return self._size

    def split(self, *args):
        offset = 0
        buffers = list()
        for sz in args:
            if offset + sz > self.size():
                raise RuntimeError("buffer not big enough to split")
            buffers.append(mock_split_buffer(self, offset, sz))
            offset += sz
        return buffers


class mock_split_buffer(mock_shared_buffer):
    def __init__(self, parent, offset, size):
        self._parent = parent._parent
        self._size = size
        self._buffer = parent._buffer[offset:offset+size]
        self._io_addr = parent._io_addr + offset


def mock_allocate_shared_buffer(handle, size):
    roundup = int(math.ceil(size/64) * 64) + 1
    buf = mock_shared_buffer(handle, roundup)
    buf.allocate()
    handle._parent._buffers.append(buf)
    return buf


opae.fpga.allocate_shared_buffer = mock_allocate_shared_buffer
modes = ('read', 'write')
begin_cl = (1, 16, 32, 64, 128, 256)
read_ch = ('auto', 'vl0', 'vh0', 'vh1')
write_ch = ('auto', 'vl0', 'vh0', 'vh1')
cont_mode = (True, False)
with_csv = (True, False)
prime_fpga = (None, '--warm-fpga-cache', '--cool-fpga-cache')
mcl_choices = (1, 2, 4)

nlb0_params = [
    (str(b), rd, wr, c, v, str(mcl)) for (
        b, rd, wr, c, v, mcl) in itertools.product(
            begin_cl, read_ch, write_ch, cont_mode, with_csv, mcl_choices)]

nlb3_params = [
    (m,
     str(b),
     rd,
     wr,
     c,
     v,
     pr) for (
        m,
        b,
        rd,
        wr,
        c,
        v,
        pr) in itertools.product(
            modes,
            begin_cl,
            read_ch,
            write_ch,
            cont_mode,
            with_csv,
        prime_fpga)]

NORMAL_OUTPUT = re.compile(r'''
\ACachelines\sRead_Count\sWrite_Count\sCache_Rd_Hit\sCache_Wr_Hit\sCache_Rd_Miss\sCache_Wr_Miss\sEviction\sClocks\(@(\d+(?:\.\d+)?)\sMHz\)\sRd_Bandwidth\sWr_Bandwidth\s*
(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+\.\d+\sGB/s\s*){2}\s*
VH0_Rd_Count\sVH0_Wr_Count\sVH1_Rd_Count\sVH1_Wr_Count\sVL0_Rd_Count\sVL0_Wr_Count\s*
(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*\Z
''', re.VERBOSE)

CSV_OUTPUT_HDR = re.compile(r'''
Cachelines,Read_Count,Write_Count,Cache_Rd_Hit,Cache_Wr_Hit,Cache_Rd_Miss,Cache_Wr_Miss,Eviction,Clocks\(@(\d+(?:\.\d+)?)\sMHz\),Rd_Bandwidth,Wr_Bandwidth,VH0_Rd_Count,VH0_Wr_Count,VH1_Rd_Count,VH1_Wr_Count,VL0_Rd_Count,VL0_Wr_Count\s+
(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+\.\d+),(\d+\.\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)s*
''', re.VERBOSE)


class NLBTest(unittest.TestCase):
    def setUp(self):
        self._nlb = mock_nlb(400*1E6)
        # self._nlb.start()
        self._acc_handle = self._nlb.handle()
        self._parser = argparse.ArgumentParser(add_help=False)
        self._parser.add_argument(
            '-m',
            '--mode',
            choices=[
                'lpbk1',
                'read',
                'write',
                'trput'])
        self._parser.add_argument(
            '--loglevel',
            choices=[
                'exception',
                'error',
                'warning',
                'info',
                'debug'],
            default='warning',
            help='error level to set')
        self._parser.add_argument(
            '--version',
            help='print version information then quit',
            action='store_true',
            default=False)
        self._parser.add_argument('-h', '--help',
                                  action='store_true',
                                  default=False,
                                  help='print help message and exit')

    @params(*nlb0_params)
    def test_nlb0(self, begin, read_ch, write_ch, cont, csv, mcl):
        h = self._acc_handle
        cmdline_args = [
            '-m',
            'lpbk1',
            '-b',
            begin,
            '-r',
            read_ch,
            '-w',
            write_ch,
            '--multi-cl',
            mcl,
            '--loglevel',
            'warning']
        if cont:
            cmdline_args.append('--cont')
        if csv:
            cmdline_args.append('--csv')

        args, _ = self._parser.parse_known_args(cmdline_args)

        logger = logging.getLogger("fpgadiag")
        stream_handler = logging.StreamHandler(stream=sys.stderr)
        stream_handler.setFormatter(logging.Formatter(
            '%(asctime)s: [%(name)-8s][%(levelname)-6s] - %(message)s'))
        logger.addHandler(stream_handler)
        logger.setLevel(getattr(logging, args.loglevel.upper()))

        nlb = nlb0('lpbk1', self._parser)
        nlb.logger.setLevel(getattr(logging, args.loglevel.upper()))
        self.assertTrue(nlb.setup(cmdline_args))
        output = ''
        with mock.patch('sys.stdout', new=StringIO()) as fake_stdout:
            with mock.patch('time.sleep', new=lambda x: None):
                nlb.run(h, mock_fmehandle())
                output = fake_stdout.getvalue().strip()
        self.assertFalse(output == '')
        if not csv:
            m = NORMAL_OUTPUT.match(output)
            self.assertNotEqual(m, None)
        else:
            if nlb.args.suppress_hdr:
                pass
            else:
                m = CSV_OUTPUT_HDR.match(output)
                self.assertNotEqual(m, None)
        self.assertCfg(nlb.args)

    @params(*nlb3_params)
    def test_nlb3(self, mode, begin, read_ch, write_ch, cont, csv, prime_fpga):
        h = self._acc_handle
        cmdline_args = [
            '-m',
            mode,
            '-b',
            begin,
            '-r',
            read_ch,
            '-w',
            write_ch,
            '--loglevel',
            'warning']
        if cont:
            cmdline_args.append('--cont')
        if csv:
            cmdline_args.append('--csv')
        if prime_fpga is not None:
            cmdline_args.append(prime_fpga)

        args, _ = self._parser.parse_known_args(cmdline_args)
        nlb = nlb3(mode, self._parser)
        nlb.logger.setLevel(getattr(logging, args.loglevel.upper()))
        self.assertTrue(nlb.setup(cmdline_args))
        output = ''
        with mock.patch('sys.stdout', new=StringIO()) as fake_stdout:
            with mock.patch('time.sleep', new=lambda x: None):
                nlb.run(h, mock_fmehandle())
                output = fake_stdout.getvalue().strip()
        self.assertFalse(output == '')
        if not csv:
            m = NORMAL_OUTPUT.match(output)
            self.assertNotEqual(m, None)
        else:
            if nlb.args.suppress_hdr:
                pass
            else:
                m = CSV_OUTPUT_HDR.match(output)
                self.assertNotEqual(m, None)
        self.assertCfg(nlb.args)

    def assertCfg(self, args):
        cfg = long(self._nlb._mmio[CFG_REG])
        num_lines = long(self._nlb._mmio[0x0130])
        self.assertEqual(((cfg >> 12) & 7), int(args.read_vc))
        self.assertEqual(((cfg >> 17) & 7), int(args.write_vc))
        self.assertEqual(((cfg >> 1) & 1), 1 if args.cont else 0)
        self.assertEqual(((cfg >> 5) & 3), args.multi_cl-1)
        self.assertEqual(num_lines, args.end)


class PerfCountersTests(unittest.TestCase):
    def test_no_counters(self):
        h = mock.MagicMock()
        h.find = mock.Mock(side_effect=RuntimeError())
        c_counters = nlb.cache_counters(h)
        c_values = c_counters.read()
        self.assertFalse(all(c_values._values.values()))

    def test_missing_counters(self):
        h = mock.MagicMock()
        h.find.return_value = mock.Mock()

        fake_value = 0

        def get_fabric_counter(_, name):
            if name.startswith("mmio_"):
                raise RuntimeError
            m = mock.Mock()
            m.read64 = mock.Mock(return_value=fake_value)
            return m

        h.find.return_value.__getitem__ = get_fabric_counter
        f_counters = nlb.fabric_counters(h)
        f_values = f_counters.read()
        self.assertEqual(sum(f_values._values.values()), 0)

        fake_value = 100
        begin_f, end_f = (None, None)
        r = f_counters.reader()
        self.assertNotEqual(r, None)
        with f_counters.reader() as r:
            begin_f = r.read()
        self.assertNotEqual(begin_f, None)

        fake_value = 275
        with f_counters.reader() as r:
            end_f = r.read()
        self.assertNotEqual(end_f, None)

        delta = end_f - begin_f
        self.assertNotEqual(delta, None)
        self.assertEqual(
            sum(delta._values.values()),
            175 * (len(delta._values) - 2))


class FpgaMuxTests(unittest.TestCase):
    def test_fpgamux_create(self):
        muxconfig = [{ "app": "nlb0",
                      "name": "nlb0",
                      "disabled": False,
                      "config": { "begin": 1 } }]
        with open('mux.json', 'w') as fd:
            json.dump(muxconfig, fd)

        tests = fpgamux.create()
        self.assertNotEqual(len(tests), 0)
        self.assertEqual(type(tests[0]), nlb0)


if __name__ == "__main__":
    t = PerfCountersTests("test_missing_counters")
    t.run()
