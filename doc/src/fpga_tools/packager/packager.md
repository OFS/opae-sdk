# packager #

## SYNOPSIS ##

`packager <cmd> [options]`

## DESCRIPTION ##
The packager provides tools to help AFU developers create Accelerator Function files. The Accelerator Function file is the programming file for an AFU in the Intel® Acceleration Stack for FPGAs platform. It includes metadata the packager tool generates and a raw binary file (.rbf) that the Intel Quartus Prime software generates. 

Currently, the packager's only function is to create an AFU file. Refer to Section 2 for more information about the packager. The packager depends on a JSON file to describe AFU metadata. Refer to Section 3 for more information about the JASON file.

**The packager requires Python 2.7.1 and Python 2.7.3. The tool indicates if it is being called with a compatible version of Python.**

## PACKAGER INTERFACE ##

The packager is a command line tool with the following interface:

`$ packager <CMD> [options]`

To display the list of commands for the packager, run:

`$ packager help`

To display help for a specific command, run:

`$ packager <CMD> --help`

To display the version of the packager tool, run:

`$ packager version`

To generate an AFU file, run:

`$ packager create-gbs --rbf=<RBF_PATH> --afu=<AFU_JSON_PATH> --gbs=<GBS_PATH>`

<RBF_PATH> is the path to the RBF file for the AFU. The Quartus Prime software generates this RBF by compiling the AFU. 
<AFU_JSON_PATH> is the path to the Accelerator Description file. This is a JSON file that describes the metadata that is appended to the AFU.
<GBS_PATH> is the path to the RBF file for the FPGA interface manager (FIM) that contains the FPGA interface unit and other interfaces.

**TIP**: JSON files can be very particular about things like trailing commas. If you’re getting errors, try using jsonlint.com to validate that your JSON is formatted correctly. 

To modify metadata in an existing AFU file, run the following command:

`$ packager modify-gbs --input-gbs=<PATH_TO_GBS_TO_BE_MODIFIED> --outputgbs=<NAME_FOR_NEW_GBS> --set-value <key>:<value>`

You can pass in a number of <key>:<value> pairs with --set-value to update values in “afu- image”. 

To print the metadata of an existing AFU: 

`$ packager get-info --gbs=<GBS_PATH>` 

To extract the RBF from the AFU:

`$ packager get-rbf --gbs=<GBS_PATH> --rbf=<NAME_FOR_RBF>`

## Accelerator Description File ##

This Accelerator Description File is a JSON file that describes an AFU. Its currently describes the metadata associated with a AFU. This metadata is used by the OPAE during reconfiguration. Here is an example file:

```
{
   "version": 1
   "platform-name": "DCP",
   "afu-image": {
      "magic-no": 488605312,
      "interface-uuid": "576c885b-0a4f-549b-ca2e-3b2e12d579f7",
      "clock-frequency-low": "50",
      "clock-frequency-high": "200",
      "power" = 10,
      "afc-clusters": [{
         "total-contexts": 1,
         "name": "nlb_400",
         "afc-type-uuid": "c000c966-0d82-4272-9aef-fe5f84570612"
      }
      ]  
   }
}
```

The clock frequency fields are for OpenCL which requires PLL reconfiguration. Quartus Prime compilation derives a set of PLL parameter values to update the PLL Dynamic Reconfiguration Block. The metadata stores these parameter values. After reprogramming the AFU using partial reconfiguration (PR), the software driver reconfigures the PLL by writing these parameter values over PCIe/CCI. No kernel space floating point processing is necessary.

**Note the version field. This file format may change as the architecture evolves. The version reflects changes to the file format.**

CATEGORY | NAME | TYPE | DESCRIPTION | MANDATORY
---------|------|------|-------------|----------
Per-AFU  | version | Integer | Version of the metadata format. | Yes
Per-AFU  | magic-no (to be deprecated)| Integer | Magic no. Associated with the FPGA Interface Manager. | No
Per-AFU  | platform-name | String | Name of the platform for which the metadata is intended. The field value is “DCP” for Intel® Acceleration Stack for FPGAs. | No
Per-AFU  | interface-uuid | UUID | Interface id associated with the FPGA Interface Manager. | Yes
Per-AFU  | power | Integer | Accelerator Function power consumption, in watts. Set to 0 for Intel® Acceleration Stack for FPGAs platforms. | Yes
Per-AFU  | clock-frequency-low | Float | Clock frequency for 1st PLL (Clock network)1 in MHz. | No
Per-AFU  | clock-frequency-high | Float | Clock frequency for 2nd PLL (0 if absent) in MHz. | No
Per-AFC Cluster | total-contexts | Integer | Number of AFCs in this cluster. Always be 1 in current architectures. | Yes
Per-AFC Cluster | afc-type-uuid |  UUID | AFC type = AFU ID in current architectures. | Yes
Per-AFC Cluster | name | string | AFC name = AFU name in current architectures. | Yes
