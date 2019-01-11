# hssi_config #

## Synopsis ##
```hssi_config``` reads or writes HSSI registers on either on an Intel&reg; FPGA using the 
FPGA Interface Manager (FIM) or on an HSSI retimer card attached to the board.
```hssi_config``` is only available for the Integrated FPGA Platform. You cannot run it 
on the PCIe accelerator card (PAC).

## Usage ##
`hssi_config [--resource|-r <sysfs resource>] [--socket-id|s 0|1] command [command options]`

Where command is one of the following:

```sh
    dump [outfile.csv] [--input-file inputfile.csv]
    iread instance (0,1) device-addr byte-address byte-count
    iwrite instance (0,1) device-addr byte-address byte1 [byte2 [byte3...]]
    load [inputfile.csv] [--c-header]
    read lane(0-15) reg-address
    rread device(0x30, 0x32, 0x34, 0x36) channel(0-3) address
    rwrite device(0x30, 0x32, 0x34, 0x36) channel(0-3) address value
    test (rd|rw) inputfile.csv [--acktimes] [--repeat N]
    write lane(0-15) reg-address value
```

The first argument is the command and any additional arguments are command arguments.
The following options and commands are available:

### Options ###

`[--resource|-r <sysfs resource path>`

The resource path in the sysfs pseudo-filesystem.
Example:
    `/sys/devices/pci0000\:5e/0000\:5e\:00.0/resource0`

`[--socket-id 0|1]`

The socket id of the target FPGA.
Required on two-socket systems to differentiate between the two possible target FPGAs.

### Commands ###
`dump [outfile.csv] [--input-file inputfile.csv]`

Dump registers to stdout or to a file, if provided. ```hssi_config``` has a built-in set of registers to
dump. The first argument is the path to a file to write. The command dumps to stdout if you do not 
specify a file name. Use the --input-file option to specify a different set of registers. 
    
`load [inputfile.csv] [--c-header]`

Load a set of register values from either stdin or an input file, if provided. The first argument 
is the path to a file containing the registers to load. Loads from stdin if omitted.  
    
Use --c-header to generate a C header file with an array of 64-bit numbers to write to the 
```HSSI_CTRL``` register. This header file can substitute for the input file. 
NOTE: You must perform the acknowledge routine after each write.

`read lane(0-15) reg-address`

Read from a single XCVR (transceiver) register.
The first command argument is the XCVR lane. Use -1 to specify a read from all lanes.
The second argument is the XCVR address (offset).

`write lane(0-15) reg-address value`

Write to a single XCVR register.
The first argument is the XCVR lane.
The second argument is the XCVR address(offset).
The third argument is the value to write to the register.

`rread device(0x30, 0x32, 0x34, 0x36) channel(0-3) address`

Read from a single retimer register.
The first argument is the I2C device address.
The second argument is the channel.
The third argument is the register address (or I2C byte address).

`rwrite device(0x30, 0x32, 0x34, 0x36) channel(0-3) address value`

Write to a single retimer register.
The first argument is the I2C device address.
The second argument is the channel.
The third argument is the register address (or I2C byte address).
The fourth argument is the value to write.

`iread instance (0,1) device-addr byte-address byte-count`

Read from a device on the I2C bus.
The first argument is the I2C controller instance (0 or 1).
The second argument is the device address to read from.
The third argument is the byte address of the register to read from the device.
The fourth argument is the number of bytes to read.

`iwrite instance (0,1) device-addr byte-address byte1 [byte2 [byte3...]]`

Write to a device on the I2C bus.
The first argument is the I2C controller instance (0 or 1).
The second argument is the device address to read from.
The third argument is the byte address of the register to read from the device.
All subsequent arguments are the bytes to write to the device.

`test (rd|rw) inputfile.csv [--acktimes]`

Perform built-in test for reading or writing XCVR registers.
The first argument is the path to a file containing the registers to test.

## Overview ##

The ```hssi_config``` utility reads or writes hssi equalization parameters stored in either
the transceiver (XCVR) registers or the registers of the retimer on the I2C bus. To access registers, 
the hssi controller writes to the ```HSSI_CTRL``` register and reads from the ```HSSI_STAT```
register in the FPGA Management Engine (FME). These two registers implement the HSSI AUX bus
mailbox protocol to access devices on other buses. Because ```hssi_config``` maps the FME MMIO
space directly, the FPGA driver is not required.

## Locating the FME Device ##

The FME reads and writes the ```HSSI_CTRL``` and ```HSSI_STAT``` registers on the NIOS device.
The FME maps the MMIO address space of the FME identified by its resource in the sysfs psuedo-filesystem
in Linux operating systems. To identify resource paths, use the ```lspci``` utility to query for Intel
devices with device id of bcc0.

Example:

`lspci -d 8086:bcc0`

This command should print out at least one line like the following example:

`5e:00.0 Processing accelerators: Intel Corporation Device bcc0`

Use the first three numbers (bus:device.function) to locate the device resource
in the sysfs filesystem:

`/sys/devices/pci0000\:<bus>/0000\:<bus>\:<device>.<function>/resource0`

For example, the example above with bus of 5e, device of 00 and function of 0 would
use a resource path as follows:

`/sys/devices/pci0000\:5e/0000\:5e\:00.0/resource0`


## CSV File Format ##

Any CSV file parsed by ```hssi_config``` must meet have at least four columns. The following table provides
the column specifications:


| Column | Name                        | Description                                                                                  |
|:------:|-----------------------------|----------------------------------------------------------------------------------------------|
| 1      | Register type.              | Can be either ```FPGA_RX```, ```FPGA_TX```, ```RTMR_RX```, ```RTMR_TX``` (or their corresponding numeric values, 1-4).|
| 2      | Lane or Channel             | 0-15 for XCVR lanes on FPGA, 0-3 for retimer channels. -1 to designate all lanes or channels.|
| 3      | Device address (on I2C bus) | Only applies to retimer registers. 0 - 3, -1 to designate all devices.                        |
| 4      | Register address (or offset)| Examples: 0x213, 0x100.                                                                      |
| 5      | Register value to write     | Examples: 0x1, 1, 0. Applies only when loading or writing registers.                            |



## Examples of Commands ##

### Dumping Registers ###

Dump default register set to stdout:

`>hssi_config dump`

Dump default registers to a file, data.csv:

`>hssi_config dump data.csv`

Dump to an output file, data.csv, Registers specified in an input file, regspec.csv:

`>hssi_config dump data.csv --input-file regspec.csv`

#### Reading Single Registers ####

Read register from XCVR at 0x2e1 on lane 0:

`>hssi_config read 0 0x2e1`

Read register 0x109 from retimer on channel 0, device 0x30, channel 1:

`>hssi_config rread 0 0x30 0x109`

### Loading Registers ###

Load registers specified in an input file called data.csv:

`>hssi_config load data.csv`

Load registers specified from stdin:

`>hssi_config load`

```
FPGA_RX,1,-1,0x213,0
FPGA_RX,2,-1,0x213,0
FPGA_RX,3,-1,0x213,0
<CTRL-D>
```

#### Writing Single Registers ####

Write 1 to XCVR register at 0x2e1 on lane 0:

`>hssi_config write 0 0x2e1 1`

Read register 0x109 from retimer on channel 0, device 0x30, channel 1:

`>hssi_config rread 0 0x30 0x109`

## Testing HSSI Read and Write ##


`>  test (rd|rw) register-file.csv [--acktimes]`

`rd|wr`

Specifies either a `rd` or `wr` of transceiver registers. For writes,
every register in the file is read from and written to in the following sequence:

  1. Read the register, save the value
  2. Write 1 to the register
  3. Read the register, verify that the register value is 1
  4. Write 0 to the register
  5. Read the register, verify that the register value is 0
  6. Write the original value to the register
  7. Read the register, assert it is the original value

`register=file.csv`

Specifies the path to a file containing the set of registers to test. 

`--acktimes ` 

Specifies the time spent in the `ack` routine. When measured, a summary of ack times prints 
to stdout. This argument is optional.

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | Corrected typos.  | 
