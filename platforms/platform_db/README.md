# Platform Description Databases

The platform databases serve two purposes:

1. To describe the classes of wired interfaces to local devices offered by
   particular physical platforms.

2. To describe parameterizable properties of the local device interfaces.
   Some interfaces may maintain a consistent wired interface but vary their
   characteristics across platforms. For example, the set of channels offered
   by CCI-P may vary from platform to platform depending on the physical
   interconnect.

## Platform Wired Interfaces

Each JSON database is a dictionary, supporting the following keys:

- **version**: Integer [required]

  Indicates the JSON database version.  Currently only version 1 is supported.

- **platform-name**: String [required]

  The name of the platform.  Typically, this will match the name of
  the JSON file.

- **parent**: String [optional]

  The name of a parent platform JSON database.  The parent will be loaded
  and fields from the child will be merged into the parent.  For most fields,
  the merge overwrites any existing parent data.  For module-ports-offered,
  the merge completely overwrites entries with matching class/interface
  pairs.  Keys within a module-ports-offered class/interface entry aren't
  merged.  The parent's entry is removed and replaced with the child's
  entry.

- **ase-platform**: String [required]

  The name of the ASE platform that simulates the physical platform.

- **module-ports-offered**: List [required]

  The set of ports offered by the platform to AFUs.  Entries in the
  list have the same format as the module-ports list found in [AFU
  top-level interface databases](../afu_top_ifc_db).

  module-ports-offered differs from the AFU's module-ports in
  the following ways:

  - For vector classes, max-entries must be defined.

  - There may be multiple instances of the same class.  In this case,
    all instances must be optional.  At most one instance of the
    class will be instantiated, since the AFU is allowed to request
    only one instance of a class.

  - Some classes require configuration parameters.  See the *Platform Parameters*
    section below.  Individual platforms may override a parameter's default
    by setting updated values in a "params" dictionary within
    module-ports-offered classes.  Setting a parameter's value to null
    will keep it from being defined.

    See [../afu\_top\_ifc\_db/](../afu_top_ifc_db/) for both
    the schema and a list of valid class/interface pairs.

## Platform Parameters

[platform\_defaults.json](platform_defaults.json) is a special database, holding
default values for module port classes that are configurable.  By convention,
all configuration parameters available for each class should be set in
[platform\_defaults.json](platform_defaults.json) in order to have a central
repository of parameters.  Particular platforms can override these default values
by specifying platform-specific values in a "params" dictionary within
an offered class.  For example, a PCIe-only platform can indicate the
set of supported VCs with:

```json
{
   "description": "PCIe-only platform",
   "module-ports-offered" :
      [
         {
            "class": "cci-p",
            "interface": "struct",
            "params":
               {
                  "vc-supported": "{ 1, 0, 1, 0 }"
               }
         }
      ]
}
```

Like other configuration databases, platform parameters are consumed by
the [afu\_platform\_config](../scripts/afu_platform_config) script.
The script emits parameter values as preprocessor variables of the form
PLATFORM\_PARAM\_\<class\>\_\<param\>.  Consistent with the other
variables, all characters are converted to upper case and dashes become
underscores.  These preprocessor variables are further exported automatically
to SystemVerilog data structures in
[../platform\_if/rtl/device\_cfg](../platform_if/rtl/device_cfg).
CCI-P descriptors are in [ccip\_cfg\_pkg.sv](../platform_if/rtl/device_cfg/ccip_cfg_pkg.sv)
and local memory in [local\_mem\_cfg\_pkg.sv](../platform_if/rtl/device_cfg/local_mem_cfg_pkg.sv).
The SystemVerilog configuration should be incorporated into AFUs using the
standard mechanism:

```Verilog
  `include "platform_if.vh"
```

The platform database is tagged with a version.  This version could be
used in the canonicalizePlatformDefaultsDb function within
[afu\_platform\_config](../scripts/afu_platform_config) to maintain
compatibility with old databases as interface classes change over time.
