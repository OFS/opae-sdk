# fpgaotsu #

## SYNOPSIS ##

`fpgaotsu.py [-h] [--rsu-only] [--rsu] [-v] fpgaostu.json`

## DESCRIPTION ##
One-time update tool updates non Root of Trust PCIe FPGA cards to Root of Trust (RoT) PCIe FPGA cards. Tool updates NIOS factory, NIOS user, BMC root key hash, SR key hash, MAX10 device tree, FPGA factory image, FPGA user, NIOS boot loader, MAX10 factory and MAX10 user to Root of Trust firmware.


### POSITIONAL ARGUMENTS ###
`{fpgaostu.json}`

Specifies the input json file or manifest.

### OPTIONAL ARGUMENTS ###
`-h, --help`

   Print usage information.

`--rsu-only`

  Reset fpga pcie card and loads updated firmware.

`--rsu`

  Upadtes fpga with Root of Trust firmware and Reset fpga pcie card.

  `-v --verbose`

  Enables debug logging.


### EXAMPLE ###

`fpgaotsu.py /usr/share/opae/d5005/one-time-update/fpgaotsu_d5005.json`

 Programs all FPGA D5005 cards with Root of Trust firmware.

`fpgaotsu.py /usr/share/opae/n3000/one-time-update/fpgaotsu_d3000.json`

 Programs all FPGA N3000 cards with Root of Trust firmware.

#### FPGA D5005 card:####
User has to power cycle system after updateing Root of Trust with one-time update tool.

#### FPGA N3000 card:####
One-time update tool reset fpga pcie card two times after udpating RoT firmware, Boot to Max10 factory image after first reset and boot to FPGA factory image after second reset.
No need to power cycle system, fpga card reset loads to Root of Trust firmware.


## Testing steps ##

### one-time update install steps: ###

   1. Install from opae-one-time-update.rpm
      This should install dependencies:
      * opae-intel-fpga-driver
      * opae-tools-extra
      * Security-Tools
   2. Verify one-time update files (firmware and manifest) installed in
      * /usr/share/opae/n3000/one-time-update
      * /usr/share/opae/d5005/one-time-update

   3. Update FPGA N3000 card with temporary max10 image to accees second flash.
      1. sudo fpgaflash bmc_img usr/share/opae/n3000/max10_system_revd_dual_v111.1.13_temporary_cfm0_auto.rpd --rsu
      2. Verify sysfs attributes /sys/class/fpga/intel-fpga-dev.*/intel-fpga-fme.*/spi-altera.*.auto/spi_master/spi*/spi*.*/intel-generic-qspi.*.auto

   4. Run "fpgaotsu.py /usr/share/opae/n3000/one-time-update/fpgaotsu_d5005.json" for intel fpga N3000 card or 
      Run "fpgaotsu.py /usr/share/opae/d5005/one-time-update/fpgaotsu_d5005.json" for intel fpga D5005 card
      * Updates Nios, MAX10 and FPGA images for FPGA N3000 and D5005.
      * User has to power cycle FPGA D5005 card system after updating Root of Trust firmware.


### one-time update tool input specs: ###

#### fpgaotsu_n3000.json: ####

#### fpgaotsu_d5005.json: ####
