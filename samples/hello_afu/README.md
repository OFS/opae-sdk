# Simple AFU example

A simple AFU template that demonstrates the primary CCI-P interface.  The
RTL satisfies the bare miminum requirements of an AFU, responding to MMIO
reads to return the device feature header and the AFU's UUID.

Of note in this example:

- RTL sources are specified in hw/sources.txt.  In addition to
  SystemVerilog, the AFU's JSON file is included also under 'hw' directory.  The AFU JSON
  describes the interfaces required by the AFU.  It also holds a UUID to
  identify the AFU when it is loaded on an FPGA.

- The AFU declares that it expects the ccip_std_afu top-level interface by
  setting "afu-top-interface" to "ccip_std_afu" in hw/hello_afu.json.
  This is the base CCI-P interface with clocks, reset and CCI-P Rx/Tx
  structures.  Other interface options will be described in subsequent
  examples.

- The AFU UUID is declared exactly once in the sources: in the JSON file.
  The RTL loads the UUID from afu_json_info.vh, which is automatically generated
  from `hello_afu.json` using `afu_json_mgr`. Similarly, software compilation loads the
  UUID from `afu_json_info.h`, which is again automatically generated from `hello_afu.json`.

- The software demonstrates the minimum necessary to attach to an FPGA
  using OPAE.  The RTL demonstrates the minimum necessary to satisfy the
  OPAE driver and the hello_afu software.

## Simulation with ASE

  Follow the steps in ../README.md for building ASE libraries using default OPAE libraries.
  Ensure your `$QUARTUS_HOME` and `$MTI_HOME` user environment variables point
  to the roots of valid installations of Quartus (17.0 or higher) and Modelsim
  (10.3 or higher). While these steps are also listed in ../README.md, we also list them here:

```console
    $ mkdir opae_build
    $ cd opae_build
    $ cmake -DBUILD_ASE_SAMPLES=on <Directory where OPAE repository was cloned.>
    $ make -j
```

  Simulation requires two software processes: one for RTL simulation and
  the other to run the connected software.

  `make` will build both the required files for the RTL simulation and the corresponding connected softwre.
  If `make` failes, confirm that your Python version is at least 2.7 and
  that the jsonschema Python package is installed
  (https://pypi.python.org/pypi/jsonschema).

  To launch the RTL simulation execute following steps inside `opae_build` directory:

```console
    $ ./samples/hello_afu/hw/ase_server.sh
```

  This will run the RTL simulator.  If this step fails it is
  likely that your RTL simulator is not installed properly.  ModelSim,
  Questa and VCS are supported.

  The simulator prints a message that it is ready for simulation.  It also
  prints a message to set the `ASE_WORKDIR` environment variable.  Open
  another shell and cd to the `opae_build` directory.  To run the software:

```console
    $ <Set ASE_WORKDIR as directed by the simulator>
    $ LD_PRELOAD=./lib/libopae-c-ase.so ./hello_afu
```

  The software and simulator should both run quickly, log transactions and
  exit.

## Synthesis with Quartus #

  RTL simulation and synthesis are driven by the same `sources.txt`, `hello_afu.json` and
  underlying OPAE scripts.  To construct a Quartus synthesis environment
  for this AFU, enter following commands in same directory containing this README:

```console
    $ afu_synth_setup --source hw/sources.txt --json hw/hello_afu.json build_synth
    $ cd build_synth
    $ ${OPAE_PLATFORM_ROOT}/bin/run.sh
```

  `run.sh` will invoke Quartus, which must be properly installed.  The end
  result will be a file named `hello_afu.gbs` in the build_synth directory.
  This GBS file may be loaded onto a compatible FPGA using OPAE's `fpgaconf`
  tool.

  To run the software connected to an FPGA, run `hello_afu` binary without specifying
  `LD_PRELOAD=./lib/libopae-c-ase.so`
