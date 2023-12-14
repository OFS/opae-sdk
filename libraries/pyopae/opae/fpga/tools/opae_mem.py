#!/usr/bin/env python3
# Copyright(c) 2023, Intel Corporation
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

"""opae-mem provides a means to view the MMIO space of
   FPGA accelerators, whether connected to DFL, VFIO, UIO,
   or ASE.

   Devices are enumerated using the opae-mem ls command:

   $ sudo chmod 666 /dev/uio*
   $ opae-mem ls
   [0000:d8:00.0] (8086:bcce 8086:0000)
     UIO 0x14 00000000-0000-0000-0000-000000000000
     UIO 0x20 00000000-0000-0001-0000-000000000000
   [0000:3b:00.0] (8086:bcce 8086:0000)
     UIO 0x15 00042415-0004-2415-0000-001100010000
     UIO 0x20 00000000-0000-0001-0000-000000000000
     UIO 0x14 00000000-0000-0000-0000-000000000000

   This output shows two FPGA cards with a total of five FPGA
   MMIO regions.

   The first card at address 0000:d8:00.0 has two MMIO regions
   accessible via UIO:
     Feature ID 0x14: s10 IOPLL
     Feature ID 0x20: PCIe Subsystem

   The second card at address 0000:3b:00.0 has three MMIO regions
   accessible via UIO:
     Feature ID 0x15: HSSI Subsystem
     Feature ID 0x20: PCIe Subsystem
     Feature ID 0x14: s10 IOPLL

   The peek command provides a way to view a range of MMIO addresses:

   $ opae-mem peek -d 0000:3b:00.0 -f 0x20 --count 4 0x0
   00000000:  3000000010000020   0000000000000000  | ......0........|
   00000010:  0000000000000001   0000000000000000  |................|

   Here, --count 4 causes the command to display four 64-bit qwords,
   from addresses 0x0 through 0x18.

   The default format is hex display, which is modeled after the
   output of hexdump -C. The format can be controlled using the -F
   option. Valid values for -F are simple, hex, and json.

   $ opae-mem peek -d 0000:3b:00.0 -f 0x20 --count 4 -F simple 0x0
   00000000: 3000000010000020
   00000008: 0000000000000000
   00000010: 0000000000000001
   00000018: 0000000000000000

   $ opae-mem peek -d 0000:3b:00.0 -f 0x20 --count 4 -F json 0x0
   {
     "0x00000000": "0x3000000010000020",
     "0x00000008": "0x0000000000000000",
     "0x00000010": "0x0000000000000001",
     "0x00000018": "0x0000000000000000"
   }

   The output of opae-mem peek can be sent to a file using the -o
   option. This is useful for capture/playback. Playback is available
   with the opae-mem poke command.

   $ opae-mem peek -d 0000:3b:00.0 -f 0x20 --count 4 -F json -o file.json 0x0
   $ cat file.json
   {
     "0x00000000": "0x3000000010000020",
     "0x00000008": "0x0000000000000000",
     "0x00000010": "0x0000000000000001",
     "0x00000018": "0x0000000000000000"
   }

   The poke command provides a way to write MMIO addresses:

   $ opae-mem poke -d 0000:3b:00.0 -f 0x20 0x0 0xc0cac01a

   In the above poke command, 0x0 is the MMIO offset to write
   and 0xc0cac01a is the value.

   The output of the opae-mem peek command with -F json can
   be played back as a series of writes to an MMIO region
   using the opae-mem poke --json option:

   $ opae-mem poke -d 0000:3b:00.0 -f 0x20 --json file.json

   The opae-mem mb-read command is used to issue read requests
   to an FPGA mailbox interface:

   $ opae-mem mb-read -d 0000:3b:00.0 -f 0x20 -c 4 0x0
   mb-read [0000:3b:00.0] 0x20 This command needs -b mailbox_base. For feature_id 0x20, try -b 0x0028

   $ opae-mem mb-read -d 0000:3b:00.0 -f 0x20 -c 4 -b 0x28 0x0
   00000000:  01000000 00000001 01104000 00000000  |.........@......|

   Each mailbox address represents a 32-bit data value.

   Like peek, the default display format for mb-read is hex.
   To change the display format, use the -F option, which accepts
   simple, hex, or json.

   $ opae-mem mb-read -d 0000:3b:00.0 -f 0x20 -c 4 -b 0x28 -F simple 0x0
   00000000: 01000000
   00000004: 00000001
   00000008: 01104000
   0000000c: 00000000

   $ opae-mem mb-read -d 0000:3b:00.0 -f 0x20 -c 4 -b 0x28 -F json 0x0
   {
     "0x00000028": {
       "0x00000000": "0x01000000",
       "0x00000004": "0x00000001",
       "0x00000008": "0x01104000",
       "0x0000000c": "0x00000000"
     }
   }

   The mb-write command provides a way to issue write commands
   to an FPGA mailbox interface.

   $ opae-mem mb-write -d 0000:3b:00.0 -f 0x20 -b 0x28 0x0 0xc0cac01a

   In the above command, 0x0 is the mailbox address and 0xc0cac01a
   is the 32-bit data value to be written.

   The output of the opae-mem mb-read command with -F json can
   be played back as a series of writes to a mailbox interface
   using the opae-mem mb-write --json option:

   $ opae-mem mb-read -d 0000:3b:00.0 -f 0x20 -c 4 -b 0x28 -F json -o mb.json 0x0
   $ opae-mem mb-write -d 0000:3b:00.0 -f 0x20 -b 0x28 --json mb.json

   Each of the above commands has explicitly specified the
   -d PCIE_ADDR and -f FEATURE_ID parameters, making for some
   long command lines. To shorten the length, opae-mem can
   be "locked" to a (device, feature_id) pair:

   $ opae-mem lock -d 0000:3b:00.0 -f 0x20
   lock [0000:3b:00.0] 0x20 OK

   Once "locked" to a device, issuing the command again displays
   the lock status:

   $ opae-mem lock
   [locked 0000:3b:00.0 0x20] lock currently held by 0000:3b:00.0 0x20.

   From now until the time the session is unlocked,
   opae-mem commands may omit the explicit -d and -f
   parameters:

   $ opae-mem peek 0x0
   00000000:  3000000010000020                     | ......0        |

   "Locking" is simply a convenient way to shorten the opae-mem
   command line. Each of the other commands operates in the same
   way, as if -d and -f were specified explicitly.

   Note: a "lock" can be overridden by specifying -d and/or -f:

   $ opae-mem -V peek -d 0000:d8:00.0 -f 0x14 -c 4 0x0
   [locked 0000:3b:00.0 0x20 [override 0000:d8:00.0 0x14]] peek [0000:d8:00.0] 0x14 offset=0x0 region=0 format=hex
   00000000:  3000000010000014   0000000000000000  |.......0........|
   00000010:  0000000000000000   1000000000000000  |................|

   The preceding command used a lock override by specifying a
   different device address to -d and a different feature_id
   to -f. The -V (verbose) option was given to show the
   lock override status.

   The unlock command is used to release a lock:

   $ opae-mem unlock
   [locked 0000:3b:00.0 0x20] unlock Please tell me the device / feature id to unlock. (-d 0000:3b:00.0 -f 0x20)

   $ opae-mem unlock -d 0000:3b:00.0 -f 0x20
   [locked 0000:3b:00.0 0x20] unlock [0000:3b:00.0] 0x20 OK

   $ opae-mem lock
   lock Give me the device address and feature id.
"""

from argparse import ArgumentParser, FileType
import json
import os
import sys

from opae import fpga
from opae.fpga import dfh
from opae.fpga import feature
from opae.fpga import hexview
from opae.fpga import mailbox
import opae.fpga.pcie.address as addr

try:
    from pathlib import Path
except ImportError:
    from pathlib2 import Path  # noqa


VERSION = '0.0.1'

DEVICE_HELP = ('PCIe filter of device '
               '[segment:]bus:device.function'
               ' or \'vendor:device[ subvendor:subdevice]\'')


# {
#   "locked_by": {
#     "address": "0000:dc:00.0",
#     "feature_id": 32
#   }
# }
LOCK_FILE = os.path.join(Path.home(), '.local/etc/opae/opae-mem.json')


def enum_filter(addr_or_id):
    """Used as the type parameter for argparse.
       Converts a PCIe address or a PCIe id tuple to
       a dictionary containing the appropriate keys
       for an opae.fpga.properties object."""
    return addr.device_filter(addr_or_id, addr_or_id)


def get_lock_owner():
    """(address, feature_id)"""
    locker = (None, None)
    p = Path(LOCK_FILE)
    if p.exists() and p.is_file():
        with open(p, 'r', encoding='utf-8') as fp:
            try:
                contents = json.load(fp)
                locked_by = contents['locked_by']
                locker = (addr.pcie_address(locked_by['address']),
                          addr.hex_int(f'0x{locked_by["feature_id"]:0x}'))
            except json.JSONDecodeError as jde:
                print(f'{jde.msg} at {p} line {jde.lineno} col {jde.colno}')
                print(f'{p} is invalid or corrupt. Please fix or remove it.')
                sys.exit(1)
    return locker


def lock_override_requested(address: addr.pcie_address, feature_id: addr.hex_int, args):
    """Given the current lock owner (address, feature_id) and any filter specified
       during argument parsing, determine whether (args.device, args.feature_id)
       provides a lock override request.
       * address and feature_id describe the current lock owner.
       * args may describe an override request.

       Returns a tuple indicating t[0]: whether the lock is being overridden,
       t[1]: the token matching the corresponding new lock owner if t[0] is True,
       otherwise the current lock owner, and t[2] the overriding feature id if
       t[0] is True, otherwise the current lock owner's feature id."""

    # Find the current owner by enumeration.
    owner_filt = addr.device_filter(str(address), str(address))
    owner_filt['feature_id'] = feature_id
    owner_toks = feature.enumerate(args.width, args.region, **owner_filt)
    if not owner_toks:
        print('Previous locker not found. Releasing lock.')
        release_lock()
        sys.exit(1)

    if args.device is None and args.feature_id is None:
        return (False, owner_toks[0], feature_id) # no override

    if args.device is None:
        over_filt = owner_filt
        over_filt['feature_id'] = args.feature_id
    elif args.feature_id is None:
        over_filt = args.device
        over_filt['feature_id'] = feature_id
    else:
        over_filt = args.device
        over_filt['feature_id'] = args.feature_id

    owner_props = fpga.properties(owner_toks[0])

    # Find the overriding device by enumeration.
    over_toks = feature.enumerate(args.width, args.region, **over_filt)
    if not over_toks:
        if args.feature_id is None:
            return (False, owner_toks[0], feature_id)
        print(f'Error enumerating {addr.undo_device_filter(args.device)}. Skipping')
        return (False, owner_toks[0], feature_id)

    if len(over_toks) > 1:
        print(f'more than 1 device matches. Please narrow your search criteria.')
        sys.exit(1)

    over_props = fpga.properties(over_toks[0])

    if owner_props.object_id != over_props.object_id:
        if args.feature_id is None:
            return (True, over_toks[0], feature_id)
        return (True, over_toks[0], args.feature_id)

    # Same object_id
    if args.feature_id is None:
        return (False, owner_toks[0], feature_id)

    return (feature_id != args.feature_id, owner_toks[0], args.feature_id)


def set_lock_owner(address: addr.pcie_address, feature_id: addr.hex_int):
    """Set the lock owner to (address, feature_id)."""
    locker = get_lock_owner()
    if locker[0]:
        if address == locker[0] and feature_id == locker[1]:
            return # nothing to do

    p = Path(LOCK_FILE)
    parent = p.parent
    if not parent.exists():
        parent.mkdir(parents=True)

    contents = {
                 'locked_by': {
                   'address': f'{address}',
                   'feature_id': int(feature_id)
                 }
               }

    with open(p, 'w', encoding='utf-8') as fp:
        json.dump(contents, fp)


def release_lock():
    """Release the device lock by removing any existing lock file."""
    p = Path(LOCK_FILE)
    if not p.exists() or not p.is_file():
        return False
    p.unlink()
    return True


def description_str(tok_or_hndl, feature_id: int):
    """Queries the properties of the given token or handle object
       to construct a string consisting of the PCIe address along
       with a feature ID."""
    p = fpga.properties(tok_or_hndl)
    s = f'[{p.segment:04x}:{p.bus:02x}:{p.device:02x}.{p.function}]'
    if feature_id is not None:
        s += f' 0x{int(feature_id):02x}'
    return s


def args_description_str(args):
    """Converts a device filter back to its PCIe address or
       PCIe ID string."""
    return addr.undo_device_filter(args.device)


class lock_cmd():
    """Locks the script to a specified (device, feature id) pair so
       that subsequent runs don't need to specify the -d and -f options."""
    needs_device = False

    @staticmethod
    def add_subparser(subparser):
        """Add argparser subparser options."""
        lock = subparser.add_parser('lock')
        lock.add_argument('-d', '--device', dest='sdevice',
                          metavar='DEVICE', type=enum_filter,
                          help=DEVICE_HELP)
        lock.add_argument('-f', '--feature-id', dest='sfeature_id',
                          metavar='FEATURE_ID',
                          type=addr.hex_int, default=None,
                          help='DFL feature ID')

    def __call__(self, args, lowner, lfeature_id):
        descr = args_description_str(args)
        descr = ' ' + descr + ' ' if descr else ' '
        msg = f'lock{descr}'
        if lowner:
            if not args.device or args.feature_id is None:
                print(msg + f'currently held by {lowner} 0x{lfeature_id:02x}.')
                return 0
        else:
            if not args.device or args.feature_id is None:
                print(msg + 'Give me the device address and feature id.')
                return 1

        args.device['feature_id'] = args.feature_id
        tokens = feature.enumerate(args.width, args.region, **args.device)
        if not tokens:
            print(msg + 'no device/feature id found.')
            return 1
        if len(tokens) > 1:
            print(msg + 'more than 1 device matches. Please narrow your search with feature id.')
            return 1

        msg = f'lock {description_str(tokens[0], args.feature_id)}'
        address = addr.properties_to_address(fpga.properties(tokens[0]))
        set_lock_owner(address, int(args.feature_id))
        print(msg + ' OK')
        return 0


class unlock_cmd():
    """Command for script unlock from token. Allows reversing a prior
       lock_cmd action."""
    needs_device = False

    @staticmethod
    def add_subparser(subparser):
        """Add argparser subparser options."""
        unlock = subparser.add_parser('unlock')
        unlock.add_argument('-d', '--device', dest='sdevice',
                            metavar='DEVICE', type=enum_filter,
                            help=DEVICE_HELP)
        unlock.add_argument('-f', '--feature-id', dest='sfeature_id',
                            metavar='FEATURE_ID',
                            type=addr.hex_int, default=None,
                            help='DFL feature ID')

    def __call__(self, args, lowner, lfeature_id):
        descr = args_description_str(args)
        descr = ' ' + descr + ' ' if descr else ' '
        msg = f'unlock{descr}'
        if lowner:
            if not args.device:
                print(msg + f'Please tell me the device / feature id to unlock. (-d {lowner} -f 0x{lfeature_id:02x})')
                return 1
        else:
            print(msg + 'Not currently locked to a device.')
            return 1

        tokens = feature.enumerate(args.width, args.region, **args.device)
        if not tokens:
            print(msg + 'no device found.')
            sys.exit(1)
        elif len(tokens) > 1:
            print(msg + 'more than one device matches. Please narrow your search with feature id.')
            sys.exit(1)
        else:
            msg = f'unlock {description_str(tokens[0], args.feature_id)}'
            address = addr.properties_to_address(fpga.properties(tokens[0]))
            owner_addr, _ = get_lock_owner()
            if address == owner_addr:
                if release_lock():
                    msg += ' OK'
                else:
                    msg += ' Lock file not found!'
                print(msg)
            else:
                print(msg + f' {address} is not the current lock owner.')
                sys.exit(1)
        return 0


class ls_cmd():
    """Command for listing devices. Devices may be filtered by address,
       feature ID, token type, and OPAE interface."""
    needs_device = False

    ifc_to_str = {
        fpga.IFC_DFL: "DFL",
        fpga.IFC_VFIO: "VFIO",
        fpga.IFC_SIM_DFL: "DFL (ASE)",
        fpga.IFC_SIM_VFIO: "VFIO (ASE)",
        fpga.IFC_UIO: "UIO",
    }

    str_to_ifc = {
        "dfl": fpga.IFC_DFL,
        "vfio": fpga.IFC_VFIO,
        "dfl_ase": fpga.IFC_SIM_DFL,
        "vfio_ase": fpga.IFC_SIM_VFIO,
        "uio": fpga.IFC_UIO,
    }

    @staticmethod
    def add_subparser(subparser):
        """Add argparse subparser options."""
        ls = subparser.add_parser('ls')
        ls.add_argument('-d', '--device', dest='sdevice',
                        metavar='DEVICE', type=enum_filter,
                        help=DEVICE_HELP)
        ls.add_argument('-f', '--feature-id', dest='sfeature_id',
                        metavar='FEATURE_ID',
                        type=addr.hex_int, default=None,
                        help='DFL feature ID')
        ls.add_argument('-t', '--token-type',
                        choices=['device', 'accel'], default='accel',
                        help='Token type filter')
        ls.add_argument('-i', '--interface',
                        choices=list(ls_cmd.str_to_ifc.keys()), default=None,
                        help='OPAE interface filter')

    def __call__(self, args, lowner, lfeature_id):
        filt = args.device if args.device else {}
        if args.feature_id is not None:
            filt['feature_id'] = args.feature_id
        if args.token_type is not None:
            filt['type'] = (fpga.DEVICE if args.token_type == 'device'
                            else fpga.ACCELERATOR)
        if args.interface is not None:
            filt['interface'] = self.str_to_ifc[args.interface]

        tokens = feature.enumerate(args.width, args.region, **filt)

        # Capture the properties for each token, arranging
        # each by its PCIe address and PCIe ID.
        devs = {}
        for t in tokens:
            p = fpga.properties(t)
            key = f'[{addr.properties_to_address(p)}] ({addr.properties_to_id(p)})'
            if key in devs:
                devs[key].append((t, p))
            else:
                devs[key] = [(t, p)]

        access = addr.memory_access(args.width)

        for k, v in devs.items():
            print(k)
            for t, p in v:
                try:
                    with fpga.open(t, fpga.OPEN_SHARED) as hndl:
                        access.hndl = hndl
                        csr = dfh.dfh0(access.read(0, dfh.dfh0.width, args.region))
                        guid = access.read_guid(8, args.region)
                        if args.verbose:
                            msg = f'  interface={self.ifc_to_str[p.interface]} feature=0x{csr.bits.id:02x} guid={guid}'
                        else:
                            msg = f'  {self.ifc_to_str[p.interface]} 0x{csr.bits.id:02x} {guid}'
                        print(msg)
                except RuntimeError:
                    # FME tokens don't have mappable MMIO.
                    pass

        return 0


class peek_cmd():
    """Command for memory peek. Allows multiple read via command
       line arguments. The output may be formatted as simple, hex,
       or JSON and may be redirected to a file."""
    needs_device = True

    @staticmethod
    def add_subparser(subparser):
        """Add argparse subparser options."""
        peek = subparser.add_parser('peek')
        peek.add_argument('offset', type=addr.hex_int,
                          help='the MMIO offset (CSR) to peek')
        peek.add_argument('-d', '--device', dest='sdevice',
                          metavar='DEVICE', type=enum_filter,
                          help=DEVICE_HELP)
        peek.add_argument('-f', '--feature-id', dest='sfeature_id',
                          metavar='FEATURE_ID',
                          type=addr.hex_int, default=None,
                          help='DFL feature ID')
        peek.add_argument('-c', '--count', type=addr.hex_int,
                          default=addr.hex_int('1'),
                          help='the number of CSRs to peek')
        peek.add_argument('-o', '--output', type=FileType('w'),
                          default=sys.stdout,
                          help='file to store peek output')
        peek.add_argument('-F', '--format',
                          choices=['simple', 'hex', 'json'],
                          default='hex',
                          help='output format')

    def __call__(self, hndl, args, feature_id):
        descr = description_str(hndl, feature_id)
        descr = ' ' + descr + ' ' if descr else ' '
        msg = f'peek{descr}'

        if int(args.count) <= 0:
            print(msg + f'invalid count: {args.count}')
            return 1

        if args.verbose:
            print(msg + f'offset={args.offset} '
                  f'region={args.region} format={args.format}')

        access = addr.memory_access(args.width, hndl)

        if args.format == 'simple':
            offset = args.offset
            for _ in range(int(args.count)):
                data = access.read(offset, 64, args.region)
                print(f'{offset:08x}: {data:016x}', file=args.output)
                offset += 8
        elif args.format == 'hex':
            hview = hexview.hex_view(access, access.access_width)
            hview.render(int(args.offset),
                         int(args.count) * (access.access_width / 8),
                         args.region, args.output)
        elif args.format == 'json':
            mem = {}
            offset = args.offset
            for _ in range(int(args.count)):
                data = access.read(offset, 64, args.region)
                mem[f'0x{offset:08x}'] = f'0x{data:016x}'
                offset += 8
            json.dump(mem, args.output, indent=2)

        return 0


class poke_cmd():
    """Command for memory poke. Allows single write via command
       line arguments or multiple write via JSON file."""
    needs_device = True

    @staticmethod
    def add_subparser(subparser):
        """Add argparse subparser options."""
        poke = subparser.add_parser('poke')
        poke.add_argument('offset', type=addr.hex_int, nargs='?',
                          help='the MMIO offset (CSR) to poke')
        poke.add_argument('value', type=addr.hex_int, nargs='?',
                          help='the value to poke')
        poke.add_argument('-d', '--device', dest='sdevice',
                          metavar='DEVICE', type=enum_filter,
                          help=DEVICE_HELP)
        poke.add_argument('-f', '--feature-id', dest='sfeature_id',
                          metavar='FEATURE_ID',
                          type=addr.hex_int, default=None,
                          help='DFL feature ID')
        poke.add_argument('-j', '--json', type=FileType('r'),
                          default=None,
                          help='name of JSON file containing data to poke')

    def __call__(self, hndl, args, feature_id):
        descr = description_str(hndl, feature_id)
        descr = ' ' + descr + ' ' if descr else ' '
        msg = f'poke{descr}'

        access = addr.memory_access(args.width, hndl)

        if args.json is None:
            if args.offset is None or args.value is None:
                print(msg + 'offset and value are required parameters.')
                return 1
            if args.verbose:
                print(msg + f'offset={args.offset} value={args.value} region={args.region}')
            access.write(args.offset, args.value, 64, args.region)
        else:
            if args.verbose:
                print(msg + f'region={args.region}')
            mem = json.load(args.json)
            for k, v in mem.items():
                access.write(int(k, 0), int(v, 0), 64, args.region)

        return 0


class mb_read_cmd():
    """Command for mailbox read. Allows multiple read via command
       line arguments. The output may be formatted as simple, hex,
       or JSON and may be redirected to a file."""
    needs_device = True

    @staticmethod
    def add_subparser(subparser):
        """Add argparse subparser options."""
        mb_read = subparser.add_parser('mb-read')
        mb_read.add_argument('address', type=addr.hex_int,
                             help='the mailbox address to read')
        mb_read.add_argument('-d', '--device', dest='sdevice',
                             metavar='DEVICE', type=enum_filter,
                             help=DEVICE_HELP)
        mb_read.add_argument('-f', '--feature-id', dest='sfeature_id',
                             metavar='FEATURE_ID',
                             type=addr.hex_int, default=None,
                             help='DFL feature ID')
        mb_read.add_argument('-b', '--mailbox-base', type=addr.hex_int,
                             default=None, help='the CSR offset of the mailbox')
        mb_read.add_argument('-c', '--count', type=addr.hex_int,
                             default=addr.hex_int('1'),
                             help='the number of addresses to read')
        mb_read.add_argument('-t', '--timeout', type=int, default=100,
                             help='total number of microseconds to wait when polling')
        mb_read.add_argument('-s', '--sleep', type=int, default=1,
                             help='number of microseconds to sleep between each poll')
        mb_read.add_argument('-o', '--output', type=FileType('w'),
                             default=sys.stdout,
                             help='file to store mb-read output')
        mb_read.add_argument('-F', '--format',
                             choices=['simple', 'hex', 'json'],
                             default='hex',
                             help='output format')

    def __call__(self, hndl, args, feature_id):
        descr = description_str(hndl, feature_id)
        descr = ' ' + descr + ' ' if descr else ' '
        msg = f'mb-read{descr}'

        if args.mailbox_base is None:
            print(msg + 'This command needs -b mailbox_base.', end='')
            base = mailbox.feature_id_to_mailbox_base(int(feature_id))
            if base is not None:
                print(f' For feature_id 0x{feature_id:02x}, try -b 0x{base:04x}')
            else:
                print()
            return 1

        if int(args.count) <= 0:
            print(msg + f'invalid count: {args.count}')
            return 1

        if args.verbose:
            print(msg + f'address={args.address} base={args.mailbox_base} '
                        f'timeout={args.timeout} sleep={args.sleep}')

        mb_access = mailbox.mailbox_access(args.width,
            int(args.mailbox_base), hndl,
            poll_to=args.timeout / 1000000,
            poll_sleep=args.sleep / 1000000)

        if args.format == 'simple':
            address = int(args.address)
            for _ in range(int(args.count)):
                data = mb_access.read(address, region=args.region)
                print(f'{address:08x}: {data:08x}', file=args.output)
                address += 4
        elif args.format == 'hex':
            hview = hexview.hex_view(mb_access, 32)
            hview.render(int(args.address),
                         int(args.count) * 4,
                         args.region, args.output)
        elif args.format == 'json':
            mem = {}
            mb_base = args.mailbox_base
            address = int(args.address)
            for _ in range(int(args.count)):
                data = mb_access.read(address, region=args.region)
                mem[f'0x{address:08x}'] = f'0x{data:08x}'
                address += 4
            mb = { f'0x{mb_base:08x}': mem }
            json.dump(mb, args.output, indent=2)

        return 0


class mb_write_cmd():
    """Command for mailbox write. Allows single write via command
       line arguments or multiple write via JSON file."""
    needs_device = True

    @staticmethod
    def add_subparser(subparser):
        """Add argparse subparser options."""
        mb_write = subparser.add_parser('mb-write')
        mb_write.add_argument('address', type=addr.hex_int, nargs='?',
                              help='the mailbox address to write')
        mb_write.add_argument('value', type=addr.hex_int, nargs='?',
                              help='the value to write')
        mb_write.add_argument('-d', '--device', dest='sdevice',
                              metavar='DEVICE', type=enum_filter,
                              help=DEVICE_HELP)
        mb_write.add_argument('-f', '--feature-id', dest='sfeature_id',
                              metavar='FEATURE_ID',
                              type=addr.hex_int, default=None,
                              help='DFL feature ID')
        mb_write.add_argument('-b', '--mailbox-base', type=addr.hex_int,
                              default=None, help='the CSR offset of the mailbox')
        mb_write.add_argument('-t', '--timeout', type=int, default=100,
                              help='total number of microseconds to wait when polling')
        mb_write.add_argument('-s', '--sleep', type=int, default=1,
                              help='number of microseconds to sleep between each poll')
        mb_write.add_argument('-j', '--json', type=FileType('r'),
                              default=None,
                              help='name of JSON file containing data to write')

    def __call__(self, hndl, args, feature_id):
        descr = description_str(hndl, feature_id)
        descr = ' ' + descr + ' ' if descr else ' '
        msg = f'mb-write{descr}'

        if args.mailbox_base is None and args.json is None:
            print(msg + 'This command requires -b mailbox_base.', end='')
            base = mailbox.feature_id_to_mailbox_base(int(feature_id))
            if base is not None:
                print(f' For feature_id 0x{feature_id:02x}, try -b 0x{base:04x}')
            else:
                print()
            return 1

        if args.json is None:
            if args.address is None or args.value is None:
                print(msg + 'address and value are required parameters.')
                return 1
            if args.verbose:
                print(msg + f'address={args.address} value={args.value} base={args.mailbox_base} '
                            f'timeout={args.timeout} sleep={args.sleep}')
            mb_access = mailbox.mailbox_access(args.width,
                int(args.mailbox_base), hndl,
                poll_to=args.timeout / 1000000,
                poll_sleep=args.sleep / 1000000)
            mb_access.write(int(args.address), int(args.value), region=args.region)
        else:
            if args.verbose:
                print(msg + f'address={args.address} value={args.value} base={args.mailbox_base} '
                            f'timeout={args.timeout} sleep={args.sleep}')
            mb = json.load(args.json)
            for mb_base in mb.keys():
                mem = mb[mb_base]
                mb_access = mailbox.mailbox_access(args.width,
                    int(mb_base, 0), hndl,
                    poll_to=args.timeout / 1000000,
                    poll_sleep=args.sleep / 1000000)
                for a in mem.keys():
                    address = int(a, 0)
                    data = int(mem[a], 0)
                    mb_access.write(address, data, region=args.region)

        return 0


def find_device(args):
    """Use args.device and args.feature_id to obtain the
       corresponding token."""
    if args.device is None:
        args.device = {}
    if args.feature_id is not None:
        args.device['feature_id'] = args.feature_id

    devices = feature.enumerate(args.width, args.region, **args.device)
    if not devices:
        undo = addr.undo_device_filter(args.device)
        print(f'No device found for filter criteria: {undo}')
        sys.exit(1)

    if len(devices) > 1:
        print('Found more than one device for filter criteria.')
        print('Please try narrowing the search by providing a PCIe address.')
        sys.exit(1)

    return devices[0]


def main():
    """Application entry point."""
    actions = {
               'lock': lock_cmd,
               'unlock': unlock_cmd,
               'ls': ls_cmd,
               'peek': peek_cmd,
               'poke': poke_cmd,
               'mb-read': mb_read_cmd,
               'mb-write': mb_write_cmd,
              }

    parser = ArgumentParser()

    parser.add_argument('-d', '--device', type=enum_filter,
                        help=DEVICE_HELP)
    parser.add_argument('-f', '--feature-id',
                        type=addr.hex_int, default=None,
                        help='DFL feature ID')
    parser.add_argument('-w', '--width', type=int, choices=[64, 32],
                        default=64, help='CSR access width')
    parser.add_argument('-r', '--region', type=int,
                        default=0, help='CSR MMIO region')
    parser.add_argument('-V', '--verbose', action='store_true',
                        default=False, help='be verbose with output')
    parser.add_argument('-v', '--version', action='version',
                        version=f"%(prog)s {VERSION}",
                        help='display version information and exit')
    subparser = parser.add_subparsers(dest='which')

    for _, v in actions.items():
        v.add_subparser(subparser)

    args = parser.parse_args()

    # Favor the sub-parser's sdevice and sfeature_id when present.
    if hasattr(args, 'sdevice') and args.sdevice is not None:
        args.device = args.sdevice
    if hasattr(args, 'sfeature_id') and args.sfeature_id is not None:
        args.feature_id = args.sfeature_id

    action = None
    if hasattr(args, 'which') and args.which:
        action = actions[args.which]()

    if not action:
        print(f'Please choose an action from {", ".join(actions.keys())}.\n')
        parser.print_help()
        sys.exit(1)

    # Determine whether a previous session locked a device.
    # If so, retrieve the PCIe address of the locked device.
    lowner, lfeature_id = get_lock_owner()

    tok = None
    feature_id = lfeature_id
    pr = isinstance(action, (lock_cmd, unlock_cmd)) or args.verbose
    if lowner:
        if pr:
            print(f'[locked {lowner} 0x{lfeature_id:0x}', end='')

        is_override, tok, feature_id = lock_override_requested(
            lowner, lfeature_id, args)

        if is_override and pr:
            print(f' [override {addr.tok_or_handle_to_address(tok)} 0x{feature_id:0x}]', end='')

        if pr:
            print('] ', end='\n' if isinstance(action, ls_cmd) else '')

    if feature_id is None:
        feature_id = args.feature_id

    res = 0
    if action.needs_device:
        if not tok:
            tok = find_device(args)
        with fpga.open(tok, 0) as hndl:
            res = action(hndl, args, feature_id)
    else:
        res = action(args, lowner, lfeature_id)

    sys.exit(res)

if __name__ == '__main__':
    main()
