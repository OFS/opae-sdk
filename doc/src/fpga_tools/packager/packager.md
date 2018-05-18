# packager #

## SYNOPSIS ##

`packager <cmd> [arguments]`

## Description ##
The packager provides tools that Accelerator Functional Unit (AFU) developers use to create Accelerator Function (AF) 
files. The AF file is the programming file for an AFU on Intel&reg; FPGA platforms. The packager tool concatenates
the metadata from the JSON file to a raw binary file `(.rbf)` that the Intel Quartus&reg; Prime software generates. 

The packager's only function is to create an AF file. Refer to [Packager Command Syntax](#packager-command-syntax) for more information
about invoking the packager. The packager depends on a JSON file to describe AFU metadata. Refer to 
[Accelerator Description File](#accelerator-description-file) for more information about the JSON file.

**The packager requires Python 2.7.1 and Python 2.7.3. The tool indicates if it is being called with a compatible 
of Python.**

## Packager Command Syntax ##

The packager is a command line tool with the following syntax:

`$ packager <cmd> [arguments]`

The following table describes the `<CMD>` arguments:

| Command | Arguments       | Description |
|---------| ----------------| ------------|
| ```create-gbs```  | ```--rbf=<RBF_PATH>``` <br>```--afu=<AFU_JSON_PATH>``` <br>```--gbs=<GBS_PATH>``` <br>```--set-value=<key>.<value>```| Creates the AF file. The engineering name for this file is the green bit stream, abbreviated gbs. The `--rbf` and `--afu`   arguments are required.  `<RBF_PATH>` is the path to the RBF file for the AFU. The Quartus&reg; Prime software generates this RBF by compiling the AFU design. `<AFU_JSON_PATH>` is the path to the Accelerator Description file. This is a JSON file that describes the metadata that `create-gbs` appends to the RBF. `<GBS_PATH>` is the path to the RBF file for the FPGA Interface Manager (FIM) that contains the FPGA interface unit and other interfaces. If you do not specify the `--gbs`, the command defaults to `<rbf_name>.gbs`. You can use the optional `--set-value=<key>.<value>` argument to set values for JSON metadata. To set more than one JSON value, list a series of `<key>.<value>`  pairs.|
|```modify-gbs``` | ```--gbs=<gbs_PATH>```| Modifies the AF file. The `--input-gbs`argument is required. If you do not provide the `--output-gbs` argument, `modify-gbs` overwrites the `--input-gbs` file. Use the `--set-value=<key>.<value>` argument to set values for JSON metadata. To set more than one JSON value, list a series of `<key>.<value>`  pairs.|
|```gbs-info``` | ```--input-gbs=<gbs_PATH>```| Prints information about the AF file. The `--input-gbs` argument is required.|
|```get-rbf``` | ```--gbs=<GBS_PATH>``` <br>```--rbf=<RBF_PATH>```| Creates the RBF by extracting it from the AF file. The `--gbs`argument is required. If you do not specify the `--rbf` argument, the command defaults to `<gbs_name.rbf` . |
| None, or any `<CMD>`  | `--help` | Summarizes the `<CMD>` options. Typing `packager --help` gives a list of `<CMD>` values. Typing `packager <CMD> --help` provides detailed help for `<CMD>` | 


## Examples ##

To generate an AF file, run:

`$ packager create-gbs --rbf=<RBF_PATH> --afu=<AFU_JSON_PATH> --gbs=<GBS_PATH>`

**TIP**: JSON files are very particular about syntax such as trailing commas. If you are getting errors, use `jsonlint.com` to
validate that your JSON is formatted correctly. 

To modify metadata in an existing AF, run the following command:

`$ packager modify-gbs --input-gbs=<PATH_TO_GBS_TO_BE_MODIFIED> --outputgbs=<NAME_FOR_NEW_GBS> --set-value <key>:<value>`

You can pass in a number of <key>:<value> pairs with --set-value to update values in an AF. 

To print the metadata of an existing AF: 

`$ packager get-info --gbs=<GBS_PATH>` 

To extract the RBF from the AF:

`$ packager get-rbf --gbs=<GBS_PATH> --rbf=<NAME_FOR_RBF>`

## Accelerator Description File ##

The Accelerator Description File is a JSON file that describes the metadata associated with an AFU.
The Open Progammable Accelerator Engine (OPAE) uses this metadata during reconfiguration. Here is an example file:

```
{
   "version": 1,
   "platform-name": "DCP",
   "afu-image": {
      "magic-no": 488605312,
      "interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
      "power": 0,
      "accelerator-clusters": [{
         "name": "dma_test_afu",
         "total-contexts": 1,   
         "accelerator-type-uuid": "331DB30C-9885-41EA-9081-F88B8F655CAA"
      }
      ]  
   }
}
```
The packager stores these parameter values in the resultant AF. After reprogramming the AFU using partial reconfiguration (PR), the 
software driver reconfigures the PLLs by writing the clock-frequency-high and clock-frequency-low values (if present) over the 
PCIe&reg; and CCI interfaces. 

.. note::
```
The JSON file format may change as the architecture evolves. Any changes to the current format trigger an update
to the version number.  
```

CATEGORY | NAME | TYPE | DESCRIPTION | MANDATORY
---------|------|------|-------------|:----------:|
Per-AFU  | version | Integer | Version of the metadata format. | Yes
Per-AFU  | magic-no (to be deprecated)| Integer | Magic no. Associated with the FPGA Interface Manager. | No
Per-AFU  | platform-name | String | Name of the platform for which the metadata is intended. The field value is “DCP” for Intel  Acceleration Stack for FPGAs. | No
Per-AFU  | interface-uuid | UUID | Interface id associated with the FPGA Interface Manager. | Yes
Per-AFU  | power | Integer | Accelerator Function power consumption, in watts. Set to 0 for Intel Acceleration Stack for FPGAs platforms. | Yes
Per-AFU  | clock-frequency-low | Float | Clock frequency for 1st PLL (Clock network)1 in MHz. | No
Per-AFU  | clock-frequency-high | Float | Clock frequency for 2nd PLL (0 if absent) in MHz. | No
Per-AFC Cluster | total-contexts | Integer | Number of AFCs in this cluster. Always be 1 in current architectures. | Yes
Per-AFC Cluster | afc-type-uuid |  UUID | AFC type = AFU ID in current architectures. | Yes
Per-AFC Cluster | name | string | AFC name = AFU name in current architectures. | Yes

| Date | Intel Acceleration Stack Version | Changes Made |
|:------|----------------------------|:--------------|
|2018.05.21| DCP 1.1 Beta (works with Quartus Prime Pro 17.1.1) |  Fixed typos. |


