# AFU Top-Level Interface Database

The AFU top-level interface databases describe the top-level module name and
ports expected by an AFU.

Instead of starting with the database fields below, reading a few databases
in this directory may be a better introduction.  [ccip\_std\_afu.json](ccip_std_afu.json)
describes the original, standard CCI-P interface. The interface has been broken down into
several classes of signals, such as clocks and CCI-P request/response structures.
[ccip\_std\_afu\_avalon\_mm.json](ccip_std_afu_avalon_mm.json) describes a platform
that includes CCI-P plus local memory. The JSON file imports CCI-P using the *parent*
entry, pointing to [ccip\_std\_afu.json](ccip_std_afu.json). Local memory is described
as a vector of SystemVerilog Avalon memory interfaces, one instance per memory bank.

Each JSON database is a dictionary, supporting the following primary keys:

- **version**: Integer [required]

  Indicates the JSON database version.  Currently only version 1 is supported.

- **description**: String [optional]

  One line description.

- **comment**: String or list of strings [optional]

  JSON doesn't have a comment escape character.  In order to embed comments,
  the AFU top-level database ignores JSON dictionary entries named "comment".
  Comments may be a list of strings in order to write multi-line comments.

- **module-name**: String [required]

  The name of the AFU's top-level module.  The platform will instantiate a module
  with this name.

- **platform-shim-module-name**: String [optional]

  The name of a shim that will be instantiated automatically between the platform
  and the AFU's top-level module (module-name).  When present, platform shims
  offer a standard set of transformations that are common to many AFUs.  Platform
  shims may automatically instantiate clock-crossing FIFOs.  The shims may
  also insert registers to relax timing.  Clock management and register
  insertion are controlled by parameters in an AFU's JSON file.  See the clock
  module-ports parameter, described in [README\_AFU](README_AFU.md) in this
  directory.  When not defined or NULL, no shim is instantiated.

- **parent**: String [optional]

  The name of a parent AFU top-level interface JSON database.  The parent will
  be loaded and fields from the child will be merged into the parent.  For most
  fields, the merge overwrites any existing parent data.  For module-ports,
  the merge completely overwrites entries with matching classes.  Keys within
  a module-ports class aren't merged.  The parent class is removed and
  replaced with the child's module-ports class entry.

- **module-ports**: List [required]

  The set of ports expected by the AFU top-level module (module-name).
  Each entry in the module-ports list is a dictionary which describes
  a port or group of ports to the AFU top-level module.
  The format of a module-ports dictionary entry is described below.


## Module Ports

There may be at most one instance of a given class in a module-ports
list.  For example, only one type of local-memory may be requested.

Each of the module-ports is a dictionary with the following keys:

- **class**: String [required]

  The major interface class, such as clocks, cci-p and local-memory.
  Supported classes are listed below.

- **interface**: String [required]

  The interface type expected by the AFU for the given class.

- **optional**: Boolean [optional (defaults to false)]

  If true, the AFU will accept platforms that do not offer the
  class and interface.  The Verilog preprocessor variable
  AFU\_TOP\_REQUIRES\_<class>\_<interface> will be defined in the
  compilation when the interface is present.  The variable
  PLATFORM\_PROVIDES\_<class> will also be defined.  Note: class
  and interface are converted to upper case and dashes become
  underscores.

- **version**: Integer [optional (defaults to 1)]

  Version allows for variations in either the JSON database or in the
  actual interface.  The significance of version numbers is specific
  to a given class and interface.

- **vector**: Boolean [optional (defaults to false)]

  When true, the module port is a vector of multiple instances
  of the port.  For example, local-memory banks may be passed
  as a vector of memory interfaces.  The minimum and maximum number
  of entries is specified using min-entries and max-entries.

- **min-entries**: Integer [optional (defaults to 1), must be > 0]

  For vector ports, the minimum number of instances required by
  the AFU.

- **max-entries**: Integer [optional (defaults to infinite), must be > min-entries]

  For vector ports, the maximum number of instances accepted by
  the AFU.

- **default-entries**: Integer [optional, no default]

  When both the platform and the AFU are willing to accept a range
  of vector lengths, default-entries are instantiated.  When both
  the AFU and the platform specify default-entries, the AFU takes
  precedence.

- **define**: List of strings [optional, no default]

  When the module port is present, add these strings as
  Verilog preprocessor values to the platform configuration.

---------------------------------------------------------------------------

## Supported Class/Interface Types

### Class *clocks*

- pClk3_usr2:
```Verilog
    input  logic        pClk,                 // Primary CCI-P interface clock.
    input  logic        pClkDiv2,             // Aligned, pClk divided by 2.
    input  logic        pClkDiv4,             // Aligned, pClk divided by 4.
    input  logic        uClk_usr,             // User clock domain. Refer to clock programming guide.
    input  logic        uClk_usrDiv2,         // Aligned, user clock divided by 2.
    input  logic        pck_cp2af_softReset,  // CCI-P ACTIVE HIGH Soft Reset
```

---------------------------------------------------------------------------

### Class *power*

- 2bit:
```Verilog
    input  logic [1:0]  pck_cp2af_pwrState,   // CCI-P AFU Power State
```

---------------------------------------------------------------------------

### Class *error*

- 1bit:
```Verilog
    input  logic        pck_cp2af_error,      // CCI-P Protocol Error Detected
```

---------------------------------------------------------------------------

### Class *cci-p*

- struct:
```Verilog
    input  t_if_ccip_Rx pck_cp2af_sRx,        // CCI-P Rx Port
    output t_if_ccip_Tx pck_af2cp_sTx         // CCI-P Tx Port
```

---------------------------------------------------------------------------

### Class *local-memory*

- avalon_mm:
```Verilog
    // Vector of Avalon memory interfaces, one per bank.
    // The module parameter NUM_LOCAL_MEM_BANKS is expected.
    avalon_mem_if.to_fiu local_mem[NUM_LOCAL_MEM_BANKS]
```

- avalon_mm_legacy_wires_2bank [DEPRECATED]:
```Verilog
    // This interface was used in a very early discrete card memory
    // interface.  It will be deprecated and should not be used
    // in new AFUs.

    // The preprocessor variable DDR_ADDR_WIDTH is expected.
    // The preprocessor variable INCLUDE_DDR4 is defined automatically
    // when the memory is present.

    // bank A
    input   logic                       DDR4a_USERCLK,    // EMIF shared clock
    input   logic                       DDR4a_waitrequest,
    input   logic [511:0]               DDR4a_readdata,
    input   logic                       DDR4a_readdatavalid,
    output  logic [6:0]                 DDR4a_burstcount,
    output  logic [511:0]               DDR4a_writedata,
    output  logic [DDR_ADDR_WIDTH-1:0]  DDR4a_address,
    output  logic                       DDR4a_write,
    output  logic                       DDR4a_read,
    output  logic [63:0]                DDR4a_byteenable,

    // bank B
    input   logic                       DDR4b_USERCLK,
    input   logic                       DDR4b_waitrequest,
    input   logic [511:0]               DDR4b_readdata,
    input   logic                       DDR4b_readdatavalid,
    output  logic [6:0]                 DDR4b_burstcount,
    output  logic [511:0]               DDR4b_writedata,
    output  logic [DDR_ADDR_WIDTH-1:0]  DDR4b_address,
    output  logic                       DDR4b_write,
    output  logic                       DDR4b_read,
    output  logic [63:0]                DDR4b_byteenable,
```
