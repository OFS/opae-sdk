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

DEFAULT_RSU_CONFIG = {
  (0x8086, 0x0b2b, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY): None, # D5005
  (0x8086, 0xbcce,          0x8086,          0x138d): None, # D5005
  (0x8086, 0x0b30, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY): None, # N3000
  (0x1c2c, 0x1000,               0,               0): {     # N5010
    'fpga_default_sequences': [
      "fpga_user1",
      "fpga_user2",
      "fpga_user1 fpga_user2",
      "fpga_user2 fpga_user1",
      "fpga_factory",
      "fpga_factory fpga_user1",
      "fpga_factory fpga_user2",
      "fpga_factory fpga_user1 fpga_user2",
      "fpga_factory fpga_user2 fpga_user1",
      "fpga_user1 fpga_user2 fpga_factory",
      "fpga_user2 fpga_user1 fpga_factory"
    ]
  },
  (0x8086, 0xbcce, 0x8086, 0x1770): {                       # N6000
    'fpga_default_sequences': [
      "fpga_user1",
      "fpga_user2",
      "fpga_user1 fpga_user2",
      "fpga_user2 fpga_user1",
      "fpga_factory",
      "fpga_factory fpga_user1",
      "fpga_factory fpga_user2",
      "fpga_factory fpga_user1 fpga_user2",
      "fpga_factory fpga_user2 fpga_user1",
      "fpga_user1 fpga_user2 fpga_factory",
      "fpga_user2 fpga_user1 fpga_factory"
    ]
  },
  (0x8086, 0xbcce, 0x8086, 0x1771): {                       # N6001
    'fpga_default_sequences': [
      "fpga_user1",
      "fpga_user2",
      "fpga_user1 fpga_user2",
      "fpga_user2 fpga_user1",
      "fpga_factory",
      "fpga_factory fpga_user1",
      "fpga_factory fpga_user2",
      "fpga_factory fpga_user1 fpga_user2",
      "fpga_factory fpga_user2 fpga_user1",
      "fpga_user1 fpga_user2 fpga_factory",
      "fpga_user2 fpga_user1 fpga_factory"
    ]
  },
  (0x8086, 0xbcce, 0x8086, 0x17d4): {                       # C6100
    'fpga_default_sequences': [
      "fpga_user1",
      "fpga_user2",
      "fpga_user1 fpga_user2",
      "fpga_user2 fpga_user1",
      "fpga_factory",
      "fpga_factory fpga_user1",
      "fpga_factory fpga_user2",
      "fpga_factory fpga_user1 fpga_user2",
      "fpga_factory fpga_user2 fpga_user1",
      "fpga_user1 fpga_user2 fpga_factory",
      "fpga_user2 fpga_user1 fpga_factory"
    ]
  },
  (0x8086, 0xaf00, 0x8086,      0): {                       # OFS
    'fpga_default_sequences': [
      "fpga_user1",
      "fpga_user2",
      "fpga_user1 fpga_user2",
      "fpga_user2 fpga_user1",
      "fpga_factory",
      "fpga_factory fpga_user1",
      "fpga_factory fpga_user2",
      "fpga_factory fpga_user1 fpga_user2",
      "fpga_factory fpga_user2 fpga_user1",
      "fpga_user1 fpga_user2 fpga_factory",
      "fpga_user2 fpga_user1 fpga_factory"
    ]
  },
  (0x8086, 0xbcce, 0x8086,      0): {                       # OFS
    'fpga_default_sequences': [
      "fpga_user1",
      "fpga_user2",
      "fpga_user1 fpga_user2",
      "fpga_user2 fpga_user1",
      "fpga_factory",
      "fpga_factory fpga_user1",
      "fpga_factory fpga_user2",
      "fpga_factory fpga_user1 fpga_user2",
      "fpga_factory fpga_user2 fpga_user1",
      "fpga_user1 fpga_user2 fpga_factory",
      "fpga_user2 fpga_user1 fpga_factory"
    ]
  }
}

RSU_CONFIG = DEFAULT_RSU_CONFIG

DEFAULT_FPGAREG_CONFIG = {
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
    'platform': 'Intel Acceleration Development Platform C6100'
  },
  (0x8086, 0xbccf, 0x8086, 0x17d4): {
    'platform': 'Intel Acceleration Development Platform C6100'
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

FPGAREG_CONFIG = DEFAULT_FPGAREG_CONFIG


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


def load_rsu_config(rsu_cfg, root, config, configurations):
    """Parse section config of the one OPAE configuration file
       into the desired internal rsu data structure (see
       DEFAULT_RSU_CONFIG for the format).
    """
    if config not in configurations:
        print(f'Error parsing config: {config} not found.')
        return (False, rsu_cfg)

    c = configurations[config]
    key = 'enabled'
    if key not in c or not c[key]:
        return (True, rsu_cfg) # continue parsing

    key = 'devices'
    if key not in c:
        print(f'Error parsing config: no "devices" key in "{config}".')
        return (False, rsu_cfg)
    if type(c[key]) != list:
        print(f'Error parsing config: "devices" key not a list.')
        return (False, rsu_cfg)

    devs = parse_devices(c[key])
    if not devs:
        print(f'Error parsing config: "devices" is empty.')
        return (False, rsu_cfg)

    key = 'opae'
    if key not in c or type(c[key]) != dict:
        print(f'Error parsing config: "opae" key '
              f'missing or invalid in {config}.')
        return (False, rsu_cfg)

    opae = c[key]
    key = 'rsu'
    if key not in opae or type(opae[key]) != list:
        print(f'Warning parsing config: "rsu" key '
              f'missing or invalid in {config} "opae".')
        return (True, rsu_cfg) # continue parsing

    rsu = opae[key]
    for r in rsu:
        key = 'enabled'
        if key not in r or not r[key]:
            continue

        key = 'devices'
        if key not in r or len(r[key]) == 0:
            print(f'Warning parsing config: "devices" key '
                  f'missing or invalid in {config} "rsu".')
            continue
    
        sequences = {}
        key = 'fpga_default_sequences'
        if key in r and type(r[key]) is str:
            defs = r[key]
            sequences[key] = root[defs]
    
        if not sequences:
            sequences = None
    
        for d in r['devices']:
            if d not in devs:
                print(f'Error parsing config: {d} not found '
                      f'in "devices" for {config}.')
                return (False, rsu_cfg)
    
            rsu_cfg[devs[d]] = sequences

    return (True, rsu_cfg)


def load_rsu_configuration(cfg):
    """Walk through each "configs" section of the given parsed
       JSON object cfg. If cfg is None or if an error is encountered
       during parsing, then return None. Otherwise, return the parsed
       rsu object.
    """
    if not cfg: # No parsed config from file.
        return None

    if not 'configs' in cfg:
        print(f'Error parsing config: "configs" key not found.')
        return None
    if not 'configurations' in cfg:
        print(f'Error parsing config: "configurations" key not found.')
        return None

    rsu_cfg = {}
    for c in cfg['configs']:
        status, rsu_cfg = load_rsu_config(rsu_cfg,
                                          cfg,
                                          c,
                                          cfg['configurations'])
        if not status:
            return None

    return rsu_cfg


def print_rsu_configuration(cfg):
    """Pretty print an rsu configuration data object.
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


def load_fpgareg_config(fpgareg_cfg, root, config, configurations):
    """Parse section config of the one OPAE configuration file
       into the desired internal fpgareg data structure (see
       DEFAULT_FPGAREG_CONFIG for the format).
    """
    if config not in configurations:
        print(f'Error parsing config: {config} not found.')
        return (False, fpgareg_cfg)

    c = configurations[config]
    key = 'enabled'
    if key not in c or not c[key]:
        return (True, fpgareg_cfg) # continue parsing

    key = 'platform'
    if key not in c:
        print(f'Error parsing config: no "platform" key in "{config}".')
        return (False, fpgareg_cfg)
    if type(c[key]) != str:
        print(f'Error parsing config: "platform" key not a string.')
        return (False, fpgareg_cfg)
    platform = c[key]

    key = 'devices'
    if key not in c:
        print(f'Error parsing config: no "devices" key in "{config}".')
        return (False, fpgareg_cfg)
    if type(c[key]) != list:
        print(f'Error parsing config: "devices" key not a list.')
        return (False, fpgareg_cfg)

    devs = parse_devices(c[key])
    if not devs:
        print(f'Error parsing config: "devices" is empty.')
        return (False, fpgareg_cfg)

    key = 'opae'
    if key not in c or type(c[key]) != dict:
        print(f'Error parsing config: "opae" key '
              f'missing or invalid in {config}.')
        return (False, fpgareg_cfg)

    opae = c[key]
    key = 'fpgareg'
    if key not in opae or type(opae[key]) != list:
        print(f'Warning parsing config: "fpgareg" key '
              f'missing or invalid in {config} "opae".')
        return (True, fpgareg_cfg) # continue parsing

    value = { 'platform': platform }

    fpgareg = opae[key]
    for f in fpgareg:
        key = 'enabled'
        if key not in f or not f[key]:
            continue

        key = 'devices'
        if key not in f or len(f[key]) == 0:
            print(f'Warning parsing config: "devices" key '
                  f'missing or invalid in {config} "fpgareg".')
            continue

        for d in f['devices']:
            if d not in devs:
                print(f'Error parsing config: {d} not found '
                      f'in "devices" for {config}.')
                return (False, fpgareg_cfg)
    
            fpgareg_cfg[devs[d]] = value

    return (True, fpgareg_cfg)


def load_fpgareg_configuration(cfg):
    """Walk through each "configs" section of the given parsed
       JSON object cfg. If cfg is None or if an error is encountered
       during parsing, then return None. Otherwise, return the parsed
       fpgareg object.
    """
    if not cfg: # No parsed config from file.
        return None

    if not 'configs' in cfg:
        print(f'Error parsing config: "configs" key not found.')
        return None
    if not 'configurations' in cfg:
        print(f'Error parsing config: "configurations" key not found.')
        return None

    fpgareg_cfg = {}
    for c in cfg['configs']:
        status, fpgareg_cfg = load_fpgareg_config(fpgareg_cfg,
                                                  cfg,
                                                  c,
                                                  cfg['configurations'])
        if not status:
            return None

    return fpgareg_cfg


def print_fpgareg_configuration(cfg):
    """Pretty print an fpgareg configuration data object.
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
       constructing internal representations of the rsu and
       fpgareg configuration data. If the file cannot be
       found or if an error is encountered during parsing,
       then use a hard-coded default configuration.
    """
    cfg = None
    cfg_file = find_config_file()

    if not cfg_file: # Use defaults
        return

    with open(cfg_file, 'r') as fd:
        cfg = json.load(fd)

    global RSU_CONFIG
    r = load_rsu_configuration(cfg)
    if r:
        RSU_CONFIG = r

    global FPGAREG_CONFIG
    f = load_fpgareg_configuration(cfg)
    if f:
        FPGAREG_CONFIG = f


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
    """The high-level user interface to the parsed rsu and
       fpgareg configuration data.
    """
    def rsu_is_supported(self, vid, did, svid, sdid):
        global RSU_CONFIG
        for key in RSU_CONFIG:
            if key_matches_id(key, vid, did, svid, sdid):
                return True
        return False

    def rsu_fpga_defaults_for(self, vid, did, svid, sdid):
        global RSU_CONFIG
        ID = (vid, did, svid, sdid)
        for key in RSU_CONFIG:
            if key_matches_id(key, *ID):
                s = RSU_CONFIG[key]
                return s if s is None else s['fpga_default_sequences']

    def fpgareg_is_supported(self, vid, did, svid, sdid):
        global FPGAREG_CONFIG
        for key in FPGAREG_CONFIG:
            if key_matches_id(key, vid, did, svid, sdid):
                return True
        return False

    def fpgareg_platform_for(self, vid, did, svid, sdid):
        global FPGAREG_CONFIG
        ID = (vid, did, svid, sdid)
        for key in FPGAREG_CONFIG:
            if key_matches_id(key, *ID):
                return FPGAREG_CONFIG[key]['platform']


load_configs()
