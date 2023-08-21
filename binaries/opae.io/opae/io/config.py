# Copyright(c) 2022, Intel Corporation
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

import json
import os

try:
    from pathlib import Path
except ImportError:
    from pathlib2 import Path


OPAE_VENDOR_ANY = 0xffff
OPAE_DEVICE_ANY = 0xffff

DEFAULT_OPAE_IO_CONFIG = {
  (0x8086, 0xbcbd, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel Integrated Multi-Chip Acceleration Platform'
  },
  (0x8086, 0xbcc0, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel Integrated Multi-Chip Acceleration Platform'
  },
  (0x8086, 0xbcc1, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel Integrated Multi-Chip Acceleration Platform'
  },
  (0x8086, 0x09c4, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel Programmable Acceleration Card with Intel Arria 10 GX FPGA'
  },
  (0x8086, 0x09c5, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel Programmable Acceleration Card with Intel Arria 10 GX FPGA'
  },
  (0x8086, 0x0b2b, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel FPGA Programmable Acceleration Card D5005'
  },
  (0x8086, 0x0b2c, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel FPGA Programmable Acceleration Card D5005'
  },
  (0x8086, 0xbcce, 0x8086, 0x138d) : {
    'platform': 'Intel FPGA Programmable Acceleration Card D5005'
  },
  (0x8086, 0xbccf, 0x8086, 0x138d) : {
    'platform': 'Intel FPGA Programmable Acceleration Card D5005'
  },
  (0x8086, 0x0b30, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel FPGA Programmable Acceleration Card N3000'
  },
  (0x8086, 0x0b31, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel FPGA Programmable Acceleration Card N3000'
  },
  (0x8086, 0x0ddb, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY) : {
    'platform': 'Intel Acceleration Development Platform CMC'
  },
  (0x1c2c, 0x1000, 0, 0) : {
    'platform': 'Silicom FPGA SmartNIC N5010 Series'
  },
  (0x1c2c, 0x1001, 0, 0) : {
    'platform': 'Silicom FPGA SmartNIC N5010 Series'
  },
(0x1c2c, 0x1002, 0, 0) : {
    'platform': 'Silicom FPGA SmartNIC N5013'
  },
  (0x1c2c, 0x1003, 0, 0) : {
    'platform': 'Silicom FPGA SmartNIC N5014'
  },
  (0x1ded, 0x8103, 0x1ded, 0x4342) : {
    'platform': 'Alibaba Card F5'
  },
  (0x8086, 0xbcce, 0x8086, 0x1770) : {
    'platform': 'Intel Acceleration Development Platform N6000'
  },
  (0x8086, 0xbccf, 0x8086, 0x1770) : {
    'platform': 'Intel Acceleration Development Platform N6000'
  },
  (0x8086, 0xbcce, 0x8086, 0x1771) : {
    'platform': 'Intel Acceleration Development Platform N6001'
  },
  (0x8086, 0xbccf, 0x8086, 0x1771) : {
    'platform': 'Intel Acceleration Development Platform N6001'
  },
  (0x8086, 0xbcce, 0x8086, 0x17d4): {
    'platform': 'Intel IPU Platform F2000X-PL'
  },
  (0x8086, 0xbccf, 0x8086, 0x17d4): {
    'platform': 'Intel IPU Platform F2000X-PL'
  },
  (0x8086, 0xaf00, 0x8086, 0): {
    'platform': 'Intel Open FPGA Stack Platform'
  },
  (0x8086, 0xaf01, 0x8086, 0): {
    'platform': 'Intel Open FPGA Stack Platform'
  },
  (0x8086, 0xbcce, 0x8086, 0): {
    'platform': 'Intel Open FPGA Stack Platform'
  },
  (0x8086, 0xbccf, 0x8086, 0): {
    'platform': 'Intel Open FPGA Stack Platform'
  }
}

OPAE_IO_CONFIG = DEFAULT_OPAE_IO_CONFIG


def find_config_file():
    """ Find the one OPAE configuration file and return
        its path as a string in Posix format.
        First, check environment variable LIBOPAE_CFGFILE.
        If not found there, then check a series of paths
        relative to the user's home directory. Finally,
        check a few system paths. Returns None if not found.
    """
    cfg = os.environ.get('LIBOPAE_CFGFILE')
    if cfg:
        try:
            resolved = Path(cfg).resolve(strict=True)
            if resolved.exists() and resolved.is_file():
                return resolved.as_posix()
        except FileNotFoundError:
            pass

    home_paths = ['~/.local/opae.cfg',
                  '~/.local/opae/opae.cfg',
                  '~/.config/opae/opae.cfg']

    for h in home_paths:
        try:
            p = Path(h).expanduser()
        except RuntimeError:
            continue

        try:
            resolved = p.resolve(strict=True)
            if resolved.exists() and resolved.is_file():
                return resolved.as_posix()
        except FileNotFoundError:
            pass

    sys_paths = ['/usr/local/etc/opae/opae.cfg',
                 '/etc/opae/opae.cfg']

    for s in sys_paths:
        try:
            resolved = Path(s).resolve(strict=True)
            if resolved.exists() and resolved.is_file():
                return resolved.as_posix()
        except FileNotFoundError:
            pass


def parse_devices(devices):
    """Parse the "devices" portion of the one config file for a given
       config object. Returns a dictionary of the "name" keys mapped to
       the integer VID, DID, SVID, SDID contained in a tuple.
    """
    devs = {}
    for d in devices:
        if 'name' not in d:
            print(f'Error parsing config: "name" key not in "devices" entry.')
            return None
        if type(d['name']) != str:
            print(f'Error parsing config: "name" key not a string.')
            return None

        name = d['name']

        if 'id' not in d:
            print(f'Error parsing config: "id" key not in "devices" entry.')
            return None
        if type(d['id']) != list or len(d['id']) != 4:
            print(f'Error parsing config: "id" key not a list or too few items.')
            return None

        ID = d['id']

        vid = int(ID[0], 0)
        did = int(ID[1], 0)

        if type(ID[2]) == str and ID[2] == '*':
            svid = OPAE_VENDOR_ANY
        else:
            svid = int(ID[2], 0)

        if type(ID[3]) == str and ID[3] == '*':
            sdid = OPAE_DEVICE_ANY
        else:
            sdid = int(ID[3], 0)

        devs[name] = (vid, did, svid, sdid)

    return devs


def load_opae_io_config(opae_io_cfg, root, config, configurations):
    """Parse section config of the one OPAE configuration file
       into the desired internal opae.io data structure (see
       DEFAULT_OPAE_IO_CONFIG for the format).
    """
    if config not in configurations:
        print(f'Error parsing config: {config} not found.')
        return False

    c = configurations[config]
    key = 'enabled'
    if key not in c or not c[key]:
        return True # continue parsing

    key = 'platform'
    if key not in c:
        print(f'Error parsing config: no "platform" key in "{config}".')
        return False
    if type(c[key]) != str:
        print(f'Error parsing config: "platform" key not a string.')
        return False
    platform = c[key]

    key = 'devices'
    if key not in c:
        print(f'Error parsing config: no "devices" key in "{config}".')
        return False
    if type(c[key]) != list:
        print(f'Error parsing config: "devices" key not a list.')
        return False

    devs = parse_devices(c[key])
    if not devs:
        print(f'Error parsing config: "devices" is empty.')
        return False

    key = 'opae'
    if key not in c or type(c[key]) != dict:
        print(f'Error parsing config: "opae" key '
              f'missing or invalid in {config}.')
        return False

    opae = c[key]
    key = 'opae.io'
    if key not in opae or type(opae[key]) != list:
        print(f'Warning parsing config: "opae.io" key '
              f'missing or invalid in {config} "opae".')
        return True # continue parsing

    value = { 'platform': platform }

    opae_io = opae[key]
    for o in opae_io:
        key = 'enabled'
        if key not in o or not o[key]:
            continue

        key = 'devices'
        if key not in o or len(o[key]) == 0:
            print(f'Warning parsing config: "devices" key '
                  f'missing or invalid in {config} "opae.io".')
            continue
    
        for d in o['devices']:
            if d not in devs:
                print(f'Error parsing config: {d} not found '
                      f'in "devices" for {config}.')
                return False
    
            opae_io_cfg[devs[d]] = value

    return True


def load_opae_io_configuration(cfg):
    """Walk through each "configs" section of the given parsed
       JSON object cfg. If cfg is None or if an error is encountered
       during parsing, then return None. Otherwise, return the parsed
       opae.io object.
    """
    if not cfg: # No parsed config from file.
        return None

    if not 'configs' in cfg:
        print(f'Error parsing config: "configs" key not found.')
        return None
    if not 'configurations' in cfg:
        print(f'Error parsing config: "configurations" key not found.')
        return None

    opae_io_cfg = {}
    for c in cfg['configs']:
        status = load_opae_io_config(opae_io_cfg,
                                     cfg,
                                     c,
                                     cfg['configurations'])
        if not status:
            return None

    return opae_io_cfg


def print_opae_io_configuration(cfg):
    """Pretty print an opae.io configuration data object.
    """
    for key in cfg:
        print(f'0x{key[0]:04x}:0x{key[1]:04x} ', end='')
        if key[2] == OPAE_VENDOR_ANY:
            print(f'*     ', end='')
        else:
            print(f'0x{key[2]:04x}', end='')
        print(':', end='')
        if key[3] == OPAE_DEVICE_ANY:
            print(f'*     ', end='')
        else:
            print(f'0x{key[3]:04x}', end='')

        print(' ', end='')
        print(cfg[key])


def load_configs():
    """Find the one OPAE config file and parse it,
       constructing internal representations of the opae.io
       configuration data. If the file cannot be found
       or if an error is encountered during parsing,
       then use a hard-coded default configuration.
    """
    cfg = None
    cfg_file = find_config_file()

    if not cfg_file: # Use defaults
        return

    with open(cfg_file, 'r') as fd:
        cfg = json.load(fd)

    global OPAE_IO_CONFIG
    o = load_opae_io_configuration(cfg)
    if o:
        OPAE_IO_CONFIG = o


def key_matches_id(key, vid, did, svid, sdid):
    """Determine whether the given key (a 4-tuple of
       integer PCIe ID components) is a match for the
       given parameters.
    """
    if key[0] != vid:
        return False
    if key[1] != did:
        return False
    if key[2] != OPAE_VENDOR_ANY and key[2] != svid:
        return False
    if key[3] != OPAE_DEVICE_ANY and key[3] != sdid:
        return False
    return True


class Config:
    """The high-level user interface to the parsed opae.io
       configuration data.
    """
    @staticmethod
    def opae_io_is_supported(vid, did, svid, sdid):
        global OPAE_IO_CONFIG
        for key in OPAE_IO_CONFIG:
            if key_matches_id(key, vid, did, svid, sdid):
                return True
        return False

    @staticmethod
    def opae_io_platform_for(vid, did, svid, sdid):
        global OPAE_IO_CONFIG
        ID = (vid, did, svid, sdid)
        for key in OPAE_IO_CONFIG:
            if key_matches_id(key, *ID):
                return OPAE_IO_CONFIG[key]['platform']


load_configs()
