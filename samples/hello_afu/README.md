# Simple "Hello World" AFU example

This example is nearly the simplest possible accelerator. The RTL receives an
address via a memory mapped I/O (MMIO) write and generates a CCI write to the
memory line at that address, containing the string "Hello World!". The
software spins, waiting for the line to update. Once available, the software
prints the string.

Of note in this example:

- RTL sources are specified in [hw/sources.txt](hw/sources.txt).  In addition to
  SystemVerilog, the AFU's JSON file is included also under 'hw' directory.  The AFU JSON
  describes the interfaces required by the AFU.  It also holds a UUID to
  identify the AFU when it is loaded on an FPGA.

- The AFU declares that it expects the ccip_std_afu top-level interface by
  setting "afu-top-interface" to "ccip_std_afu" in [hw/hello_afu.json](hw/hello_afu.json).
  This is the base CCI-P interface with clocks, reset and CCI-P Rx/Tx
  structures.  Other interface options will be described in subsequent
  examples.

- The AFU UUID is declared exactly once in the sources: in the JSON file.
  [hw/hello_afu.json](hw/hello_afu.json) and is extracted by
  the OPAE `afu_json_mgr` script into both Verilog (`afu_json_info.vh`) and C
  `afu_json_info.h` header files. The RTL loads the UUID from
  `afu_json_info.vh`.  Similarly, software compilation loads the
  UUID from `afu_json_info.h`.

- The software demonstrates the minimum necessary to attach to an FPGA
  using OPAE.  The RTL demonstrates the minimum necessary to satisfy the
  OPAE driver and the hello_afu software.

## AFU RTL code

The user-customizable RTL is contained entirely in
[hw/rtl/afu.sv](hw/rtl/afu.sv) and demonstrates the
following universal AFU requirements:

- The CCI request and response ports are clocked by pClk.

- Reset (pck_cp2af_softReset) is synchronous with pClk.

- Outgoing request and incoming response wires must be registered.

- All AFUs must implement a read-only device feature header (DFH) in MMIO
  space at MMIO address 0. The DFH holds a 128 bit AFU ID, mapped to a pair of
  64 bit MMIO "registers".

## AFU software code

The software side is contained entirely in [sw/hello_afu.c](sw/hello_afu.c):

- The AFU ID in the software must match the AFU ID in the hardware's DFH.

- The FPGA-accessible shared memory is mapped explicitly.

- Memory addresses passed to the FIU's CCI request wires are in a
  physical I/O address space. Since all CCI requests refer to entire 512
  bit memory lines, the example passes the line-based physical address
  to which "Hello World!" should be written.

- The code in `connect_to_accel()` is a simplification of the ideal
  sequence. The code detects at most one accelerator matching the
  desired UUID.  Later examples detect when multiple instances of the
  same hardware are available in case one is already in use.

## Simulation with ASE

  Ensure your `$QUARTUS_HOME` and `$MTI_HOME` user environment variables point
  to the roots of valid installations of Quartus (17.0 or higher) and Modelsim
  (10.3 or higher). While these steps are also listed in ../README.md, we also list them here:

```console
    $ mkdir opae_build
    $ cd opae_build
    $ cmake -DOPAE_BUILD_ASE_SAMPLES=on <Directory where OPAE repository was cloned.>
    $ make -j
```

  Simulation requires two software processes: one for RTL simulation and
  the other to run the connected software.

  `make -j` will build both the required files for the RTL simulation and the
  corresponding  connected softwre. If `make -j` failes, confirm that your
  Python version is at least 2.7 and that the jsonschema Python package is installed
  (https://pypi.python.org/pypi/jsonschema).

  To launch the RTL simulation execute following steps inside `opae_build` directory:

```console
    $ ./samples/hello_afu/hw/ase_server.sh
```

  This will run the RTL simulator.  If this step fails it is
  likely that your RTL simulator is not installed properly.

  The simulator prints a message that it is ready for simulation.  It also
  prints a message to set the `ASE_WORKDIR` environment variable.  Open
  another shell and cd to the `opae_build` directory.  To run the software:

```console
    $ <Set ASE_WORKDIR as directed by the simulator>
    $ LD_PRELOAD=./lib/libopae-c-ase.so ./bin/hello_afu
```

  The software and simulator should both run quickly, log transactions and
  exit.

## Synthesis with Quartus #

  RTL simulation and synthesis are driven by the same `sources.txt`, `hello_afu.json` and
  underlying OPAE scripts.  To construct a Quartus synthesis environment
  for this AFU, enter following commands in same directory containing this
  `README` file:

```console
    $ afu_synth_setup --source hw/sources.txt build_synth
    $ cd build_synth
    $ ${OPAE_PLATFORM_ROOT}/bin/run.sh
```

  `run.sh` will invoke Quartus, which must be properly installed.  The end
  result will be a file named `hello_afu.gbs` in the build_synth directory.
  This GBS file may be loaded onto a compatible FPGA using OPAE's `fpgaconf`
  tool.

## Running the sample on hardware

To run the software connected to an FPGA, compile it as above, but invoke the
 binary `hello_afu` binary without specifying
 `LD_PRELOAD=./lib/libopae-c-ase.so`.  If you have already run ASE simulation,
 the binary has already been compiled and `make -j` will do nothing:

```console
    # Load the AFU into the partial reconfiguration region
    $ sudo opae_build/bin/fpgaconf hello_afu.gbs

    $ cd opae_build
    # sudo may be required to invoke hello_afu, depending on your environment.
    $ ./bin/hello_afu
```
