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

import os
import unittest

try:
    from pathlib import Path
except ImportError:
    from pathlib2 import Path

from opae.io.config import find_config_file, load_configs, Config

TEST_CONFIG = '''{
  "configurations": {

    "mcp" : {
      "enabled": true,
      "platform": "Intel Integrated Multi-Chip Acceleration Platform",

      "devices": [
        { "name": "mcp0_pf", "id": [ "0x8086", "0xbcbd", "*", "*" ] },
        { "name": "mcp1_pf", "id": [ "0x8086", "0xbcc0", "*", "*" ] },
        { "name": "mcp1_vf", "id": [ "0x8086", "0xbcc1", "*", "*" ] }
      ],

      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "mcp0_pf", "mcp1_pf", "mcp1_vf" ]
          }
        ]
      }
    },

    "a10gx": {
      "enabled": true,
      "platform": "Intel Programmable Acceleration Card with Intel Arria 10 GX FPGA",

      "devices": [
        { "name": "a10gx_pf", "id": [ "0x8086", "0x09c4", "*", "*" ] },
        { "name": "a10gx_vf", "id": [ "0x8086", "0x09c5", "*", "*" ] }
      ],

      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "a10gx_pf", "a10gx_vf" ]
          }
        ]
      }
    },

    "d5005": {
      "enabled": true,
      "platform": "Intel FPGA Programmable Acceleration Card D5005",

      "devices": [
        { "name": "d5005_0_pf", "id": [ "0x8086", "0x0b2b", "*",      "*"      ] },
        { "name": "d5005_0_vf", "id": [ "0x8086", "0x0b2c", "*",      "*"      ] },
        { "name": "d5005_1_pf", "id": [ "0x8086", "0xbcce", "0x8086", "0x138d" ] },
        { "name": "d5005_1_vf", "id": [ "0x8086", "0xbccf", "0x8086", "0x138d" ] }
      ],

      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "d5005_0_pf", "d5005_0_vf", "d5005_1_pf", "d5005_1_vf" ]
          }
        ]
      }
    },

    "n3000": {
      "enabled": true,
      "platform": "Intel FPGA Programmable Acceleration Card N3000",

      "devices": [
        { "name": "n3000_pf", "id": [ "0x8086", "0x0b30", "*", "*" ] },
        { "name": "n3000_vf", "id": [ "0x8086", "0x0b31", "*", "*" ] }
      ],

      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "n3000_pf", "n3000_vf" ]
          }
        ]
      }
    },

    "n5010": {
      "enabled": true,
      "platform": "Silicom FPGA SmartNIC N5010 Series",

      "devices": [
        { "name": "n5010_pf", "id": [ "0x1c2c", "0x1000", "0", "0" ] },
        { "name": "n5010_vf", "id": [ "0x1c2c", "0x1001", "0", "0" ] }
      ],

      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "n5010_pf", "n5010_vf" ]
          }
        ]
      }
    },

    "n6000": {
      "enabled": true,
      "platform": "Intel Acceleration Development Platform N6000",

      "devices": [
        { "name": "n6000_pf", "id": [ "0x8086", "0xbcce", "0x8086", "0x1770" ] },
        { "name": "n6000_vf", "id": [ "0x8086", "0xbccf", "0x8086", "0x1770" ] }
      ],

      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "n6000_pf", "n6000_vf" ]
          }
        ]
      }
    },

    "n6001": {
      "enabled": true,
      "platform": "Intel Acceleration Development Platform N6001",

      "devices": [
        { "name": "n6001_pf", "id": [ "0x8086", "0xbcce", "0x8086", "0x1771" ] },
        { "name": "n6001_vf", "id": [ "0x8086", "0xbccf", "0x8086", "0x1771" ] }
      ],

      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "n6001_pf", "n6001_vf" ]
          }
        ]
      }
    },

    "c6100": {
      "enabled": true,
      "platform": "Intel IPU Platform F2000X-PL",

      "devices": [
        { "name": "c6100_pf", "id": [ "0x8086", "0xbcce", "0x8086", "0x17d4" ] },
        { "name": "c6100_vf", "id": [ "0x8086", "0xbccf", "0x8086", "0x17d4" ] }
      ],

      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "c6100_pf", "c6100_vf" ]
          }
        ]
      }
    },

    "ofs": {
      "enabled": true,
      "platform": "Intel Open FPGA Stack Platform",

      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xaf00", "0x8086", "0" ] },
        { "name": "ofs0_vf", "id": [ "0x8086", "0xaf01", "0x8086", "0" ] },
        { "name": "ofs1_pf", "id": [ "0x8086", "0xbcce", "0x8086", "0" ] },
        { "name": "ofs1_vf", "id": [ "0x8086", "0xbccf", "0x8086", "0" ] }
      ],

      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "ofs0_pf", "ofs0_vf", "ofs1_pf", "ofs1_vf" ]
          }
        ]
      }
    },

    "test": {
      "enabled": true,
      "platform": "Unit Test Platform",
  
      "devices": [
        { "name": "test0_pf", "id": [ "0x8086", "0xbeef", "0", "0" ] },
        { "name": "test0_vf", "id": [ "0x8086", "0xc01a", "0", "0" ] },
        { "name": "test1_pf", "id": [ "0x8086", "0xbee0", "0", "0" ] },
        { "name": "test1_vf", "id": [ "0x8086", "0xc01b", "0", "0" ] }
      ],
  
      "opae": {
        "opae.io": [
          {
            "enabled": true,
            "devices": [ "test0_pf", "test1_pf" ]
          }
        ]
      }
    }
  },

  "configs": [
    "mcp",
    "a10gx",
    "d5005",
    "n3000",
    "n5010",
    "n6000",
    "n6001",
    "c6100",
    "ofs",
    "test"
  ]

}'''


class TestEnv(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
      cls.cfg = Path().cwd().joinpath('opae.cfg')
      cls.cfg.write_text(TEST_CONFIG)

      assert cls.cfg.exists()
      assert cls.cfg.is_file()

      os.environ['LIBOPAE_CFGFILE'] = cls.cfg.as_posix()

      load_configs()

    @classmethod
    def tearDownClass(cls):
        cls.cfg.unlink()
        del os.environ['LIBOPAE_CFGFILE']

    def test_find(self):
      assert find_config_file() == os.environ['LIBOPAE_CFGFILE']

    def test_opae_io_is_supported(self):
        not_supported = [
                (0x8086, 0xc01a, 0, 0),
                (0x8086, 0xc01b, 0, 0)
                ]
        supported = [
                (0x8086, 0xbcbd, 0, 0),
                (0x8086, 0xbcc0, 0, 0),
                (0x8086, 0xbcc1, 0, 0),
                (0x8086, 0x09c4, 0, 0),
                (0x8086, 0x09c5, 0, 0),
                (0x8086, 0x0b2b, 0, 0),
                (0x8086, 0x0b2c, 0, 0),
                (0x8086, 0xbcce, 0x8086, 0x138d),
                (0x8086, 0xbccf, 0x8086, 0x138d),
                (0x8086, 0x0b30, 0, 0),
                (0x8086, 0x0b31, 0, 0),
                (0x1c2c, 0x1000, 0, 0),
                (0x1c2c, 0x1001, 0, 0),
                (0x8086, 0xbcce, 0x8086, 0x1770),
                (0x8086, 0xbccf, 0x8086, 0x1770),
                (0x8086, 0xbcce, 0x8086, 0x1771),
                (0x8086, 0xbccf, 0x8086, 0x1771),
                (0x8086, 0xbcce, 0x8086, 0x17d4),
                (0x8086, 0xbccf, 0x8086, 0x17d4),
                (0x8086, 0xaf00, 0x8086, 0),
                (0x8086, 0xaf01, 0x8086, 0),
                (0x8086, 0xbcce, 0x8086, 0),
                (0x8086, 0xbccf, 0x8086, 0),
                (0x8086, 0xbeef, 0, 0),
                (0x8086, 0xbee0, 0, 0)
                ]

        for n in not_supported:
            assert not Config.opae_io_is_supported(*n), 'No {:04x}:{:04x} {:04x}:{:04x}'.format(*n)

        for s in supported:
            assert Config.opae_io_is_supported(*s), 'Yes {:04x}:{:04x} {:04x}:{:04x}'.format(*s)

    def test_opae_io_platform_for(self):
        not_supported = [
                (0x8086, 0xc01a, 0, 0),
                (0x8086, 0xc01b, 0, 0)
                ]

        for n in not_supported:
            assert Config.opae_io_platform_for(*n) is None

        assert Config.opae_io_platform_for(0x8086, 0xbcbd, 0, 0) == 'Intel Integrated Multi-Chip Acceleration Platform'
        assert Config.opae_io_platform_for(0x8086, 0xbcc0, 0, 0) == 'Intel Integrated Multi-Chip Acceleration Platform'
        assert Config.opae_io_platform_for(0x8086, 0xbcc1, 0, 0) == 'Intel Integrated Multi-Chip Acceleration Platform'
        assert Config.opae_io_platform_for(0x8086, 0x09c4, 0, 0) == 'Intel Programmable Acceleration Card with Intel Arria 10 GX FPGA'
        assert Config.opae_io_platform_for(0x8086, 0x09c5, 0, 0) == 'Intel Programmable Acceleration Card with Intel Arria 10 GX FPGA'
        assert Config.opae_io_platform_for(0x8086, 0x0b2b, 0, 0) == 'Intel FPGA Programmable Acceleration Card D5005'
        assert Config.opae_io_platform_for(0x8086, 0x0b2c, 0, 0) == 'Intel FPGA Programmable Acceleration Card D5005'
        assert Config.opae_io_platform_for(0x8086, 0xbcce, 0x8086, 0x138d) == 'Intel FPGA Programmable Acceleration Card D5005'
        assert Config.opae_io_platform_for(0x8086, 0xbccf, 0x8086, 0x138d) == 'Intel FPGA Programmable Acceleration Card D5005'
        assert Config.opae_io_platform_for(0x8086, 0x0b30, 0, 0) == 'Intel FPGA Programmable Acceleration Card N3000'
        assert Config.opae_io_platform_for(0x8086, 0x0b31, 0, 0) == 'Intel FPGA Programmable Acceleration Card N3000'
        assert Config.opae_io_platform_for(0x1c2c, 0x1000, 0, 0) == 'Silicom FPGA SmartNIC N5010 Series'
        assert Config.opae_io_platform_for(0x1c2c, 0x1001, 0, 0) == 'Silicom FPGA SmartNIC N5010 Series'
        assert Config.opae_io_platform_for(0x8086, 0xbcce, 0x8086, 0x1770) == 'Intel Acceleration Development Platform N6000'
        assert Config.opae_io_platform_for(0x8086, 0xbccf, 0x8086, 0x1770) == 'Intel Acceleration Development Platform N6000'
        assert Config.opae_io_platform_for(0x8086, 0xbcce, 0x8086, 0x1771) == 'Intel Acceleration Development Platform N6001'
        assert Config.opae_io_platform_for(0x8086, 0xbccf, 0x8086, 0x1771) == 'Intel Acceleration Development Platform N6001'
        assert Config.opae_io_platform_for(0x8086, 0xbcce, 0x8086, 0x17d4) == 'Intel IPU Platform F2000X-PL'
        assert Config.opae_io_platform_for(0x8086, 0xbccf, 0x8086, 0x17d4) == 'Intel IPU Platform F2000X-PL'
        assert Config.opae_io_platform_for(0x8086, 0xaf00, 0x8086, 0) == 'Intel Open FPGA Stack Platform'
        assert Config.opae_io_platform_for(0x8086, 0xaf01, 0x8086, 0) == 'Intel Open FPGA Stack Platform'
        assert Config.opae_io_platform_for(0x8086, 0xbcce, 0x8086, 0) == 'Intel Open FPGA Stack Platform'
        assert Config.opae_io_platform_for(0x8086, 0xbccf, 0x8086, 0) == 'Intel Open FPGA Stack Platform'
        assert Config.opae_io_platform_for(0x8086, 0xbeef, 0, 0) == 'Unit Test Platform'
        assert Config.opae_io_platform_for(0x8086, 0xbee0, 0, 0) == 'Unit Test Platform'


class TestFindHome0(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
      cls.cfg = Path('~/.local').expanduser()
      if not cls.cfg.exists():
          cls.cfg.mkdir(parents=True)
      cls.cfg = cls.cfg.joinpath('opae.cfg')

      assert not cls.cfg.exists()

      cls.cfg.write_text(TEST_CONFIG)

      assert cls.cfg.exists()
      assert cls.cfg.is_file()

    @classmethod
    def tearDownClass(cls):
        cls.cfg.unlink()

    def test_find(self):
      assert find_config_file() == self.cfg.as_posix()


class TestFindHome1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
      cls.cfg = Path('~/.local/opae').expanduser()
      if not cls.cfg.exists():
          cls.cfg.mkdir(parents=True)
      cls.cfg = cls.cfg.joinpath('opae.cfg')

      assert not cls.cfg.exists()

      cls.cfg.write_text(TEST_CONFIG)

      assert cls.cfg.exists()
      assert cls.cfg.is_file()

    @classmethod
    def tearDownClass(cls):
        cls.cfg.unlink()

    def test_find(self):
      assert find_config_file() == self.cfg.as_posix()


class TestFindHome2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
      cls.cfg = Path('~/.config/opae').expanduser()
      if not cls.cfg.exists():
          cls.cfg.mkdir(parents=True)
      cls.cfg = cls.cfg.joinpath('opae.cfg')

      assert not cls.cfg.exists()

      cls.cfg.write_text(TEST_CONFIG)

      assert cls.cfg.exists()
      assert cls.cfg.is_file()

    @classmethod
    def tearDownClass(cls):
        cls.cfg.unlink()

    def test_find(self):
      assert find_config_file() == self.cfg.as_posix()
