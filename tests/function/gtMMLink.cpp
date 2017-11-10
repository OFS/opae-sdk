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

#include <opae/fpga.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

#define MM_DEBUG_LINK_SIGNATURE 0x170
#define MM_DEBUG_LINK_VERSION 0x174
#define EXPECT_SIGNATURE 0x53797343
#define MAX_SUPPORTED_VERSION 1
#define FPGA_PORT_INDEX_STP 1

using namespace common_test;

/**
 * @test       01
 *
 * @brief      Obtain the signal tap resource of a port and verify the
 *             signature CSR built into the Signal Tap hardware. This
 *             proves access to the HW and mapping to user space of the
 *             correct location.
 */
TEST(LibopaecMMLinkCommonHW, 01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t* mmio_ptr = NULL;

  uint32_t signature, version;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
  ASSERT_EQ(FPGA_OK, fpgaMapMMIO(h, FPGA_PORT_INDEX_STP, &mmio_ptr));
  EXPECT_FALSE(mmio_ptr == NULL);

  ASSERT_EQ(FPGA_OK, fpgaReadMMIO32(h, FPGA_PORT_INDEX_STP,
                                    MM_DEBUG_LINK_SIGNATURE, &signature));
  ASSERT_EQ(signature, EXPECT_SIGNATURE);

  ASSERT_EQ(FPGA_OK, fpgaReadMMIO32(h, FPGA_PORT_INDEX_STP,
                                    MM_DEBUG_LINK_VERSION, &version));
  ASSERT_EQ(version, MAX_SUPPORTED_VERSION);

  // Unmap memory range otherwise, will not accept open from same process
  EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, FPGA_PORT_INDEX_STP));
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}
