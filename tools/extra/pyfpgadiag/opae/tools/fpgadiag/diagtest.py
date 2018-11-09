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
import sys
import time
import nlb
from datetime import timedelta
from opae.utils import csr, parse_int, cl_align, CACHELINE_BYTES
from opae.utils.byteutils import KiB, MiB
# pylint: disable=E0611
from opae import fpga


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
    STATUS2 = 0x0170
    DSM_COMPLETE = 0x40
    DSM_TIMEOUT_USEC = 10000
    MODE_LPBK1 = "lpbk1"
    MODE_READ = "read"
    MODE_WRITE = "write"
    MODE_TRPUT = "trput"
    MODE_SW = "sw"

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
    modes = csr.f_enum(name="mode", lbpk1=0, read=1, write=2, trput=3, sw=7)

    def __init__(self, mode, parser):
        self._mode = mode
        self.cfg = nlb.CFG(width=32)
        self.ctl = nlb.CTL(width=32)
        self._parser = parser
        self.logger = logging.getLogger(self.__class__.__name__)
        stream_handler = logging.StreamHandler(stream=sys.stderr)
        stream_handler.setFormatter(logging.Formatter(
            '%(asctime)s: [%(name)-8s][%(levelname)-6s] - %(message)s'))
        self.logger.addHandler(stream_handler)

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
            '--segment',
            help='PCIe segment number',
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
        prop_names = [
            'bus',
            'device',
            'function',
            'segment',
            'socket_id',
            'guid']
        filt = dict([(p, getattr(args, p))
                     for p in prop_names if getattr(args, p)])
        tokens = fpga.enumerate(type=fpga.ACCELERATOR, **filt)
        if not tokens:
            self.logger.error("could not find resources")
            return None
        if len(tokens) > 1:
            self.logger.warn("found more than one accelerator")
        return tokens

    def add_arguments(self, parser):
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
        parser.add_argument(
            "--dsm-timeout-usec",
            type=int,
            help="Timeout for test completion")
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
                            type=int, default=400000000,
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
                            help="Hide stats at end")
        parser.add_argument(
            "--mem-timeout", default=0.5, type=float,
            help="Seconds to wait before timing out on memory poll")

    def setup(self, in_args=None):
        """setup is called to validate arguments and will return True
           if arguments are valid, False otherwise."""
        parser = self._parser.add_argument_group(self._mode)
        self.add_arguments(parser)
        self.args, _ = self._parser.parse_known_args(in_args)
        if self.args.end is None:
            self.args.end = self.args.begin
        return True

    def configure_test(self):
        """configure_test is used to configure the configuration register
           and to perform any other test configuration steps before the test
           starts"""
        args = self.args

        if self.args.target == 'ase':
            self.DSM_TIMEOUT_USEC = 10000000

        if args.dsm_timeout_usec:
            self.DSM_TIMEOUT_USEC = args.dsm_timeout_usec

        self.cfg["mode"] = int(self.modes(args.mode))

        if str(args.cache_policy) == "wrpush-I":
            self.cfg["wrpush_i"] = int(args.cache_policy)
        else:
            self.cfg["wrthru_en"] = int(args.cache_policy)
        self.cfg["rdsel"] = int(args.cache_hint)
        self.cfg["rd_chsel"] = int(args.read_vc)
        self.cfg["wr_chsel"] = int(args.write_vc)
        if args.wrfence_vc is None:
            self.cfg["wf_chsel"] = int(self.wf_chsel[str(args.write_vc)])
        else:
            self.cfg["wf_chsel"] = int(args.wrfence_vc)

        if args.cont:
            self.cfg["cont"] = 1

        self.cfg["multiCL_len"] = args.multi_cl - 1

    def buffer_size(self):
        """buffer_size is used to get the number of bytes necessary for each
           test buffer (src and dst)."""
        return self.args.end*CACHELINE_BYTES

    def get_buffers(self, handle):
        """get_buffers gets the three basic buffers needed for test.
           dsm (device status memory), src (source or input),
           and dst (destination or output).

        :param handle: Use given handle for allocating buffers and preparing
                       for use with accelerator
        """
        bsize = self.buffer_size()
        dsm = fpga.allocate_shared_buffer(handle, int(KiB(4)))
        # allocate the smallest possible workspace for DSM, SRC, DST
        scratch = None

        if KiB(2) >= bsize or(
                KiB(4) < bsize and MiB(1) >= bsize) or(
                MiB(2) < bsize and MiB(512) > bsize):
            scratch = fpga.allocate_shared_buffer(handle, bsize*2)
            src, dst = scratch.split(bsize, bsize)
        else:
            src = fpga.allocate_shared_buffer(handle, bsize)
            dst = fpga.allocate_shared_buffer(handle, bsize)
        return (dsm, src, dst)

    def run(self, handle, device):
        """run Run the test flow which consists of:
            1. Allocating buffers
            2. Initializing or priming the buffers
            3. Telling the accelerator about the buffers' io addresses
            4. Setting up the configuration register based on mode/arguments
            5. Writing the configuration data to the register
            6. Begin iterating on the number of cachelines as indicated by
               begin and end arguments. Each iteration will write to the
               control register to start and then test/wait on buffers like
               DSM before continuing to the next iteration. Counters will also
               be read at the beginning and at the end of the iteration and
               their deltas will be used when printing out statistics.

        :param handle: A handle to an accelerator
        :param device: A handle to the device object
        """
        args = self.args
        ctl = self.ctl
        # TODO: replace with get_user_clocks API
        status2 = handle.read_csr64(self.STATUS2)
        freq = (status2 >> 32) & 0xffff
        if freq > 0:
            args.freq = freq * 1E6

        self.logger.info("allocating bufers")
        (dsm, src, dst) = self.get_buffers(handle)

        self.logger.info("setup buffers")
        self.setup_buffers(handle, dsm, src, dst)

        self.logger.info("writing dsm address")
        self.write_csr64(handle, self.DSM_ADDR, dsm.io_address())

        self.logger.info("deassert/assert reset bit")
        self.write_csr32(handle, ctl.offset(), ctl.value(reset=0))
        self.write_csr32(handle, ctl.offset(), ctl.value(reset=1))

        self.logger.info(
            "writing src/dst buffer addresses (cacheline aligned)")
        self.write_csr64(handle, self.SRC_ADDR, cl_align(src.io_address()))
        self.write_csr64(handle, self.DST_ADDR, cl_align(dst.io_address()))

        self.logger.info("configuring test values to write")
        self.configure_test()
        self.logger.info("writing cfg register")
        self.write_csr32(handle, self.cfg.offset(), self.cfg.value())

        c_counters = nlb.cache_counters(device)
        f_counters = nlb.fabric_counters(device)

        dsm_tpl = nlb.dsm_tuple()
        for i in range(args.begin, args.end+1, args.multi_cl):
            self.logger.debug("running test with cl: %s", i)
            dsm.fill(0)
            self.logger.info("deassert/assert reset bit")
            self.write_csr32(handle, ctl.offset(), ctl.value(reset=0))
            self.write_csr32(handle, ctl.offset(), ctl.value(reset=1))

            self.logger.info("writing number of cachelines")
            self.write_csr64(handle, self.NUM_LINES, i)

            self.logger.info("getting begin counters")
            with c_counters.reader() as r:
                begin_cache = r.read()

            with f_counters.reader() as r:
                begin_fabric = r.read()

            self.logger.debug("starting test")
            self.write_csr32(handle, ctl.offset(), ctl.value(reset=1, start=1))

            self.logger.info("testing buffers")
            self.test_buffers(handle, i, dsm, src, dst)
            self.logger.info("stopping test")

            handle.write_csr32(
                ctl.offset(), ctl.value(
                    stop=1, reset=1, start=1))

            self.logger.info("getting end counters")
            with c_counters.reader() as r:
                end_cache = r.read()

            with f_counters.reader() as r:
                end_fabric = r.read()

            if args.suppress_stats:
                dsm_tpl += nlb.dsm_tuple(dsm)
            else:
                self.display_stats(i,
                                   nlb.dsm_tuple(dsm),
                                   end_cache-begin_cache,
                                   end_fabric-begin_fabric)

            self.logger.info("validating results")
            self.validate_results(i, dsm, src, dst)
            self.logger.info("end of test")

    def test_buffers(self, handle, cachelines, dsm, src, dst):
        """test_buffers The base version of test_buffers only waits for the
                         DSM complete bit to be set or for the timeout period
                         if continuous mode is selected.

        :param handle: A handle to the accelerator being tested
        :param cachelines: The number of cachelines for the current iteration.
        :param dsm: The DSM buffer
        :param src: The source or input buffer
        :param dst: The destination or output buffer
        """
        if self.args.cont:
            time.sleep(to_seconds(self.args))
        else:
            self.wait_for_dsm(dsm)

    def setup_buffers(self, handle, dsm, src, dst):
        src.fill(0xAE)
        dst.fill(0x00)

    def validate_results(self, num_cl, dsm, src, dst):
        pass

    def wait_for_dsm(self, dsm):
        if not dsm.poll(self.DSM_COMPLETE, 0x1, mask=0x1,
                        timeout_usec=self.DSM_TIMEOUT_USEC):
            self.logger.error("Timeout waiting for DSM")
            return False
        return True

    def display_stats(self, cachelines, dsm, cache, fabric):
        stats = nlb.nlb_stats(
            cachelines,
            dsm,
            cache,
            fabric,
            frequency=self.args.freq,
            cont=self.args.cont)
        if self.args.csv:
            print(stats.to_csv(not self.args.suppress_hdr))
        else:
            print(stats.to_str())

    def write_csr32(self, handle, offset, value):
        self.logger.debug(
            "write_csr32(0x{:016x}, 0x{:08x})".format(
                offset, value))
        handle.write_csr32(offset, value)

    def write_csr64(self, handle, offset, value):
        self.logger.debug(
            "write_csr64(0x{:016x}, 0x{:016x})".format(
                offset, value))
        handle.write_csr64(offset, value)
