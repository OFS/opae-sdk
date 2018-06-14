dma_afu is a starting point for building an AFU that requires buffers to be
transferred between host memory and FPGA-attached local memory.

The dma_afu sample may be configured and compiled with Quartus using the
afu_synth_setup script described in ../hello_afu/README.  However, because
dma_afu ships with Qsys sources that have not been translated into Verilog, an
extra step is required for simulation with ASE.  A script is provided here that
invokes the usual afu_sim_setup script and also invokes Qsys to translate the
Qsys sources to Verilog:

  $ ${OPAE_PLATFORM_ROOT}/hw/common/scripts/setup_sim.sh -a <path to>/samples/dma_afu -r build_ase_dir

Where:

  ${OPAE_PLATFORM_ROOT} points to the root of a platform release tree.

  -a (AFU) is the directory containing this README.

  -r is the target build directory.


The resulting build_ase_dir is equivalent to the directories constructed by
afu_sim_setup.  The setup_sim.sh script works with all of the samples.
