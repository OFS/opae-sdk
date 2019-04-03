# super-rsu #

## SYNOPSIS ##
```console
super-rsu [-h] [-n] [--loglevel {trace,debug,error,warn,info,notset}]
          [--rsu-only] [--skip-rsu] [--force-flash] [--timeout TIMEOUT]
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
   images defined in the "flash" section in the rsu_config data.
   Each item in the "flash" section is a "flash spec" that contains:
   * The flash type ("user", "bmc_fw", "bmc_img", ...)
   * The filename of the image to flash. super-rsu will look for this file
     first in the same directory of the rsu_config file, and then look in the
     current working directory.
   * The version of the image.
   * An optional "force" indicator
4. Using the data in the "flash" section, the update routine involves:
  * For each spec in the "flash" section:
    1. Locate the file on the file system to use to flash.
    2. Compare the version listed in the "flash spec" to version reported by
       the target component.
    3. Create a task to call fpgaflash if either of the following conditions is
       met:
       * The "force" indicator is present and set to true
       * The version in the spec does not match the version reported by the
         system and a revision is included in the spec and the revision matches
	 the revision reported by the system.
  * For each task created from the "flash" section:
    1. Call fpgaflash with the command line arguments that correspond to the
       flash type and the file name in the spec used to create the task.
       This opens and controls the execution of fpgaflash in another process.

_NOTE_: Each update routine is run in a thread specific to a device located
on the PCIe bus. Every task in an update routine involves opening a new process
that is controlled and managed by its update routine thread. If a global timeout
is reached in the main thread, a termination request will be sent to each
thread performing the update. Consequently, the update routine will give the
current task extra time before terminating the process. There exists a risk of
a PAC device being left in an indeterminate state. In the case a flash process
of a user image is interrupted, commanding the device to perform an RSU command
should reload the image from the factory section of the flash. Interrupting a
factory flash may leave the device in a state that requires manual recovery.

## POSITIONAL ARGUMENTS ##
`rsu config`

Specifies the name of the file containing the RSU configuration (in JSON
format)


## OPTIONAL ARGUMENTS ##
`-h, --help`

   Print usage information.

`-n, --dry-run`

  Don't perform any updates, just a dry run.
  This will print out commands that can be executed
  in a Linux shell.

`--loglevel {trace,debug,error,warn,info,notset}`

  Log level to use. Default is 'info'.

`--rsu-only`

  Only perform the RSU command.

`--skip-rsu`

  Do not perform the RSU command.

`--force-flash`

  Flash all images regardless of versions matching or not.

`--timeout TIMEOUT`

  Maximum amount of time (sec) to wait before beginning termination
  sequence. Default is 1800.0 seconds (30 minutes).


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
    "additional_devices",
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
    "additional_devices": {
      "$id": "#/properties/additional_devices",
      "type": "array",
      "title": "The Additional_devices Schema",
      "items": [
          {
            "$id": "#/properties/additional_devices/vendor",
            "type": "string",
            "title": "The Additional Vendor Schema",
            "default": "",
            "examples": [
              "0x8086"
            ],
            "pattern": "^((0x)?[A-Fa-f0-9]{4})$"
	  },
          {
            "$id": "#/properties/additional_devices/device",
            "type": "string",
            "title": "The Additonal Device Schema",
            "default": "",
            "examples": [
              "0x0b32"
            ],
            "pattern": "^((0x)?[A-Fa-f0-9]{4})$"
	  }
        ]
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
          "force",
	],
        "properties": {
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
            "enum": ["user", "bmc_fw", "bmc_img"]
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
          }
        }
      }
    }
  }
}
```
