// Copyright(c) 2020, Intel Corporation
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
#pragma once

namespace dummy_afu {

enum {
  AFU_DFH = 0x0000,
  AFU_ID_L = 0x0008,
  AFU_ID_H = 0x0010,
  NEXT_AFU = 0x0018,
  AFU_DFH_RSVD = 0x0020,
  SCRATCHPAD = 0x0028,
  MMIO_TEST_SCRATCHPAD = 0x1000,
  MEM_TEST_CTRL = 0x2040,
  MEM_TEST_STAT = 0x2048,
  MEM_TEST_SRC_ADDR = 0x2050,
  MEM_TEST_DST_ADDR = 0x2058,
  DDR_TEST_CTRL = 0x3000,
  DDR_TEST_STAT = 0x3008,
  DDR_TEST_BANK0_STAT = 0x3010,
  DDR_TEST_BANK1_STAT = 0x3018,
  DDR_TEST_BANK2_STAT = 0x3020,
  DDR_TEST_BANK3_STAT = 0x3028,
};


union afu_dfh  {
  enum {
    offset = AFU_DFH
  };

  afu_dfh(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t FeatureID : 12;
    uint64_t FeatureRev : 4;
    uint64_t NextDfhByteOffset : 24;
    uint64_t EOL : 1;
    uint64_t Reserved41 : 19;
    uint64_t FeatureType : 4;
  };
};

union mem_test_ctrl  {
  enum {
    offset = MEM_TEST_CTRL
  };

  mem_test_ctrl(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t StartTest : 1;
    uint64_t Reserved1 : 63;
  };
};

union ddr_test_ctrl  {
  enum {
    offset = DDR_TEST_CTRL
  };

  ddr_test_ctrl(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t DDRBank0StartTest : 1;
    uint64_t DDRBank1StartTest : 1;
    uint64_t DDRBank2StartTest : 1;
    uint64_t DDRBank3StartTest : 1;
    uint64_t Reserved4 : 60;
  };
};

union ddr_test_stat  {
  enum {
    offset = DDR_TEST_STAT
  };

  ddr_test_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t NumDDRBank : 8;
    uint64_t Reserved8 : 56;
  };
};

union ddr_test_bank0_stat  {
  enum {
    offset = DDR_TEST_BANK0_STAT
  };

  ddr_test_bank0_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t TrafficGenTestPass : 1;
    uint64_t TrafficGenTestFail : 1;
    uint64_t TrafficGenTestTimeout : 1;
    uint64_t TrafficGenFSMState : 4;
    uint64_t Reserved4 : 57;
  };
};

union ddr_test_bank1_stat  {
  enum {
    offset = DDR_TEST_BANK1_STAT
  };

  ddr_test_bank1_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t TrafficGenTestPass : 1;
    uint64_t TrafficGenTestFail : 1;
    uint64_t TrafficGenTestTimeout : 1;
    uint64_t TrafficGenFSMState : 4;
    uint64_t Reserved4 : 57;
  };
};

union ddr_test_bank2_stat  {
  enum {
    offset = DDR_TEST_BANK2_STAT
  };

  ddr_test_bank2_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t TrafficGenTestPass : 1;
    uint64_t TrafficGenTestFail : 1;
    uint64_t TrafficGenTestTimeout : 1;
    uint64_t TrafficGenFSMState : 4;
    uint64_t Reserved4 : 57;
  };
};

union ddr_test_bank3_stat  {
  enum {
    offset = DDR_TEST_BANK3_STAT
  };

  ddr_test_bank3_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t TrafficGenTestPass : 1;
    uint64_t TrafficGenTestFail : 1;
    uint64_t TrafficGenTestTimeout : 1;
    uint64_t TrafficGenFSMState : 4;
    uint64_t Reserved4 : 57;
  };
};

} // end of namespace dummy_afu
