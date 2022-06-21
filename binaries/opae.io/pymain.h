// Copyright(c) 2020-2021, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

const char *pymain = R"script(
import argparse
import datetime
import logging
import os
import pdb
import sys
import libvfio

from opae.io import utils, pci
from opae.io.utils import Path

def default_parser():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('command', nargs='?')
    parser.add_argument('-d', '--device', type=pci.pci_address)
    parser.add_argument('-r', '--region', type=int, default=0)
    parser.add_argument('-a', '--access-mode', type=int, default=64, choices=[64, 32])
    parser.add_argument('--version', action='store_true', default=False)
    parser.add_argument('-h', '--help', action='store_true', default=False)
    parser.add_argument('--pdb', action='store_true', default=False)
    return parser


class base_action(object):
    open_device = False

    def __init__(self, device=None, region=None, command=None):
        self.device = device
        self.region = region
        self.command = command
        self.parser = argparse.ArgumentParser(prog=command)
        self.add_args()
        if self.open_device:
            cli.update_device(device, region)

    def __call__(self, args):
        return_code = 0
        try:
            self.execute(self.parse_args(args))
        except SystemExit as err:
            return_code = err.code if isinstance(err.code, int) else os.EX_USAGE
        except OSError as oserr:
            LOG.exception(oserr)
            return_code = oserr.errno

        cli.return_code(return_code)

    def parse_args(self, args):
        return self.parser.parse_args(args)

    def add_args(self):
        pass

    def execute(self, args):
        raise NotImplemtedError('action not implemented')

class ls_action(base_action):
    def add_args(self):
        self.parser.add_argument('-v', '--viddid', default={}, type=pci.vendev)
        self.parser.add_argument('-s', '--sub-viddid', default={}, type=pci.vendev)
        self.parser.add_argument('--all', action='store_true', default=False)
        self.parser.add_argument('--system-class', action='store_true', default=False)

    def execute(self, args):
        kwargs = args.viddid
        kwargs.update(dict((f'subsystem_{k}', v)
                           for k,v in args.sub_viddid.items()))
        utils.ls(all=args.all, system_class=args.system_class, **kwargs)
        raise SystemExit(0)

class init_action(base_action):
    def add_args(self):
        self.parser.add_argument('user_group', default='root:root', nargs='?')

    def execute(self, args):
        if not self.device:
            raise SystemExit('Need device for init.')
        utils.vfio_init(self.device, args.user_group)
        raise SystemExit(0)

class release_action(base_action):
    def add_args(self):
        pass

    def execute(self, args):
        if not self.device:
            raise SystemExit('Need device for release.')
        utils.vfio_release(self.device)
        raise SystemExit(0)

class peek_action(base_action):
    open_device = True

    def add_args(self):
        self.parser.add_argument('offset', type=utils.hex_int)

    def execute(self, args):
        if not self.device:
            raise SystemExit('Need device for peek.')
        if not self.region:
            raise SystemExit('Need region for peek.')
        rd = self.region.read64 if utils.ACCESS_MODE == 64 else self.region.read32
        print('0x{:0x}'.format(rd(args.offset)))
        raise SystemExit(0)

class poke_action(base_action):
    open_device = True

    def add_args(self):
        self.parser.add_argument('offset', type=utils.hex_int)
        self.parser.add_argument('value', type=utils.hex_int)

    def execute(self, args):
        if not self.device:
            raise SystemExit('Need device for poke.')
        if not self.region:
            raise SystemExit('Need region for poke.')
        wr = self.region.write64 if utils.ACCESS_MODE == 64 else self.region.write32
        wr(args.offset, args.value)
        raise SystemExit(0)

class script_action(base_action):
    open_device = True

    def parse_args(self, args):
        return args

    def execute(self, args):
        g = dict(globals())
        with open(self.command) as f:
            sys.argv = [self.command] + args
            exec(f.read(), g, g)
        raise SystemExit(0)


class walk_action(base_action):
    open_device = True

    def add_args(self):
        self.parser.add_argument('--offset', nargs='?', type=utils.hex_int, default=0)
        self.parser.add_argument('-u', '--show-uuid', action='store_true', default=False)
        self.parser.add_argument('-D', '--dump', action='store_true', default=False)
        self.parser.add_argument('-c', '--count', type=int, default=None)
        self.parser.add_argument('-y', '--delay', type=int, default=None)

    def execute(self, args):
        if not self.device:
            raise SystemExit('walk requires device.')
        if not self.region:
            raise SystemExit('walk requires region.')

        offset = 0 if args.offset is None else args.offset
        utils.walk(self.region, offset, args.show_uuid, args.count, args.delay, args.dump)


class dump_action(base_action):
    open_device = True

    def add_args(self):
        self.parser.add_argument('--offset', nargs='?', type=utils.hex_int, default=0)
        self.parser.add_argument('-o', '--output', type=argparse.FileType('wb'), default=sys.stdout)
        self.parser.add_argument('-f', '--format', choices=['bin', 'hex'], default='hex')
        self.parser.add_argument('-c', '--count', type=int, default=None)

    def execute(self, args):
        if not self.device:
            raise SystemExit('walk requires device.')
        if not self.region:
            raise SystemExit('walk requires region.')

        offset = 0 if args.offset is None else args.offset
        utils.dump(self.region, offset, args.output, args.format, args.count)


class no_action(base_action):
    open_device = True

    def __call__(self, args):
        pass


actions = {
    'ls': ls_action,
    'init': init_action,
    'release': release_action,
    'peek': peek_action,
    'poke': poke_action,
    'walk': walk_action,
    'dump': dump_action,
}

def do_action(action, args):
    try:
        action.validate()
    except ValueError as err:
        print(err)
        return os.EX_USAGE
    try:
        action(args)
    except SystemExit as err:
        return err.code
    return 0


def show_help():
    help_msg = '''
  opae.io - peek and poke FPGA CSRs
      "opae.io"
      "opae.io -v | --version"
      "opae.io -h | --help"
      "opae.io ls [-v | --viddid <VID:DID>] [-s | --sub-viddid <SVID:SDID>] [--all] [--system-class]"
      "opae.io [-d | --device <PCI_ADDRESS>] [-r | --region <REGION_NUMBER>] [-a <ACCESS_MODE>] [init | release | peek | poke | <script> [arg1...argN]]
      "opae.io init [-d <PCI_ADDRESS>] <USER>[:<GROUP>]"
      "opae.io release [-d <PCI_ADDRESS>]"
      "opae.io [-d <PCI_ADDRESS>]"
      "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>]"
      "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] walk [<OFFSET>] [-u | --show-uuid]"
      "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] dump [<OFFSET>] [-o | --output <FILE>] [-f | --format (hex, bin)] [ -c | --count <WORD COUNT>]
      "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] peek <OFFSET>"
      "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] poke <OFFSET> <VALUE>"
      "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] <SCRIPT> <ARG1> <ARG2> ... <ARGN>"

  NOTE
  If -d or --device is omitted, opae.io will attempt to open the first device found.
  If -r or --region is omitted, opae.io will default to region 0.

  EXAMPLES
  Enumerating FPGA's:
            "$ opae.io ls"

  Initiating a session:

            "$ sudo opae.io init -d 0000:00:00.0 lab:lab" 

  Terminating a session:

            "$ sudo opae.io release -d 0000:00:00.0" 

  Entering an interactive Python environment:

            "$ opae.io -d 0000:00:00.0 -r 0" 

  Peek & Poke from the command line:

            "$ opae.io -d 0000:00:00.0 -r 0 peek 0x28" 
            "$ opae.io -d 0000:00:00.0 -r 0 poke 0x28 0xbaddecaf" 

  Executing a script:

            "$ opae.io -d 0000:00:00.0 -r 0 script.py a b c" 

'''.strip()
    print(help_msg)
    cli.return_code(0)


def get_action(args):
    action_class = None
    if args.command is None:
        action_class = no_action
    else:
        if Path(args.command).is_file():
            action_class = script_action
        elif args.command in actions:
            action_class = actions[args.command]
        else:
            return None

    if not action_class.open_device:
        return action_class(args.device, command='opae.io {}'.format(args.command))
    try:
        device = utils.find_device(args.device)
    except OSError as err:
        cli.return_code(err.errno)
        device = None
    region = utils.find_region(device, args.region) if device else None
    return action_class(device, region, args.command)


def setup_logging():
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    h = logging.FileHandler(f'/tmp/opae.io-{stamp}.log')
    h.setFormatter(logging.Formatter('[%(asctime)-15s] [%(levelname)s] %(message)s'))
    l = logging.getLogger('opae.io')
    l.setLevel(logging.DEBUG)
    l.addHandler(h)
    stdout_h = logging.StreamHandler(sys.stdout)
    stdout_h.setLevel(logging.INFO)
    l.addHandler(stdout_h)
    return l


def main(argv=None):
    setattr(builtins, 'LOG', setup_logging())
    prog, major, minor, patch = version()
    version_str = '{prog} {major}.{minor}.{patch}'.format(**locals())
    LOG.info(version_str)
    if argv is None:
        argv = sys.argv[1:]

    parser = default_parser()
    args, rest = parser.parse_known_args(argv)

    if not args or args.help or (argv == rest and len(rest)):
        show_help()
        return

    if args.version:
        print(version_str)
        cli.return_code(0)
        return

    action = get_action(args)
    if action is None:
        show_help()
    else:
        utils.ACCESS_MODE = args.access_mode
        action(rest)

if __name__ == '__main__':
    main()

)script";
