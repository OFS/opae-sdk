NLB implements a loopback test, useful for testing that an FPGA works and for
measuring I/O bandwidth.  The nlb0 and nlb3 programs that drive it ship in the
OPAE SDK.

NLB takes advantage of a few Platform Interface Manager features not used in
other samples:

- Local memory is declared optional in the AFU JSON file (hw/rtl/nlb_400.json).
  Instead of failing on platforms without local memory, afu_platform_config
  constructs a top-level module interface without local memory.  The Verilog
  preprocessor macro PLATFORM_PROVIDES_LOCAL_MEMORY is defined only if local
  memory is available.  The NLB AFU source adjusts based on this macro,
  instantiating a local memory test only when possible.

  The default value of "optional" is false.

- There are multiple variants of NLB.  Mode 0 builds a loopback and mode 3
  tests are unidirectional.  A third variant adds SignalTap.  Three primary
  source file lists are defined: hw/rtl/filelist_mode_0.txt,
  hw/rtl/filelist_mode_3.txt and hw/rtl/filelist_mode_0_stp.txt.  The files
  define an AFU-specific macro to configure the mode and then include
  hw/rtl/filelist_base.txt, which holds the shared list of sources.  The
  desired variant is chosen by naming the corresponding file list, e.g.:

    $ afu_sim_setup --source hw/rtl/filelist_mode_0.txt sim_mode_0

  File lists are parsed by rtl_src_config.  For a description of the syntax
  parsed by rtl_src_config use the --help option:

    $ rtl_src_config --help
