# super-rsu #

## SYNOPSIS ##
```console
super-rsu [-h] [-n] [--verify] | [ [--log-level {trace,debug,error,warn,info,notset}]
          [--log-file <filename>] [--rsu-only] [--with-rsu] [--force-flash] ]
 	  rsu_config
```


## DESCRIPTION ##
super-rsu is a tool that can be used for flashing image files and commanding an
Intel PAC device to perform RSU (remote system update - or a board reboot).
Performing an RSU on an Intel PAC device will cause it to reload any firmware
or programmable logic and restart its execution, a requirement for any updated
firmware or programmable logic to take effect.

At the core of super-rsu is its configuration file (referred to in this
document as 'rsu_config') which is essentially a manifest file for
identifying both the target device and the binary images (and their versions)
to be flashed.

At a high level, the flow of super-rsu should be:
1. Read and parse rsu_config file
2. Use product identifiers (like vendor, device and any additional vendor, device
   pairs that may be present in the PCIe bus) to locate all compatible
   devices on the PCIe bus.
3. For every device found on the system, update the device using the flash
   images defined in the "flash" section in the rsu_config data (or nvmupdate
   section).
   Each item in the "flash" section is a "flash spec" that contains:
   * The flash type ("user", "bmc_fw", "bmc_img", ...)
   * The filename of the image to flash. super-rsu will look for this file
     first in the same directory of the rsu_config file, and then look in the
     current working directory.
   * The version of the image.
   * An optional "force" indicator
   * An optional "requires" indicator
   The "nvmupdate" section is used to describe an Ethernet firmware file and
   its version.
4. Using the data in the "nvmupdate" and "flash" sections, the update routine
   involves:
  * If an "nvmupdate" section is present:
    1. Locate the file on the file system to use to flash the Ethernet device.
    2. Call nvmupdate to get an "inventory" of devices matching the vendor and
       device id in this section.
    3. Use this data to dynamically generate an nvmupdate compatible
       configuration file.
    4. Call nvmupdate with the generated configuration file to flash the
       Ethernet interfaces in the Vista Creek card (if version reported by
       system does not match the version in this section).
  * For each spec in the "flash" section:
    1. Locate the file on the file system to use to flash.
    2. Compare the version listed in the "flash spec" to version reported by
       the target component.
    3. Create a task to call fpgaflash if either of the following conditions is
       met (and the revision specified is compatible):
       * The "force" indicator is present and set to true.
       * The version in the spec does not match the version reported by the
         system OR the flash type is factory type.
  * For each task created from the "flash" section:
    1. Call fpgaflash with the command line arguments that correspond to the
       flash type and the file name in the spec used to create the task.
       This opens and controls the execution of fpgaflash in another process.

_NOTE_: If the system reports a revision for one of the components
being flashed, this revision must be in the set of revisions listed
in the manifest.
Example: if the system reports 'a' for bmc_img and the manifest includes
'ab', then the image will be flashed.

_NOTE_: Each update routine is run in a thread specific to a device located
on the PCIe bus. Every task in an update routine involves opening a new process
that is controlled and managed by its update routine thread.
If a task includes a timeout and the timeout is reached, a termination request
will be sent to its process and it will be counted as a failure. If a global
timeout is reached in the main thread, a termination request will be sent to each
thread performing the update. Consequently, the update routine will give the
current task extra time before terminating the process.
The RSU operation will only be performed if requested with either `--with-rsu`
command line argument or with the `--rsu-only` command line argument.
The former will perform the RSU command upon successful completion of flash
operations. The latter will skip the process of version matching and flashing
images and will only perform the RSU command. It is recommended that super-rsu
be executed again if any flash operation is interrupted.

## POSITIONAL ARGUMENTS ##
`rsu config`

Specifies the name of the file containing the RSU configuration (in JSON
format)


## OPTIONAL ARGUMENTS ##
`-h, --help`

   Print usage information.

`--verify`

  Compare versions of flashable components on the system against the manifest.
  Return non-zero exit if compatible components are not up to date.

`-n, --dry-run`

  Don't perform any updates, just a dry run.
  This will print out commands that can be executed
  in a Linux shell.

`--log-level {trace,debug,error,warn,info,notset}`

  Log level to use. Default is 'info'.

`--log-file <filename> (default: /tmp/super-rsu.log)

  Emit log messages (with DEBUG level) to filename
  _NOTE_: The default log file (/tmp/super-rsu.log) is set to rollover every
  time super-rsu is executed. This will create numbered backups before
  truncating the log file. The maximum number of backups is 50.

`--rsu-only`

  Only perform the RSU command.

`--with-rsu`

  Perform RSU after updating flash components(experimental)

`--force-flash`

  Flash all images regardless of versions matching or not.


## CONFIGURATION ##
The following is the JSON schema expected by super-rsu. Any deviance from
this schema may result in errors executing super-rsu.

```JSON
{
  "definitions": {},
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "http://example.com/root.json",
  "type": "object",
  "title": "The Root Schema",
  "required": [
    "product",
    "vendor",
    "device",
    "flash"
  ],
  "optional": [
    "nvmupdate",
  ],
  "properties": {
    "product": {
      "$id": "#/properties/product",
      "type": "string",
      "title": "The Product Schema",
      "default": "",
      "examples": [
        "n3000"
      ],
      "pattern": "^(.*)$"
    },
    "vendor": {
      "$id": "#/properties/vendor",
      "type": "string",
      "title": "The Vendor Schema",
      "default": "",
      "examples": [
        "0x8086"
      ],
      "pattern": "^((0x)?[A-Fa-f0-9]{4})$"
    },
    "device": {
      "$id": "#/properties/device",
      "type": "string",
      "title": "The Device Schema",
      "default": "",
      "examples": [
        "0x0b30"
      ],
      "pattern": "^((0x)?[A-Fa-f0-9]{4})$"
    },
    "nvmupdate": {
      "$id": "#/properties/nvmupdate",
      "type": "object",
      "title": "The nvmupdate Schema",
      "required": [
        "vendor",
        "device",
        "filename",
        "version"
      ],
      "optional": [
        "interfaces"
      ],
      "properties": {
        "vendor": {
          "$id": "#/properties/nvmupdate/vendor",
          "type": "string",
          "title": "The nvmupdate Vendor Schema",
          "default": "",
          "examples": [
             "0x8086"
          ],
          "pattern": "^((0x)?[A-Fa-f0-9]{4})$"
        },
        "device": {
          "$id": "#/properties/nvmupdate/device",
          "type": "string",
          "title": "The nvmupdate Device Schema",
          "default": "",
          "examples": [
            "0x0d58"
          ],
          "pattern": "^((0x)?[A-Fa-f0-9]{4})$"
        },
        "interfaces": {
          "$id": "#/properties/nvmupdate/interfaces",
          "type": "number",
          "title": "The nvmupdate Interfaces Schema",
          "default": "1",
          "examples": [
            2, 4
          ]
        },
        "filename": {
          "$id": "#/properties/nvmupdate/filename",
          "type": "string",
          "title": "The nvmupdate Filename Schema",
          "default": "",
          "examples": [
            "PSG_XL710_6p80_XLAUI_NCSI_CFGID2p61_Dual_DID_0D58_800049C6.bin"
          ],
          "pattern": "^(.*)$"
        },
        "version": {
          "$id": "#/properties/nvmupdate/version",
          "type": "string",
          "title": "The nvmupdate Version Schema",
          "default": "",
          "examples": [
            "800049C6"
          ],
          "pattern": "^((0x)?[A-Fa-f0-9]{8})$"
        },
        "timeout": {
          "$id": "#/properties/nvmupdate/timeout",
          "type": "string",
          "title": "The Timeout Schema",
          "default": "",
          "examples": [
            "10m"
          ],
          "pattern": "^([0-9]+(\\.[0-9]+)?([dhms]))+$"
        }
      }
    },
    "flash": {
      "$id": "#/properties/flash",
      "type": "array",
      "title": "The Flash Schema",
      "items": {
        "$id": "#/properties/flash/items",
        "type": "object",
        "title": "The Items Schema",
        "required": [
          "filename",
          "type",
          "version",
          "revision"
        ],
	"optional": [
          "enabled",
          "force",
	  "timeout",
	  "requires"
	],
        "properties": {
	  "enabled": {
            "$id": "#/properties/flash/items/properties/enabled",
	    "type": "boolean",
	    "title": "The Enabled Schema",
	    "default": "true"
	  },
          "filename": {
            "$id": "#/properties/flash/items/properties/filename",
            "type": "string",
            "title": "The Filename Schema",
            "default": "",
            "examples": [
              "vista_creek_qspi_xip_v1.0.6.ihex"
            ],
            "pattern": "^(.*)$"
          },
          "type": {
            "$id": "#/properties/flash/items/properties/type",
            "type": "string",
            "title": "The Type Schema",
            "default": "",
            "examples": [
              "bmc_fw"
            ],
            "enum": ["user", "bmc_fw", "bmc_img", "dtb", "factory_only",
	    "phy_eeprom"]
          },
          "version": {
            "$id": "#/properties/flash/items/properties/version",
            "type": "string",
            "title": "The Version Schema",
            "default": "",
            "examples": [
              "1.0.6"
            ],
            "pattern": "^\\d+\\.\\d+\\.\\d+$"
          },
          "force": {
            "$id": "#/properties/flash/items/properties/force",
            "type": "boolean",
            "title": "The Force Schema",
            "default": false,
            "examples": [
              true
            ]
          },
          "revision": {
            "$id": "#/properties/flash/items/properties/revision",
            "type": "string",
            "title": "The Revision Schema",
            "default": "",
            "examples": [
              "C"
            ],
            "pattern": "^([A-Za-z])$"
          },
          "timeout": {
            "$id": "#/properties/nvmupdate/timeout",
            "type": "string",
            "title": "The Timeout Schema",
            "default": "",
            "examples": [
              "10m"
            ],
            "pattern": "^([0-9]+(\.[0-9]+)?([dhms]))+$"
          },
          "requires": {
            "$id": "#/properties/flash/items/properties/requires",
            "type": "string",
            "title": "The Requires Schema",
            "default": "",
            "examples": [
              "bmc_img >= 1.0.12"
            ],
            "pattern": "^(([a-z_]+) ((<>!=)?=) ([0-9a-z\\.]+)$"
          }
        }
      }
    }
  }
}
```
