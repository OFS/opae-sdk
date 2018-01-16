# Platform Interface RTL

This tree contains the RTL implementation of the platform interface.  The
standard build flows put the [rtl](rtl) directory on the Verilog include
path.  AFUs should import the code using:

```Verilog
  `include "platform_if.vh"
```
