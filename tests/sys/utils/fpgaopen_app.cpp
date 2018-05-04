// Copyright(c) 2017-2018, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <getopt.h>
#include <opae/access.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "common_utils.h"
#include "common_sys.h"

using namespace std;
using namespace common_utils;

void show_help() {
  printf(
      "Usage: fpgaopen_app\n"
      "\t-s, Call fpgaOpen with the FPGA_OPEN_SHARED flag.\n"
      "\t-b, Filter on bus number provided.\n");
}

fpga_result retval = FPGA_INVALID_PARAM;

int main(int argc, char** argv) {
  printf("foapp:  =================================\n");

  signed opt = 0;
  // fpga_open_flags shared;
  int shared = 0;
  uint8_t ibus = 0;

  while ((opt = getopt(argc, argv, "hsb:")) != -1) {
    switch (opt) {
      case 'h':
        show_help();
        break;

      case 's':
        shared = FPGA_OPEN_SHARED;
        break;

      case 'b':
        ibus = strtol(optarg, NULL, 16);
        break;

      default:
        return -1;
    }
  }

  BaseFixture* pbf = new BaseFixture();

  auto functor = [=]() -> void {

    fpga_handle h = NULL;
    fpga_properties props = NULL;
    uint8_t obus = 0;

    checkReturnCodes(fpgaGetProperties(pbf->tokens[pbf->index], &props),
                     LINE(__LINE__));

    checkReturnCodes(fpgaPropertiesGetBus(props, &obus), LINE(__LINE__));

      checkReturnCodes(retval = fpgaOpen(pbf->tokens[pbf->index], &h, shared),
                       LINE(__LINE__));
      if (FPGA_OK == retval) {
        printf("foapp:  opened 0x%x\tSHARED:  %d\n", obus, shared);
        checkReturnCodes(retval = fpgaClose(h), LINE(__LINE__));
      } else {
        printf("foapp:  open failed on 0x%x\n", obus);
        exit(EXIT_FAILURE);
      }
  };

  GlobalOptions::Instance().Bus(ibus);

  pbf->TestAllFPGA(FPGA_ACCELERATOR,  // object type
                   false,             // reconfig default NLB0
                   functor);          // test code
  printf("foapp:  =================================\n");

  return retval;
}
