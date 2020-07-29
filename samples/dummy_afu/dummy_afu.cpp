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
  app.register_command<mmio_test>();
  app.register_command<dma_test>();
  app.register_command<lpbk_test>();
  return app.main(argc, argv);
}