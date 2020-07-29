#include <iostream>
#include <opae/cxx/core/handle.h>
#include <CLI/CLI.hpp>

#include "mmio.h"
#include "lpbk.h"
#include "dma.h"

#include "test_afu.h"

const char *AFU_ID = "91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73";
using namespace opae::app;

int main(int argc, char* argv[])
{
  test_afu app("dummy_afu", AFU_ID);
  auto mmio_cmd = app.register_command(run_mmio, "mmio", "run mmio tests");
  mmio_cmd->add_option("-c,--count", "number of repititions")->default_val(1);
  auto op = mmio_cmd->add_option("-s,--scratchpad-index", "index in the scratchpad array");
  op->check(CLI::Range(0,63));
  app.register_command(run_lpbk, "lpbk", "run loopback tests");
  app.register_command(run_dma, "dma", "run dma tests");
  return app.main(argc, argv);
}