#! /usr/bin/env python
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
import argparse
import json
import logging
import math
import threading
import nlb
import sys

from os import EX_OK, EX_USAGE, EX_SOFTWARE, EX_UNAVAILABLE
from os.path import basename
from fpgadiag import fpgadiag
# pylint: disable=E0611
from opae import fpga


def mux_csrwrite(fn, mask):
    def write_csr(handle, offset, value):
        fn(handle, mask | offset, value)
    return write_csr


def mux_run(test, handle, device):
    # get counters
    c_counters = nlb.cache_counters(device)
    f_counters = nlb.fabric_counters(device)
    with c_counters.reader() as r:
        begin_cache = r.read()
    with f_counters.reader() as r:
        begin_fabric = r.read()
    cl, dsm = test.run(handle, None)
    # get counters
    with c_counters.reader() as r:
        end_cache = r.read()
    with f_counters.reader() as r:
        end_fabric = r.read()
    test.display_stats(cl, dsm, end_cache-begin_cache, end_fabric-begin_fabric)


class fpgamux(fpgadiag):

    @classmethod
    def create(cls):
        parser = argparse.ArgumentParser(add_help=True)
        parser.add_argument(
            '-c',
            '--config',
            help='config file listing mux configuration',
            type=argparse.FileType('r'),
            default='mux.json')
        parser.add_argument(
            '--loglevel',
            choices=[
                'exception',
                'error',
                'warning',
                'info',
                'debug'],
            default='warning',
            help='error level to set')
        parser.add_argument(
            '--version',
            help='print version information then quit',
            action='store_true',
            default=False)

        args, _ = parser.parse_known_args()
        if args.version:
            print(
                "{} OPAE {}, build {}".format(
                    basename(sys.argv[0]),
                    fpga.version(),
                    fpga.build()))
            sys.exit(EX_OK)

        logger = logging.getLogger("fpgadiag")
        stream_handler = logging.StreamHandler(stream=sys.stderr)
        stream_handler.setFormatter(logging.Formatter(
            '%(asctime)s: [%(name)-8s][%(levelname)-6s] - %(message)s'))
        logger.addHandler(stream_handler)
        logger.setLevel(getattr(logging, args.loglevel.upper()))

        config = json.load(args.config)
        app_classes = dict([(n.__name__, n)
                            for n in set(cls.mode_class.values())])
        bits = int(math.ceil(math.log(len(config), 2)))

        apps = []
        modes = {'nlb0': 'lpbk1', 'nlb7': 'sw'}
        for i, c in enumerate(config):
            app_name = c.get('app')
            if app_name not in app_classes:
                logging.warning("app not recognized: %s", app_name)
                continue
            if c['disabled']:
                logging.info('app %s is disabled', app_name)
                continue
            app_parser = argparse.ArgumentParser(add_help=False)
            app_parser.add_argument('-m', '--mode')
            app_parser.add_argument('-h', '--help',
                                    action='store_true',
                                    default=False)
            app_mode = c['config'].get('mode', modes.get(app_name))
            app = app_classes[app_name](app_mode, app_parser)
            app_config = c['config']
            app_args = ['--mode', app_mode]
            for k, v in app_config.iteritems():
                if type(v) is bool:
                    if v:
                        app_args.append('--{}'.format(k))
                else:
                    app_args.extend(['--{}'.format(k), str(v)])
            app.write_csr32 = mux_csrwrite(app.write_csr32, i << (18-bits))
            app.write_csr64 = mux_csrwrite(app.write_csr64, i << (18-bits))
            if not app.setup(app_args):
                sys.exit(EX_USAGE)
            apps.append(app)
        return apps


def main():
    tests = fpgamux.create()
    threads = []
    tokens = tests[0].enumerate()
    if not tokens:
        tests[0].logger.error("Could not find suitable accelerator")
        sys.exit(EX_UNAVAILABLE)
    with fpga.open(tokens[0]) as h:
        parent = fpga.properties(h).parent
        with fpga.open(parent, fpga.OPEN_SHARED) as d:
            for test in tests:
                threads.append(
                        threading.Thread(
                            target=mux_run, args=(test, h, d)))

            for t in threads:
                t.start()
            for t in threads:
                t.join()

        return EX_OK
    return EX_SOFTWARE


if __name__ == '__main__':
    main()
