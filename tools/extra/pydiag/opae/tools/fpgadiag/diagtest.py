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
import logging
import time
import nlb
from datetime import timedelta
from opae import fpga
from opae.utils import csr, parse_int, cl_align, CACHELINE_BYTES
from opae.utils.byteutils import KiB, MiB


def to_seconds(args):
    return timedelta(hours=args.timeout_hour,
                     minutes=args.timeout_min,
                     seconds=args.timeout_sec,
                     milliseconds=args.timeout_msec,
                     microseconds=args.timeout_usec).seconds


class diagtest(object):
    guid = None
    cfg = nlb.CFG()
    ctl = nlb.CTL()
    NUM_LINES = 0x130
    SRC_ADDR = 0x0120
    DST_ADDR = 0x0128
    DSM_ADDR = 0x0110
    DSM_COMPLETE = 0x40
    DSM_TIMEOUT = 1.0
    cache_policy = csr.f_enum({"wrline-M": 0,
                               "wrline-I": 1,
                               "wrpush-I": 1})

    cache_hint = csr.f_enum({"rdline-I": 1,
                             "rdline-S": 0})

    rd_chsel = csr.f_enum({"auto": 0,
                           "vl0": 1,
                           "vh0": 2,
                           "vh1": 3,
                           "random": 4})
    wr_chsel = csr.f_enum({"auto": 0,
                           "vl0": 1,
                           "vh0": 2,
                           "vh1": 3,
                           "random": 4})
    wf_chsel = csr.f_enum({"auto": 0,
                           "vl0": 1,
                           "vh0": 2,
                           "vh1": 3})

    def __init__(self, mode, parser):
        self._mode = mode
        self._parser = parser
        parser.add_argument(
            "-c",
            "--config",
            help="Path to test config file")
        parser.add_argument("-t", "--target", help="one of {fpga, ase}")
        parser.add_argument(
            '-B',
            '--bus',
            help='PCIe bus number',
            type=parse_int)
        parser.add_argument(
            '-D',
            '--device',
            help='PCIe device number',
            type=parse_int)
        parser.add_argument(
            '-F',
            '--function',
            help='PCIe function number',
            type=parse_int)
        parser.add_argument(
            '-S',
            '--socket_id',
            help='socket id encoded in BBS',
            type=parse_int)
        parser.add_argument(
            '-G',
            '--guid',
            help='GUID of accelerator',
            default=self.guid)

    def enumerate(self, **kwargs):
        args, _ = self._parser.parse_known_args()
        prop_names = ['bus', 'device', 'function', 'socket_id', 'guid']
        filt = dict([(p, getattr(args, p))
                     for p in prop_names if getattr(args, p)])

        tokens = fpga.enumerate(type=fpga.ACCELERATOR, **filt)
        if not tokens:
            logging.error("could not find resources")
            return None
        if len(tokens) > 1:
            logging.warn("found more than one accelerator")
        return tokens

    def setup(self):
        parser = self._parser.add_argument_group(self._mode)
        parser.add_argument("-b", "--begin",
                            default=1,
                            type=int,
                            help="where 1 <= <value> <= 65535")
        parser.add_argument("-e", "--end",
                            type=int,
                            help="where 1 <= <value> <= 65535")
        parser.add_argument("-u", "--multi-cl",
                            type=int,
                            default=1,
                            choices=[1, 2, 4],
                            help="one of {1, 2, 4}")
        parser.add_argument("-L", "--cont",
                            default=False,
                            action='store_true',
                            help="Continuous mode")
        parser.add_argument("-p", "--cache-policy",
                            choices=self.cache_policy.keys(),
                            type=self.cache_policy,
                            default='wrline-M',
                            help="write cache policy")
        parser.add_argument("-i", "--cache-hint",
                            choices=self.cache_hint.keys(),
                            type=self.cache_hint,
                            default="rdline-I",
                            help="rd cache hint")
        parser.add_argument("-r", "--read-vc",
                            choices=self.rd_chsel.keys(),
                            type=self.rd_chsel,
                            default="auto",
                            help="read channel select")
        parser.add_argument("-w", "--write-vc",
                            choices=self.wr_chsel.keys(),
                            type=self.wr_chsel,
                            default="auto",
                            help="write channel select")
        parser.add_argument("-f", "--wrfence-vc",
                            choices=self.wf_chsel.keys(),
                            type=self.wf_chsel,
                            help="auto")
        parser.add_argument("--dsm-timeout-usec",
                            type=int, help="Timeout for test completion")
        parser.add_argument(
            "--timeout-usec", type=int, default=0,
            help="Timeout for continuous mode (microseconds portion)")
        parser.add_argument(
            "--timeout-msec", type=int, default=0,
            help="Timeout for continuous mode (milliseconds portion)")
        parser.add_argument(
            "--timeout-sec", type=int, default=0,
            help="Timeout for continuous mode (seconds portion)")
        parser.add_argument(
            "--timeout-min", type=int, default=0,
            help="Timeout for continuous mode (minutes portion)")
        parser.add_argument("--timeout-hour",
                            type=int, default=0,
                            help="Timeout for continuous mode (hours portion)")
        parser.add_argument("-T", "--freq",
                            type=int,
                            help="Clock frequency (used for bw measurements)")
        parser.add_argument("-V", "--csv",
                            default=False,
                            action='store_true',
                            help="Comma separated value format")
        parser.add_argument("--suppress-hdr",
                            default=False,
                            action='store_true',
                            help="Suppress column headers")
        parser.add_argument("--suppress-stats",
                            default=False,
                            action='store_true',
                            help="Show stas at end")

        self.args, _ = self._parser.parse_known_args()
        if self.args.help:
            self._parser.print_help()
            return False
        if self.args.end is None:
            self.args.end = self.args.begin
        return True

    def run(self, handle, args=None):
        if args is None:
            args = self.args
        dsm_csr = csr.csr(offset=self.DSM_ADDR, width=64)
        src_csr = csr.csr(offset=self.SRC_ADDR, width=64)
        dst_csr = csr.csr(offset=self.DST_ADDR, width=64)
        nl_csr = csr.csr(offset=self.NUM_LINES, width=64)
        cfg = nlb.CFG(width=32)
        ctl = nlb.CTL(width=32)
        if str(args.cache_policy) == "wrpush-I":
            cfg["wrpush_i"] = int(args.cache_policy)
        else:
            cfg["wrthru_en"] = int(args.cache_policy)
        cfg["rdsel"] = int(args.cache_hint)
        cfg["rd_chsel"] = int(args.read_vc)
        cfg["wr_chsel"] = int(args.write_vc)
        if args.wrfence_vc is None:
            cfg["wf_chsel"] = int(self.wf_chsel[str(args.write_vc)])
        else:
            cfg["wf_chsel"] = int(args.wrfence_vc)
        dsm = fpga.allocate_shared_buffer(handle, int(KiB(4)))
        # allocate the smallest possible workspace for DSM, SRC, DST
        bsize = args.end*CACHELINE_BYTES
        scratch = None
        if KiB(2) >= bsize or(
                KiB(4) < bsize and MiB(1) >= bsize) or(
                MiB(2) < bsize and MiB(512) > bsize):
            scratch = fpga.allocate_shared_buffer(handle, bsize*2)
            src = scratch[:bsize]
            dst = scratch[bsize:]
            src_io_addr = scratch.io_address()
            dst_io_addr = src_io_addr+bsize
        else:
            src = fpga.allocate_shared_buffer(handle, bsize)
            dst = fpga.allocate_shared_buffer(handle, bsize)
            src_io_addr = src.io_address()
            dst_io_addr = dst.io_address()

        ctl.write(handle, reset=0)
        ctl.write(handle, reset=1)
        dsm_csr.write(handle, value=dsm.io_address())
        src_csr.write(handle, value=cl_align(src_io_addr))
        dst_csr.write(handle, value=cl_align(dst_io_addr))
        if args.cont:
            cfg["cont"] = 1
        cfg.write(handle)

        for i in range(args.begin, args.end+1, args.multi_cl):
            ctl.write(handle, reset=0)
            ctl.write(handle, reset=1)
            nl_csr.write(handle, value=i)
            ctl.write(handle, reset=1, start=1)
            if args.cont:
                time.sleep(to_seconds(args))
            else:
                self.wait_for_dsm(dsm)
            ctl.write(handle, stop=1)

        return dsm

    def wait_for_dsm(self, dsm):
        begin = time.time()
        while dsm[self.DSM_COMPLETE] & 0x1 == 0:
            if time.time() - begin > self.DSM_TIMEOUT:
                logging.error("Timeout waiting for DSM")
                return False
            time.sleep(0.001)
        return True
