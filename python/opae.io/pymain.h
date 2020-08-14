#pragma once

const char *pymain = R"script(
import argparse
import logging
import os
import pdb
import sys
import libvfio

from logging.handlers import TimedRotatingFileHandler
from opae.io import utils
from opae.io.utils import Path

def default_parser():
    parser = argparse.ArgumentParser(add_help=False)

    parser.add_argument('command', nargs='?')
    parser.add_argument('-p', '--pci-address', type=utils.pci_address)
    parser.add_argument('-r', '--region', type=int, default=0)
    parser.add_argument('--version', action='store_true', default=False)
    parser.add_argument('-h', '--help', action='store_true', default=False)
    parser.add_argument('--pdb', action='store_true', default=False)
    return parser


class base_action(object):
    no_device = False

    def __init__(self, device=None, region=None, command=None):
        self.device = device
        self.region = region
        self.command = command
        self.parser = argparse.ArgumentParser(prog=command)
        self.add_args()
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
    no_device = True

    def add_args(self):
        self.parser.add_argument('-d', '--device', default={}, type=utils.vendev)
        self.parser.add_argument('--all', action='store_true', default=False)

    def execute(self, args):
        utils.ls(all=args.all, **args.device)

class init_action(base_action):
    def execute(self, args):
        pass

class release_action(base_action):
    def execute(self, args):
        pass

class peek_action(base_action):
    def add_args(self):
        self.parser.add_argument('offset', type=utils.hex_int)

    def execute(self, args):
        if not self.region:
            raise SystemExit('Need region for peek')
        print('0x{:0x}'.format(self.region.read64(args.offset)))

class poke_action(base_action):
    def add_args(self):
        self.parser.add_argument('offset', type=utils.hex_int)
        self.parser.add_argument('value', type=utils.hex_int)

    def execute(self, args):
        if not self.region:
            raise SystemExit('Need region for peek')
        self.region.write64(args.offset, args.value)


class script_action(base_action):
    def parse_args(self, args):
        return args

    def execute(self, args):
        g = dict(globals())
        with open(self.command) as f:
            sys.argv = [self.command] + args
            exec(f.read(), g, g)
        raise SystemExit(0)


class walk_action(base_action):
    def add_args(self):
        self.parser.add_argument('offset', nargs='?', type=utils.hex_int)
        self.parser.add_argument('-u', '--show-uuid', action='store_true', default=False)

    def execute(self, args):
        if self.device is None:
            raise SystemExit('walk requires device')

        if self.region is None:
            raise SystemExit('walk requires region')

        offset = 0 if args.offset is None else args.offset
        for offset, dfh in utils.dfh_walk(offset=offset):
            print('offset: 0x{:04x}'.format(offset))
            print('    dfh: {}'.format(dfh))
            if args.show_uuid:
                print('    uuid: {}'.format(utils.read_guid(offset+0x8)))
                

        

class no_action(base_action):
    def __call__(self, args):
        pass


actions = {
    'ls': ls_action,
    'init': init_action,
    'release': release_action,
    'peek': peek_action,
    'poke': poke_action,
    'walk': walk_action,
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
  opae-io - peek and poke FPGA CSRs
      "opae-io" 
      "opae-io -v | --version" 
      "opae-io -h | --help" 
      "opae-io ls" 
      "opae-io [-d | --device <PCI_ADDRESS>] [-r | --region <REGION_NUMBER>] [init | release | peek | poke | <script> [arg1...argN]]
      "opae-io init [-d <PCI_ADDRESS>] <USER>[:<GROUP>]" 
      "opae-io release [-d <PCI_ADDRESS>]" 
      "opae-io [-d <PCI_ADDRESS>]" 
      "opae-io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>]" 
      "opae-io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] peek <OFFSET>" 
      "opae-io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] poke <OFFSET> <VALUE>" 
      "opae-io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] <SCRIPT> <ARG1> <ARG2> ... <ARGN>" 

  NOTE
  If -d or --device is omitted, opae-io will attempt to open the first device found.
  If -r or --region is omitted, opae-io will default to region 0.

  EXAMPLES
  Enumerating FPGA's:
            "$ opae-io ls"

  Initiating a session:
            
            "$ sudo opae-io init -d 0000:00:00.0 lab:lab" 
            

  Terminating a session:
            
            "$ sudo opae-io release -d 0000:00:00.0" 
            

  Entering an interactive Python environment:
            
            "$ opae-io -d 0000:00:00.0 0" 
            

  Peek & Poke from the command line:
            
            "$ opae-io -d 0000:00:00.0 -r 0 peek 0x28" 
            "$ opae-io -d 0000:00:00.0 -r 0 poke 0x28 0xbaddecaf" 
            

  Executing a script:
            
            "$ opae-io -d 0000:00:00.0 -r 0 script.py a b c" 
            
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

    if action_class.no_device:
        return action_class(command='opae-io {}'.format(args.command))
    try:
        device = utils.find_device(args.pci_address)
    except OSError as err:
        cli.return_code(err.errno)
        device = None
    region = utils.find_region(device, args.region) if device else None
    return action_class(device, region, args.command)


def setup_logging():
    h = TimedRotatingFileHandler('opae-io.log', when='midnight')
    h.setFormatter(logging.Formatter('[%(asctime)-15s] [%(levelname)s] %(message)s'))
    l = logging.getLogger('opae-io')
    l.setLevel(logging.DEBUG)
    l.addHandler(h)
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
        action(rest)

if __name__ == '__main__':
    main()

)script";


