# fpgaotsu #

## SYNOPSIS ##

`fpgaotsu.py [-h] [--rsu-only] [--rsu] [-v] fpagostu_json`

## DESCRIPTION ##
Fpgaotsu is one time update tool update from non-RoT firmware to RoT firmware for Intel PCIe FPGA cards.Tool updates NIOS factory, NIOS user, BMC root key hash, SR key hash, MAX10 device tree, FPGA factory image, FPGA user, NIOS boot loader, MAX10 factory and MAX10 user to RoT firmware.


### POSITIONAL ARGUMENTS ###
`{fpagostu_json}`

Specifies the input josn file or manifest.

### OPTIONAL ARGUMENTS ###
`-h, --help`

   Print usage information

`---rsu-only`

  Reset fpga pcie card and loads updated firmware.

`---rsu`

  Upadtes fpga with RoT firmware and Reset fpga pcie card 

  `-v ---verbose`

  Enables debug logging


### EXAMPLE ###

`fpgaotsu.py /usr/share/opae/d5005/one-time-update/fpgaotsu_d5005.json`

 Programs all FPGA d5005 cards with RoT firmware

`fpgaotsu.py /usr/share/opae/n3009/one-time-update/fpgaotsu_dn3000.json`

 Programs all FPGA n300 cards with RoT firmware

#### FPGA D5005 card:####
User has to power cycle system after update RoT with one time update tool

#### FPGA N3000 card:####
One time update tool reset fpga pcie card two times , first time it load Max10 Image factory firmware and Second time it load FPGA factory image.
No need to power cycle system, automatically loads to RoT firmware


## Testing steps ##

### one time udpate rpm steps: ###

   To create the ontime udpate RPM, the following must be done:
   1. clone OPAE/super-rsu repository
   2. Modify the flash specs for the three image types in fogaotsu-n3000.json or fpgaotsu-d5005.json
   3. Create the super rsu RPM.
      1. Call create-rpm bash script.
      2. The RPM will be available in ~/rpmbuild/RPMS/noarch


### one time udpate install steps: ###

   1. Install from opae-onetime-udpate.rpm
      This should install dependencies:
      * opae-intel-fpga-driver
      * opae-tools-extra
      * Security-Tools
   2. Verify one time udpate files (firmware and manifest) installed in
      * /usr/share/opae/n3000/one-time-update
      * /usr/share/opae/d5005/one-time-update

   3. Update FPGA N3000 card with tempoery max10 image to accees second flash
      1. sudo fpgaflash bmc_img usr/share/opae/n3000/max10_system_revd_dual_v111.1.13_temporary_cfm0_auto.rpd --rsu
      2. Verify sysfs attributes /sys/class/fpga/intel-fpga-dev.*/intel-fpga-fme.*/spi-altera.*.auto/spi_master/spi*/spi*.*/intel-generic-qspi.*.auto

   4. Run "fpgaotsu.py /usr/share/opae/n3000/one-time-update/pgaotsu_d5005.json" for intel fpga n3000 card or 
      Run "fpgaotsu.py /usr/share/opae/d5005/one-time-update/pgaotsu_d5005.json" for intel fpga d5005 card
      * Updates Nios, MAX10 and FPGA images for FPGA N3000 and D5005
      * User has to power cycle system after update RoT


### One time update tool input specs: ###

#### fpgaotsu_n3000.json: ####

```fpgaotsu_n3000.json
{
  "product": "n3000",
  "vendor": "0x8086",
  "device": "0x0b30",

  "flash": [
    {
      "filename": "vista_dev_bmc_root_hash.raw32",
      "type": "bmc_root_key_hash",
      "start": "0x7FFC004",
      "end": "0x7FFCFFF"
    },
    {
      "filename": "root_key_programed",
      "type": "bmc_root_key_program",
      "start": "0x7ffC000",
      "end": "0x7ffC004"
    },
    {
      "filename": "vista_dev_fim_root_hash.raw32",
      "type": "sr_root_key_hash",
      "start": "0x7FFD004",
      "end": "0x7FFDFFF"
    },
    {
      "filename": "root_key_programed",
      "type": "sr_root_key_program",
      "start": "0x7ffD000",
      "end": "0x7ffD004"
    },
    {
      "filename": "vista_rot_xip_factory.bin",
      "type": "nios_factory",
      "start": "0x3800000",
      "end": "0x3A00FF0"
    },
    {
      "filename": "vista_rot_xip_factory_header.bin",
      "type": "nios_factory_header",
      "start": "0x3A00FF0",
      "end": "0x3A00FFF"
    },
    {
      "filename": "vista_fpga_factory_reverse.bin",
      "type": "fpga_factory",
      "start": "0x0020000",
      "end": "0x381FFFF"
    },
    {
      "filename": "vista_fpga_factory_reverse.bin",
      "type": "fpga_user",
      "start": "0x4000000",
      "end": "0x77FFFFF"
    },
    {
      "filename": "max10_system_revd_rot_dual_v2.15.18_ufm_bootloader_reversed.rpd",
      "type": "nios_bootloader",
      "start": "0x08000",
      "end": "0x0FFFF"
    },

    {
      "filename": "max10_system_revd_rot_dual_v2.15.18_cfm1_factory_reversed.rpd",
      "type": "max10_factory",
      "start": "0x10000",
      "end": "0xB7FFF"
    },

    {
      "filename": "max10_system_revd_rot_dual_v2.15.18_cfm0_user_reversed.rpd",
      "type": "max10_user",
      "start": "0xB8000",
      "end": "0x15FFFF"
    },

    {
      "filename": "intel-pac-n3000.dtb",
      "type": "bmc_dts",
      "start": "0x3820000",
      "end": "0x3FFFFFF"
    },

    {
      "filename": "vista_rot_xip_user.bin",
      "type": "nios_user",
      "start": "0",
      "end": "0x7EFFFF"
    },
    {
      "filename": "vista_rot_xip_user_header.bin",
      "type": "nios_user_header",
      "start": "0x7F0000",
      "end": "0x7F000F"
    }

  ]
}
```

#### fpgaotsu_d5005.json: ####

```fpgaotsu_d5005.json
{
  "product": "d5005",
  "vendor": "0x8086",
  "device": "0x0b2b",

  "flash": [
    {
      "filename": "darby_dev_bmc_root_hash.raw32",
      "type": "bmc_root_key_hash",
      "start": "0x7FFC004",
      "end": "0x7FFCFFF"
    },
    {
      "filename": "root_key_programed",
      "type": "bmc_root_key_program",
      "start": "0x7ffC000",
      "end": "0x7ffC004"
    },
    {
      "filename": "darby_dev_fim_root_hash.raw32",
      "type": "sr_root_key_hash",
      "start": "0x7FFD004",
      "end": "0x7FFDFFF"
    },
    {
      "filename": "root_key_programed",
      "type": "sr_root_key_program",
      "start": "0x7ffD000",
      "end": "0x7ffD004"
    },
    {
      "filename": "max10_darby_rot_v2.16.3_ufm_bootloader_reversed.rpd",
      "type": "nios_bootloader",
      "start": "0x08000",
      "end": "0x0FFFF"
    },
    {
      "filename": "darby_rot_xip_factory.bin",
      "type": "nios_factory",
      "start": "0xB800000",
      "end": "0x0BA00FF0"
    },
    {
      "filename": "darby_rot_xip_factory_header.bin",
      "type": "nios_factory_header",
      "start": "0x0BA00FF0",
      "end": "0xBA00FFF"
    },
    {
      "filename": "dcp_2_0_0x202000200000159_page0_reversed.bin",
      "type": "fpga_factory",
      "start": "0x20000",
      "end": "0x381FFFF"
    },
    {
      "filename": "dcp_2_0_0x202000200000159_page0_reversed.bin",
      "type": "fpga_user",
      "start": "0x4000000",
      "end": "0x77FFFFF"
    },
    {
      "filename": "max10_darby_rot_v2.16.3_cfm0_factory_reversed.rpd",
      "type": "max10_factory",
      "start": "0xB8000",
      "end": "0x15FFFF"
    },
    {
      "filename": "max10_darby_rot_v2.16.3_cfm1_user_reversed.rpd",
      "type": "max10_user",
      "start": "0x10000",
      "end": "0xB7FFF"
    },
    {
      "filename": "intel-pac-darby-creek-bmc.dtb",
      "type": "bmc_dts",
      "start": "0x3820000",
      "end": "0x3FFFFFF"
    },
    {
      "filename": "darby_rot_xip_user.bin",
      "type": "nios_user",
      "start": "0",
      "end": "0x7EFFFF"
    },
    {
      "filename": "darby_rot_xip_user_header.bin",
      "type": "nios_user_header",
      "start": "0x7F0000",
      "end": "0x7F0010"
    }
  ]
}
```