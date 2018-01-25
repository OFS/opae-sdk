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
#include <opae/buffer.h>
#include <opae/mmio.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include <sys/mman.h>
#include <algorithm>
#include <deque>
#include <random>

#include "gtest/gtest.h"

#ifdef BUILD_ASE
#include "ase/api/src/types_int.h"
#else
#include "types_int.h"
#endif

#include "common_test.h"

using namespace std;
using namespace common_test;

class LibopaecBufFCommonMOCK : public BaseFixture, public ::testing::Test {
 public:
  static const uint64_t len_min = 1024;
  static const uint64_t len_max = 4 * 1024;
  static const uint64_t len_2mb = 2 * 1024 * 1024;
  static const uint64_t len_1gb = 960 * 1024 * 1024;
  static const uint64_t accum_max = 2 * 1024UL * 1024UL * 1024UL;
};

/**
 * @test       NoPrep01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaReleaseBuffer must fail if fpga_buffer was not
 *             prepared.
 *
 */
TEST(LibopaecBufCommonALL, NoPrep01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t wsid = 1;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Release buffer
  EXPECT_NE(FPGA_OK, fpgaReleaseBuffer(h, wsid));
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       InvalidWSID01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaReleaseBuffer must fail if invalid wsid is passed
 *
 */
TEST(LibopaecBufCommonMOCK, InvalidWSID01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  uint64_t* buf_addr;
  uint64_t wsid = 1, wsidOld;

  // Open port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Allocate a buffer
  buf_len = 1024;
  ASSERT_EQ(FPGA_OK,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, 0));

  // Invalidate wsid
  wsidOld = wsid;
  wsid += 1;

  // Release buffer
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReleaseBuffer(h, wsid));
  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsidOld));
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       PrepRel4K01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaPrepareBuffer must allocate a shared memory buffer.
 *             fpgaReleaseBuffer must release a shared memory buffer.
 *
 */
TEST(LibopaecBufCommonMOCK, PrepRel4K01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  uint64_t* buf_addr;
  uint64_t wsid = 1;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Allocate buffer in MB range
  buf_len = 4 * 1024;
  EXPECT_EQ(FPGA_OK,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, 0));

  // Release buffer in MB range
  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       PrepPre0B
 *
 * @brief      When FPGA_BUF_PREALLOCATED is given and the buffer len is
 *             0, fpgaPrepareBuffer fails with FPGA_INVALID_PARAM.
 *
 */
TEST(LibopaecBufCommonALL, PrepPre0B) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  uint64_t* buf_addr = (uint64_t*)1;
  uint64_t wsid = 1;
  int flags;

  // Open port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  buf_len = 0;
  flags = FPGA_BUF_PREALLOCATED;
  EXPECT_EQ(FPGA_INVALID_PARAM,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, flags));
  // Close the device
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       PrepPreNotAligned
 *
 * @brief      When FPGA_BUF_PREALLOCATED is given and the buffer len is
 *             not a multiple of the page size, fpgaPrepareBuffer fails
 *             with FPGA_INVALID_PARAM.
 *
 */
TEST(LibopaecBufCommonALL, PrepPreNotAligned) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  uint64_t* buf_addr = (uint64_t*)1;
  uint64_t wsid = 1;
  int flags;

  // Open port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  buf_len = (4 * 1024) - 1;
  flags = FPGA_BUF_PREALLOCATED;
  EXPECT_EQ(FPGA_INVALID_PARAM,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, flags));
  // Close the device
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Prep0B
 *
 * @brief      When FPGA_BUF_PREALLOCATED is not given and the buffer
 *             len is 0, fpgaPrepareBuffer allocates a one page buffer
 *             and returns FPGA_OK.
 *
 */
TEST(LibopaecBufCommonALL, Prep0B) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  uint64_t* buf_addr;
  uint64_t wsid = 1;
  int flags;

  // Open port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  buf_len = 0;
  flags = 0;
  EXPECT_EQ(FPGA_OK,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, flags));

  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
  // Close the device
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       PrepNotAligned
 *
 * @brief      When FPGA_BUF_PREALLOCATED is not given and the buffer
 *             len is not a multiple of the page size, fpgaPrepareBuffer
 *             allocates the next multiple of page size and returns
 *             FPGA_OK.
 *
 */
TEST(LibopaecBufCommonALL, PrepNotAligned) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  uint64_t* buf_addr;
  uint64_t wsid = 1;
  int flags;

  // Open port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  buf_len = (4 * 1024) - 1;
  flags = 0;
  EXPECT_EQ(FPGA_OK,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, flags));

  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
  // Close the device
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       PrepRel2MB01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaPrepareBuffer must allocate a shared memory buffer.
 *             fpgaReleaseBuffer must release a shared memory buffer.
 *
 */
TEST(LibopaecBufCommonALL, PrepRel2MB01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  uint64_t* buf_addr;
  uint64_t wsid = 1;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Allocate buffer in MB range
  buf_len = NLB_DSM_SIZE;
  EXPECT_EQ(FPGA_OK,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, 0));

  // Release buffer in MB range
  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Write01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             Test writing to a shared memory buffer.
 *
 */
TEST(LibopaecBufCommonALL, Write01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  void* buf_addr;
  uint64_t wsid = 2;

  uint64_t offset;
  uint64_t value;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Allocate buffer
  buf_len = NLB_DSM_SIZE;
  ASSERT_EQ(FPGA_OK,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, 0));

  // Write test
  memset(buf_addr, 0, buf_len);

  for (offset = 0; offset < buf_len - sizeof(uint64_t);
       offset += sizeof(uint64_t)) {
    value = offset;
    *((volatile uint64_t*)((uint64_t)buf_addr + offset)) = value;
  }

  // Release buffer
  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       WriteRead01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             Test writing and reading to/from a shared memory buffer.
 *
 */
TEST(LibopaecBufCommonALL, WriteRead01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  void* buf_addr;
  uint64_t wsid = 2;

  uint64_t offset;
  uint64_t value;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Allocate buffer
  buf_len = NLB_DSM_SIZE;
  ASSERT_EQ(FPGA_OK,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, 0));

  // Write test
  memset(buf_addr, 0, buf_len);

  for (offset = 0; offset < buf_len - sizeof(uint64_t);
       offset += sizeof(uint64_t)) {
    value = offset;
    *((volatile uint64_t*)((uint64_t)buf_addr + offset)) = value;
    EXPECT_EQ(*((volatile uint64_t*)((uint64_t)buf_addr + offset)), offset);
  }

  // Release buffer
  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       PrepPre2MB01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             with pre-allocated buffer, fpgaPrepareBuffer must
 *             allocate a shared memory buffer. fpgaReleaseBuffer must
 *             release a shared memory buffer.
 *
 */
TEST(LibopaecBufCommonMOCK, PrepPre2MB01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  uint64_t buf_len;
  uint64_t* buf_addr;
  uint64_t wsid = 1;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Allocate buffer in MB range
  buf_len = 2 * 1024 * 1024;
  buf_addr = (uint64_t*)mmap(ADDR, buf_len, PROTECTION, FLAGS_2M, 0, 0);
  EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid,
                                       FPGA_BUF_PREALLOCATED));

  // Release buffer in MB range
  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));

  // buf_addr was preallocated, do not touch it
  ASSERT_NE(buf_addr, (void*)NULL);
  munmap(buf_addr, buf_len);
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}
