# Platform Interface RTL

This tree contains the RTL implementation of the platform interface.  The
standard build flows put the [rtl](rtl) directory on the Verilog include
path.  AFUs should import the code using:

```Verilog
  `include "platform_if.vh"
```

The RTL contains several components:

- Include files for loading the RTL into simulation or synthesis are in
  [sim/platform\_if\_addenda.txt](sim/platform_if_addenda.txt) and
  [par/platform\_if\_addenda.qsf](par/platform_if_addenda.qsf).

- Interfaces to devices such as CCI-P and local memory are stored in
  [rtl/device\_if](rtl/device_if).

- Platform-specific configuration data structures, populated by
  [platform\_db](../platform_db), are in [rtl/device\_cfg](rtl/device_cfg).

- Clock crossing and buffering shims are in
  [rtl/platform\_shims](rtl/platform_shims).  The shims are internal to
  the platform manager.  The interfaces may change from release to release.
