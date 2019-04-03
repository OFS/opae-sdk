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

#include "types_int.h"

#include "common_test.h"

using namespace std;
using namespace common_test;

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
TEST(LibopaecBufCommonMOCKHW, InvalidWSID01) {
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
TEST(LibopaecBufCommonMOCKHW, PrepRel4K01) {
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
 *             len is 0, fpgaPrepareBuffer doesn't allocates buffer
 *             and returns FPGA_INVALID_PARAM.
 *
 */
TEST(LibopaecBufCommonALL, Prep0B) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  
  // Open port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
#ifndef BUILD_ASE
  uint64_t buf_len;
  uint64_t* buf_addr;
  uint64_t wsid = 1;
  int flags;
  buf_len = 0;
  flags = 0;
  EXPECT_EQ(FPGA_INVALID_PARAM,
            fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, flags));

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReleaseBuffer(h, wsid));
#endif
  
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


