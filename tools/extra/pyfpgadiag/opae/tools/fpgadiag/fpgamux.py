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
from fpgadiag import fpgadiag


def muxify(fn, mask):
    def write_csr(offset, value):
        fn(mask | offset, value)
    return write_csr


class fpgamux(fpgadiag):

    @classmethod
    def create(cls):
        parser = argparse.ArgumentParser(add_help=False)
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
        parser.add_argument('-h', '--help',
                            action='store_true',
                            default=False,
                            help='print help message and exit')

        args, _ = parser.parse_known_args()
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
            app_args = []
            for k, v in app_config.iteritems():
                if type(v) is bool:
                    if v:
                        app_args.append('--{}'.format(k))
                else:
                    app_args.extend(['--{}'.format(k), str(v)])
            app.write_csr32 = muxify(app.write_csr32, i << (18-bits))
            app.write_csr64 = muxify(app.write_csr64, i << (18-bits))
            app.setup(app_args)
            apps.append(app)
        return apps


def main():
    tests = fpgamux.create()
    threads = []
    for test in tests:
        threads.append(threading.Thread(target=test.run, args=(h, d)))

    for t in threads:
        t.join()

    return 0


if __name__ == '__main__':
    main()
