# qpafilter #

## SYNOPSIS ##
```console
qpafilter [-h] [-v] [-s SENSOR_MAP] {create,dump} ...
```

## DESCRIPTION ##

qpafilter is a tool that, given sensor values as determined by
Quartus Power Analyzer, creates a binary file that can be used
by the FPGA BMC (Board Management Controller) firmware to set
sensor thresholds. In addition to creating this binary file,
qpafilter can parse it and dump it in a human-readable format.

## SUB-COMMANDS ##
create
 Transform qpa text file into a binary format for the BMC FW.

dump
 Parse an existing binary file and dump in a human-readable format.

### Mode 1: create ###

```console
qpafilter create [-t MIN_TEMP] [-f VIRT_FATAL_TEMP] [-w VIRT_WARN_TEMP] [-O OVERRIDE_TEMP] [-o OUTPUT] [file]
```

Convert the input sensors text file to a binary file suitable for
consumption by the Board Management Controller. The format of the
input sensors text file is that produced by the Quartus Power
Analyzer. Following is a brief example input file. The temperature
sensor label is followed by the temperature sensor Upper Fatal
value and units.

+-----------------------------------------------------------+<br>
; Temperature and Cooling                                   ;<br>
+-----------------------------------------------+-----------+<br>
; FPGA Core dts01 Temperature                   ; 55.0 °<br>
; FPGA Core dts11 Temperature                   ; 54.5 °<br>
<br>
...<br>
<br>
; HSSI_0_1 dts4 Temperature                     ; 90.7 °<br>
+-----------------------------------------------+-----------+<br>

#### Mode 1: create POSITIONAL ARGUMENTS ####
`create`
selects create mode

`file`
the input sensors text file

#### Mode 1: create OPTIONAL ARGUMENTS ####
`-t,--min-temp MIN_TEMP`
select the minimum temperature for the input temperature data.
If an individual temperature is below the minimum temperature
value, qpafilter displays a warning. If all of the input
temperatures are below the minimum, qpafilter displays an
error and halts execution.

`-f,--virt-fatal-temp VIRT_FATAL_TEMP`
specify the virtual sensor Upper Fatal value. The sensor
Upper Warning threshold values are calcuated from the ratio of
the virtual sensor Upper Warning threshold:the virtual sensor
Upper Fatal threshold.

`-w,--virt-warn-temp VIRT_WARN_TEMP`
specify the virtual sensor Upper Warning value. The sensor
Upper Warning threshold values are calculated from the ratio of
the virtual sensor Upper Warning threshold:the virtual sensor
Upper Fatal threshold.

`-O,--override-temp OVERRIDE_TEMP`
specify a temperature sensor override of the form label:percentage,
where label is the label given to the temperature sensor in the
input sensor text file; and percentage is a number between 0 and
100.

example: --override-temp='HSSI_0_1 dts4 Temperature:50'

This will calculate the Upper Warning value of the sensor labeled
"HSSI_0_1 dts4 Temperature" as 50% of the input Upper Fatal value.

`-o,--output OUTPUT`
specify the output binary file name. The default OUTPUT value is
qpafilter.blob.

`-s,--sensor-file SENSOR_MAP`
specify the name of the input sensors map file. The default value
is n5010_bmc_sensors.yml. This file contains the mapping from
sensor labels to sensor IDs. A sample input follows:

FPGA Core dts01 Temperature: 8<br>
FPGA Core dts11 Temperature: 9<br>
<br>
...<br>
<br>
HSSI_0_1 dts4 Temperature: 5<br>

#### Mode 1: examples ####

```console
$ qpafilter create thermal.txt
```

 Uses default settings to create qpafilter.blob from thermal.txt.

```console
$ qpafilter create --override-temp='FPGA Core dts11 Temperature:75' -o qpafilter.bin thermal.txt
```

 Create qpafilter.bin from thermal.txt, calculating the Upper Warning
 temperature for sensor "FPGA Core dts11 Temperature" as 75% of the
 Upper Fatal value.

### Mode 2: dump ###

```console
qpafilter dump [-o OUTPUT] [-F {csv,json,yaml}] [file]
```

Convert the input binary file to human-readable output. The
input binary file must have been previously created by a
qpafilter create ... command.

#### Mode 2: dump POSITIONAL ARGUMENTS ####
`file`
the input binary file. This file must have been created by
using qpafilter create ...

#### Mode 2: dump OPTIONAL ARGUMENTS ####
`-o,--output OUTPUT`
specify the output human-readable file name.

`-F,--format (csv|json|yaml)`

specify the human-readable output format. The default is
Comma-Separated Value (csv). JSON and YAML are also supported.

`-s,--sensor-file SENSOR_MAP`
specify the name of the input sensors map file. The default value
is n5010_bmc_sensors.yml. This file contains the mapping from
sensor labels to sensor IDs. A sample input follows:

FPGA Core dts01 Temperature: 8<br>
FPGA Core dts11 Temperature: 9<br>
<br>
...<br>
<br>
HSSI_0_1 dts4 Temperature: 5<br>

#### Mode 2: examples ####

```console
qpafilter dump qpafilter.blob
```

 uses default settings to produce csv output for binary qpafilter.blob.

```console
qpafilter dump --format=yaml qpafilter.bin
```

 displays the sensor data found in qpafilter.bin in yaml format.
