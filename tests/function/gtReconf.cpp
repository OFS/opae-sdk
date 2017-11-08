/*++

INTEL CONFIDENTIAL
Copyright 2016 - 2017 Intel Corporation

The source code contained or described  herein and all documents related to
the  source  code  ("Material")  are  owned  by  Intel  Corporation  or its
suppliers  or  licensors.  Title   to  the  Material   remains  with  Intel
Corporation or  its suppliers  and licensors.  The Material  contains trade
secrets  and  proprietary  and  confidential  information  of Intel  or its
suppliers and licensors.  The Material is protected  by worldwide copyright
and trade secret laws and treaty provisions. No part of the Material may be
used,   copied,   reproduced,   modified,   published,   uploaded,  posted,
transmitted,  distributed, or  disclosed in  any way  without Intel's prior
express written permission.

No license under any patent, copyright,  trade secret or other intellectual
property  right  is  granted to  or conferred  upon  you by  disclosure  or
delivery of the  Materials, either  expressly, by  implication, inducement,
estoppel or otherwise. Any license  under such intellectual property rights
must be express and approved by Intel in writing.

--*/

#include <opae/access.h>
#include <opae/manage.h>
#include <fstream>
#include <iostream>

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

#define NLB_MODE0_GBS \
  "/home/lab/gbs/121516_skxp_630_pr_hssiE40_7277_sd00_skxnlb400m0.gbs"
#define NLB_MODE0_GBS_INVALID "/home/lab/gbs/skxp_630_pr_invalid.gbs"
#define NLB_MODE0_GBS_CRC_CORRUPT \
  "/home/lab/gbs/121516_skxp_630_pr_hssiE40_7277_sd00_skxnlb400m0_corrupt.gbs"

#define NLB_MODE0_META_GBS \
  "/home/lab/gbs/"         \
  "121516_skxp_630_pr_hssiE40_7277_sd00_skxnlb400m0_valid_meta.gbs"
#define NLB_MODE0_NO_META_GBS \
  "/home/lab/gbs/"            \
  "121516_skxp_630_pr_hssiE40_7277_sd00_skxnlb400m0_empty_meta.gbs"
#define NLB_MODE0_CLK_META_GBS \
  "/home/lab/gbs/"             \
  "121516_skxp_630_pr_hssiE40_7277_sd00_skxnlb400m0_clk_meta.gbs"
#define NLB_MODE0_INVALID_META_GBS \
  "/home/lab/gbs/"                 \
  "121516_skxp_630_pr_hssiE40_7277_sd00_skxnlb400m0_invalid_meta.gbs"

using namespace std;
using namespace common_test;

class LibopaecPRFCommonHW : public BaseFixture, public ::testing::Test {
 public:
  void SetUp(const std::string& filename) {
    // bitstream file path
    std::string gbsfilename(filename);

    std::ifstream gbsfile(gbsfilename.c_str(), std::ios::binary);
    EXPECT_TRUE(gbsfile.good());

    gbsfile.seekg(0, std::ios::end);
    bitstream_len = gbsfile.tellg();
    EXPECT_NE(bitstream_len, 0);

    gbsfile.seekg(0, std::ios::beg);
    bitstream = (uint8_t*)calloc(bitstream_len, sizeof(char));
    EXPECT_TRUE(bitstream != NULL);

    gbsfile.read((char*)bitstream, bitstream_len);
    gbsfile.close();
  }

  virtual void TearDown() {
    if (NULL != bitstream) {
      free(bitstream);
    }
  }

  uint8_t* bitstream;
  size_t bitstream_len;
};

/**
  * @test       PR_drv_01
  *
  * @brief      When the parameters are valid and the drivers are
  *             loaded, fpgaReconfigureSlot reconfigure's slot with new
  *             bitstream.
  *
  */
TEST_F(LibopaecPRFCommonHW, PR_drv_01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint32_t slot = 0;

  SetUp(NLB_MODE0_GBS);

  // open FME device
  token_for_fme0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Reconfigure gbs
  EXPECT_EQ(FPGA_OK, fpgaReconfigureSlot(h, slot, bitstream, bitstream_len, 0));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
  * @test       PR_drv_02
  *
  * @brief      When the parameters are invalid and the drivers are
  *             loaded, fpgaReconfigureSlot return error.
  *
  */
TEST_F(LibopaecPRFCommonHW, PR_drv_02) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint32_t slot = 0;
  int fddev = -1;

  SetUp(NLB_MODE0_GBS);

  token_for_fme0(&_tok);

  // NULL Driver hnadle
  EXPECT_NE(FPGA_OK,
            fpgaReconfigureSlot(NULL, slot, bitstream, bitstream_len, 0));

  // Invalid Magic Number
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  struct _fpga_handle* _handle = (struct _fpga_handle*)h;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, fpgaReconfigureSlot(h, slot, bitstream, bitstream_len, 0));

  _handle->magic = FPGA_HANDLE_MAGIC;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // Invalid Driver handle
  EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
  _handle = (struct _fpga_handle*)h;

  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, fpgaReconfigureSlot(h, slot, bitstream, bitstream_len, 0));

  _handle->fddev = fddev;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // open FME device
  EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // NULL bitstream pointer
  EXPECT_NE(FPGA_OK, fpgaReconfigureSlot(h, slot, NULL, bitstream_len, 0));

  // invalid bitstream size
  EXPECT_NE(FPGA_OK, fpgaReconfigureSlot(h, slot, bitstream, 0, 0));

  // invalid bitstream slot
  EXPECT_NE(FPGA_OK, fpgaReconfigureSlot(h, 44, bitstream, bitstream_len, 0));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
  * @test       PR_drv_05
  *
  * @brief      When the bitstream has valid GBS header and JSON
  *             metadata fpgaReconfigureSlot reconfigures slot with new
  *             bitstream.
  *
  */
TEST_F(LibopaecPRFCommonHW, PR_drv_05) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint32_t slot = 0;

  SetUp(NLB_MODE0_META_GBS);

  // open FME device
  token_for_fme0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Reconfigure gbs
  EXPECT_EQ(FPGA_OK, fpgaReconfigureSlot(h, slot, bitstream, bitstream_len, 0));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
  * @test       PR_drv_06
  *
  * @brief      When the bitstream has valid GBS header and empty JSON
  *             metadata fpgaReconfigureSlot reconfigures slot with new
  *             bitstream.
  *
  */
TEST_F(LibopaecPRFCommonHW, PR_drv_06) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint32_t slot = 0;

  SetUp(NLB_MODE0_NO_META_GBS);

  // open FME device
  token_for_fme0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Reconfigure gbs
  EXPECT_EQ(FPGA_OK, fpgaReconfigureSlot(h, slot, bitstream, bitstream_len, 0));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
  * @test       PR_drv_07
  *
  * @brief      When the bitstream has valid GBS header and JSON
  *             metadata that has user clock info, fpgaReconfigureSlot
  *             sets the clock and reconfigures slot with new bitstream.
  *
  */
TEST_F(LibopaecPRFCommonHW, PR_drv_07) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint32_t slot = 0;

  SetUp(NLB_MODE0_CLK_META_GBS);

  // open FME device
  token_for_fme0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Reconfigure gbs
  EXPECT_EQ(FPGA_OK, fpgaReconfigureSlot(h, slot, bitstream, bitstream_len, 0));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
  * @test       PR_drv_08
  *
  * @brief      When the bitstream has valid GBS header and invalid JSON
  *             metadata fpgaReconfigureSlot does not reconfigure slot
  *             with new bitstream.
  *
  */
TEST_F(LibopaecPRFCommonHW, PR_drv_08) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint32_t slot = 0;

  SetUp(NLB_MODE0_INVALID_META_GBS);

  // open FME device
  token_for_fme0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Reconfigure gbs
  EXPECT_NE(FPGA_OK, fpgaReconfigureSlot(h, slot, bitstream, bitstream_len, 0));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}
