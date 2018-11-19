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
import logging
from opae.utils import CACHELINE_BYTES
from opae.utils.csr import csr, f_enum
# pylint: disable=E0611
from opae import fpga


class CFG(csr):
    """cfg"""
    _offset_ = 0x0140
    _bits_ = [
        ("wrthru_en", (0, 0)),
        ("cont", (1, 1)),
        ("mode", (4, 2)),
        ("multiCL_len", (6, 5)),
        ("rdsel", (10, 9)),
        ("rd_chsel", (14, 12)),
        ("wrdin_msb", (15, 15)),
        ("wrpush_i", (16, 16)),
        ("wr_chsel", (19, 17)),
        ("wf_chsel", (31, 30)),
        ("test_cfg", (27, 20)),
        ("interrupt_on_error", (28, 28)),
        ("interrupt_testmode", (29, 29))
    ]

    _enums_ = [
        f_enum(name="wrthru_en", wrline_M=0, wrline_I=1),
        f_enum(name="mode", lbpk1=0, read=1, write=2, trput=3, sw=7),
        f_enum(name="multiCL_len", mcl1=0, mcl2=1, mcl4=3),
        f_enum(name="rdsel", rds=0, rdi=1),
        f_enum(name="rd_chsel", auto=0, vl0=1, vh0=2, vh1=3, random=4),
        f_enum(name="wrdin_msb", alt_wr_prn=1),
        f_enum(name="wrpush_i", wrpush_I=1),
        f_enum(name="wr_chsel", auto=0, vl0=1, vh0=2, vh1=3, random=4),
        f_enum(name="test_cfg", csr_write=64, umsg_data=128, umsg_hint=192),
        f_enum(name="wf_chsel", auto=0, vl0=1, vh0=2, vh1=3)
    ]


class CTL(csr):
    """ctl"""
    _offset_ = 0x0138
    _bits_ = [
        ("reset", (0, 0)),
        ("start", (1, 1)),
        ("stop", (2, 2))
    ]


class null_counter(object):
    def read64(self):
        return 0

    def write64(self, v):
        pass


class null_group(object):
    def __getattr__(self, name):
        return null_counter()

    def __getitem__(self, name):
        return null_counter()

    def find(self, name, *args):
        return null_counter()


class counter_values(object):
    def __init__(self, values):
        self._values = values

    def __getitem__(self, key):
        return self._values[key]

    def __sub__(self, other):
        return counter_values(
                              dict(
                                  [(c, (self[c] - other[c]))
                                   for c in self._values.keys()]))

    def __repr__(self):
        return repr(self._values)


class null_values(object):
    def __init__(self, name):
        self._name = name

    def __getitem__(self, key):
        return None

    def __sub__(self, other):
        return null_values(self._name)

    def __repr__(self):
        return "null ({})".format(self._name)


class perf_counters(object):
    _name = "_UNDEFINED_"
    _counters = []
    _values = dict()

    def __init__(self, handle):
        logger_name = "fpgadiag.{}".format(self.__class__.__name__)
        self.logger = logging.getLogger(logger_name)
        self._handle = handle
        try:
            self._group = handle.find(self._name, fpga.SYSOBJECT_GLOB)
        except RuntimeError:
            self.logger.debug("Could not find group with name: %s", self._name)
            self._group = None
        else:
            self._values = dict([(k, 0) for k in self._counters])

    def read(self, *args):
        if self._group is None:
            values = counter_values(
                dict([(c, 0)
                      for c in args or self._counters]))
        else:
            values_dict = {}
            for c in args or self._counters:
                try:
                    c_obj = self._group[c]
                    values_dict[c] = c_obj.read64() if c_obj else 0
                except RuntimeError:
                    values_dict[c] = 0
            values = counter_values(values_dict)
        return values

    def reader(self):
        return counters_reader(self)


class cache_counters(perf_counters):
    _name = "*perf/cache"
    _counters = ["read_hit",
                 "write_hit",
                 "read_miss",
                 "write_miss",
                 "hold_request",
                 "data_write_port_contention",
                 "tag_write_port_contention",
                 "tx_req_stall",
                 "rx_req_stall",
                 "rx_eviction"]


class fabric_counters(perf_counters):
    _name = "*perf/fabric"
    _counters = ["mmio_read",
                 "mmio_write",
                 "pcie0_read",
                 "pcie0_write",
                 "pcie1_read",
                 "pcie1_write",
                 "upi_read",
                 "upi_write"]


class counters_reader(object):
    def __init__(self, group, freeze=True):
        self._group = group
        self._freeze = freeze

    def __enter__(self):
        if self._freeze:
            self._group.freeze = 1
            return self

    def __exit__(self, t, v, tb):
        if self._freeze:
            self._group.freeze = 0
        return True

    def read(self, *args):
        return self._group.read(*args)


class dsm_tuple(object):
    NUM_CLOCKS = 0x0048
    NUM_READS = 0x0050
    NUM_WRITES = 0x0054
    START_OVERHEAD = 0x0058
    END_OVERHEAD = 0x005c

    def __init__(self, mem=None):
        if mem:
            self.get(mem)
        else:
            self._num_clocks = 0
            self._start_overhead = 0
            self._end_overhead = 0
            self._num_reads = 0
            self._num_writes = 0

    def get(self, mem):
        self._num_clocks = mem.read32(self.NUM_CLOCKS)
        self._start_overhead = mem.read32(self.START_OVERHEAD)
        self._end_overhead = mem.read32(self.END_OVERHEAD)
        self._num_reads = mem.read32(self.NUM_READS)
        self._num_writes = mem.read32(self.NUM_WRITES)

    def put(self, mem):
        mem.write32(self.NUM_CLOCKS, self._num_clocks)
        mem.write32(self.START_OVERHEAD, self._start_overhead)
        mem.write32(self.END_OVERHEAD, self._end_overhead)
        mem.write32(self.NUM_READS, self._num_writes)
        mem.write32(self.NUM_WRITES, self._num_writes)

    def __add__(self, other):
        self._num_clocks = self._num_clocks + other._num_clocks
        self._start_overhead = self._start_overhead + other._start_overhead
        self._end_overhead = self._end_overhead + other._end_overhead
        self._num_reads = self._num_reads + other._num_reads
        self._num_writes = self._num_writes + other._num_writes

    @property
    def num_clocks(self):
        return self._num_clocks

    @property
    def start_overhead(self):
        return self._start_overhead

    @property
    def end_overhead(self):
        return self._end_overhead

    @property
    def num_reads(self):
        return self._num_reads

    @property
    def num_writes(self):
        return self._num_writes


class units(object):
    def __init__(self, value, **kwargs):
        self._value = value
        self._format = kwargs.get('format')

    def __format__(self, fs):
        if self._format is not None:
            return self._format.format(self._value)
        else:
            return fs.format(self._value)

    def __str__(self):
        return str(self._value)


class stats_printer(object):
    def __init__(self):
        self._data = []

    def register(self, *args):
        line = []
        for (k, v) in args:
            if v is not None:
                line.append((k, v))
        self._data.append(line)

    def to_str(self):
        s = ''
        for line in self._data:
            headers = [h for (h, v) in line]
            values = [format(v) for (h, v) in line]
            fmt = ' '.join(['{{:{}}}'.format(len(h)) for h in headers])
            s += fmt.format(*headers) + '\n'
            s += fmt.format(*values) + '\n\n'
        return s

    def to_csv(self, with_header=True):
        s = ''
        header = []
        values = []
        for line in self._data:
            for (h, v) in line:
                header.append(h)
                values.append(str(v))
        if with_header:
            s += ','.join(header) + '\n'
        s += ','.join(values)
        if with_header:
            s += '\n'
        return s


def normalize_frequency(freq):
    units = [(1E9, 'GHz'),
             (1E6, 'MHz'),
             (1E3, 'KHz'),
             (0, 'Hz')]
    for (v, s) in units:
        if freq >= v:
            return '{:2} {}'.format(freq/v, s)


class nlb_stats(stats_printer):
    def __init__(self, cachelines, dsm, cache, fabric, **kwargs):
        super(nlb_stats, self).__init__()
        Gbit = 1000.0 * 1000.0 * 1000.0
        freq = kwargs.get('frequency', 400000000)
        freq_str = normalize_frequency(freq)
        is_cont = kwargs.get("cont", False)
        raw_ticks = dsm.num_clocks
        if is_cont:
            ticks = raw_ticks - dsm.start_overhead
        else:
            ticks = raw_ticks - (dsm.start_overhead + dsm.end_overhead)

        rd_bw = (dsm.num_reads * CACHELINE_BYTES * freq)/ticks
        rd_bw /= Gbit

        wr_bw = (dsm.num_writes * CACHELINE_BYTES * freq)/ticks
        wr_bw /= Gbit

        self.register(('Cachelines', cachelines),
                      ('Read_Count', dsm.num_reads),
                      ('Write_Count', dsm.num_writes),
                      ('Cache_Rd_Hit', cache['read_hit']),
                      ('Cache_Wr_Hit', cache['write_hit']),
                      ('Cache_Rd_Miss', cache['read_miss']),
                      ('Cache_Wr_Miss', cache['write_miss']),
                      ('Eviction', cache['rx_eviction']),
                      ('Clocks(@{})'.format(freq_str), ticks),
                      ('Rd_Bandwidth', units(rd_bw, format='{:.4} GB/s')),
                      ('Wr_Bandwidth', units(wr_bw, format='{:.4} GB/s')))
        self.register(('VH0_Rd_Count', fabric['pcie0_read']),
                      ('VH0_Wr_Count', fabric['pcie0_write']),
                      ('VH1_Rd_Count', fabric['pcie1_read']),
                      ('VH1_Wr_Count', fabric['pcie1_write']),
                      ('VL0_Rd_Count', fabric['upi_read']),
                      ('VL0_Wr_Count', fabric['upi_write']))
