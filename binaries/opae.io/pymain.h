// Copyright(c) 2020-2023, Intel Corporation
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
import sys
import libvfio
import uuid

from opae.io import utils, pci
from opae.io.utils import Path


class base_action(object):
    open_device = False

    def __init__(self, device=None, region=None, command=None):
        self.device = device
        self.region = region
        self.command = command
        if self.open_device:
            cli.update_device(device, region)

    def __call__(self, args):
        return_code = 0
        try:
            self.execute(args)
        except SystemExit as err:
            return_code = err.code if isinstance(err.code, int) else os.EX_USAGE
        except OSError as oserr:
            LOG.exception(oserr)
            return_code = oserr.errno

        cli.return_code(return_code)

    def execute(self, args):
        raise NotImplemtedError('action not implemented')


class ls_action(base_action):
    @staticmethod
    def add_subparser(subparser):
        ls = subparser.add_parser('ls')
        ls.add_argument('-v', '--viddid', default={}, type=pci.vendev,
                        help='the VID:DID of the desired PCIe device')
        ls.add_argument('-s', '--sub-viddid', default={}, type=pci.vendev,
                        help='the SVID:SDID of the desired PCIe device')
        ls.add_argument('--all', action='store_true', default=False,
                        help='list ALL PCIe devices')
        ls.add_argument('--system-class', action='store_true', default=False,
                        help='display the PCIe database class for the device')

    def execute(self, args):
        kwargs = args.viddid
        kwargs.update(dict((f'subsystem_{k}', v)
                           for k,v in args.sub_viddid.items()))
        utils.ls(all=args.all, system_class=args.system_class, **kwargs)
        raise SystemExit(0)


class init_action(base_action):
    @staticmethod
    def add_subparser(subparser):
        init = subparser.add_parser('init')
        init.add_argument('-d', '--device', dest='sdevice',
                          metavar='DEVICE', type=pci.pci_address,
                          help='the PCIe address of the FPGA device')
        init.add_argument('-e', '--enable-sriov', action='store_true',
                          default=False,
                          help='enable SR-IOV during initialization')
        init.add_argument('-f', '--force', action='store_true',
                          default=False,
                          help='force the driver to unbind, '
                               'even if saving the previous driver fails')
        init.add_argument('user_group', nargs='?', default='root:root',
                          help='the user:group for assigning device permissions')

    def execute(self, args):
        if not self.device:
            raise SystemExit('Need device for init.')

        kw = {'enable_sriov': args.enable_sriov}
        utils.vfio_init(self.device, args.user_group, force=args.force, **kw)
        raise SystemExit(0)


class release_action(base_action):
    @staticmethod
    def add_subparser(subparser):
        release = subparser.add_parser('release')
        release.add_argument('-d', '--device', dest='sdevice',
                             metavar='DEVICE', type=pci.pci_address,
                             help='the PCIe address of the FPGA device')

    def execute(self, args):
        if not self.device:
            raise SystemExit('Need device for release.')

        utils.vfio_release(self.device)
        raise SystemExit(0)


class peek_action(base_action):
    open_device = True

    @staticmethod
    def add_subparser(subparser):
        peek = subparser.add_parser('peek')
        peek.add_argument('-d', '--device', dest='sdevice',
                          metavar='DEVICE', type=pci.pci_address,
                          help='the PCIe address of the FPGA device')
        peek.add_argument('-r', '--region', dest='sregion',
                          metavar='REGION', type=int, default=0,
                          help='the MMIO region of the FPGA device')
        peek.add_argument('offset', type=utils.hex_int,
                          help='the register offset to peek')

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

    @staticmethod
    def add_subparser(subparser):
        poke = subparser.add_parser('poke')
        poke.add_argument('-d', '--device', dest='sdevice',
                          metavar='DEVICE', type=pci.pci_address,
                          help='the PCIe address of the FPGA device')
        poke.add_argument('-r', '--region', dest='sregion',
                          metavar='REGION', type=int, default=0,
                          help='the MMIO region of the FPGA device')
        poke.add_argument('offset', type=utils.hex_int,
                          help='the register offset to poke')
        poke.add_argument('value', type=utils.hex_int,
                          help='the value to poke into the register')

    def execute(self, args):
        if not self.device:
            raise SystemExit('Need device for poke.')
        if not self.region:
            raise SystemExit('Need region for poke.')

        wr = self.region.write64 if utils.ACCESS_MODE == 64 else self.region.write32
        wr(args.offset, args.value)
        raise SystemExit(0)


class vf_token_action(base_action):
    open_device = True

    @staticmethod
    def add_subparser(subparser):
        vf_token = subparser.add_parser('vf_token')
        vf_token.add_argument('-d', '--device', dest='sdevice',
                              metavar='DEVICE', type=pci.pci_address,
                              help='the PCIe address of the FPGA device')
        vf_token.add_argument('vftoken', default=None,
                              help='the token (GUID) to use')

    def execute(self, args):
        if not self.device:
            raise SystemExit('Need device for vf_token.')

        try:
            token_uuid = uuid.UUID(args.vftoken)
            ret = self.device.set_vf_token(token_uuid.bytes)
            if ret:
                print('Failed to set vf_token')
                raise SystemExit(ret)
            print(f'Successfully set vf_token to {str(token_uuid)}')
        except ValueError:
            print('Invalid vf_token')
            raise SystemExit(1)

        raise SystemExit(0)


class script_action(base_action):
    open_device = True

    @staticmethod
    def add_subparser(subparser):
        script = subparser.add_parser('script')
        script.add_argument('-d', '--device', dest='sdevice',
                            metavar='DEVICE', type=pci.pci_address,
                            help='the PCIe address of the FPGA device')
        script.add_argument('-r', '--region', dest='sregion',
                            metavar='REGION', type=int, default=0,
                            help='the MMIO region of the FPGA device')
        script.add_argument('script_file', help='path to the desired script')
        script.add_argument('script_args', nargs='*', default=[],
                            help='script arguments')

    def execute(self, args):
        if not self.device:
            raise SystemExit('Need device for script.')
        if not self.region:
            raise SystemExit('Need region for script.')

        p = Path(args.script_file)
        if not p.exists() or not p.is_file():
            raise SystemExit(f'{args.script_file} does not exist or is not a file.')

        with open(args.script_file) as f:
            sys.argv = [args.script_file] + args.script_args
            exec(f.read(), dict(globals()), {})

        raise SystemExit(0)


class walk_action(base_action):
    open_device = True

    @staticmethod
    def add_subparser(subparser):
        walk = subparser.add_parser('walk')
        walk.add_argument('-d', '--device', dest='sdevice',
                          metavar='DEVICE', type=pci.pci_address,
                          help='the PCIe address of the FPGA device')
        walk.add_argument('-r', '--region', dest='sregion',
                          metavar='REGION', type=int, default=0,
                          help='the MMIO region of the FPGA device')
        walk.add_argument('--offset', nargs='?',
                          type=utils.hex_int, default=0,
                          help='the start offset if any.')
        walk.add_argument('-u', '--show-uuid', action='store_true',
                          default=False,
                          help='display IDs in human-readable format')
        walk.add_argument('-D', '--dump', action='store_true',
                          default=False,
                          help='display the raw DFH contents')
        walk.add_argument('-c', '--count', type=int, default=None,
                          help='walk at most this number of DFH entries')
        walk.add_argument('-y', '--delay', type=int, default=None,
                          help='time to sleep after each printout')
        walk.add_argument('-s', '--safe', action='store_true',
                          default=False,
                          help='check offsets for alignment before reading MMIO')

    def execute(self, args):
        if not self.device:
            raise SystemExit('walk requires device.')
        if not self.region:
            raise SystemExit('walk requires region.')

        offset = 0 if args.offset is None else args.offset
        utils.walk(self.region, offset, args.show_uuid, args.count,
            args.delay, args.dump, args.safe)

        raise SystemExit(0)


class dump_action(base_action):
    open_device = True

    @staticmethod
    def add_subparser(subparser):
        dump = subparser.add_parser('dump')
        dump.add_argument('-d', '--device', dest='sdevice',
                          metavar='DEVICE', type=pci.pci_address,
                          help='the PCIe address of the FPGA device')
        dump.add_argument('-r', '--region', dest='sregion',
                          metavar='REGION', type=int, default=0,
                          help='the MMIO region of the FPGA device')
        dump.add_argument('--offset', nargs='?', type=utils.hex_int,
                          default=0,
                          help='starting MMIO offset to dump')
        dump.add_argument('-o', '--output', type=argparse.FileType('wb'),
                          default=sys.stdout,
                          help='file to receive MMIO dump')
        dump.add_argument('-f', '--format', choices=['bin', 'hex'],
                          default='hex',
                          help='output format')
        dump.add_argument('-c', '--count', type=int, default=None,
                          help='number of qwords to dump')

    def execute(self, args):
        if not self.device:
            raise SystemExit('dump requires device.')
        if not self.region:
            raise SystemExit('dump requires region.')

        offset = 0 if args.offset is None else args.offset
        utils.dump(self.region, offset, args.output, args.format, args.count)

        raise SystemExit(0)


class interactive_action(base_action):
    open_device = True

    @staticmethod
    def add_subparser(subparser):
        pass

    def __call__(self, args):
        raise SystemExit(0)


actions = {
    'ls': ls_action,
    'init': init_action,
    'release': release_action,
    'peek': peek_action,
    'poke': poke_action,
    'walk': walk_action,
    'dump': dump_action,
    'vf_token': vf_token_action,
    'script': script_action,
}


def show_legacy_help():
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
      "opae.io [-d <PCI_ADDRESS>] vf_token <GUID>"
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


def get_action(argv, args):
    action_class = None
    if args.which is None:
        action_class = interactive_action
    elif args.which in actions:
        action_class = actions[args.which]
    else:
        return None

    # Let the subparser's sdevice and sregion attributes
    # override the main parser's device and region
    # attributes.
    if hasattr(args, 'sdevice') and args.sdevice:
        dev = args.sdevice
    else:
        dev = args.device

    if hasattr(args, 'sregion') and args.sregion:
        reg = args.sregion
    else:
        reg = args.region

    command = ' '.join(argv)

    if not action_class.open_device:
        return action_class(device=dev, command=command)

    try:
        device = utils.find_device(dev)
        region = utils.find_region(device, reg) if device else None
    except OSError as err:
        cli.return_code(err.errno)
        device = None
        region = None

    return action_class(device=device, region=region, command=command)


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

    if argv is None:
        argv = sys.argv[1:]

    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('-d', '--device', type=pci.pci_address,
                        help='the PCIe address of the FPGA device')
    parser.add_argument('-r', '--region', type=int, default=0,
                        help='the MMIO region of the FPGA device')
    parser.add_argument('-a', '--access-mode', type=int, default=64,
                        choices=[64, 32],
                        help='access MMIO registers 32- or 64-bits at a time')
    parser.add_argument('-v', '--version', action='store_true', default=False,
                        help='display version information and exit')
    parser.add_argument('-h', '--help', action='store_true', default=False,
                        help='display help information and exit')
    parser.add_argument('--legacy-help', action='store_true', default=False,
                        help='display the legacy help and exit')
    subparser = parser.add_subparsers(dest='which')

    for k in actions.keys():
        actions[k].add_subparser(subparser)

    try:
        args = parser.parse_args()
    except SystemExit as exc:
        cli.return_code(100)
        return

    if not args or args.help:
        parser.print_help()
        cli.return_code(99)
        return

    if args.legacy_help:
        show_legacy_help()
        cli.return_code(99)
        return

    if args.version:
        print(version_str)
        cli.return_code(99)
        return

    action = get_action(argv, args)
    if action is None:
        parser.print_help()
    else:
        utils.ACCESS_MODE = args.access_mode
        action(args)

if __name__ == '__main__':
    main()

)script";
