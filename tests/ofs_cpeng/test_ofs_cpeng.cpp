// Copyright(c) 2021, Intel Corporation
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
#include <chrono>
#include <future>
#include <thread>
#include <uuid/uuid.h>
#include "gtest/gtest.h"
#include "ofs_cpeng.h"


/**
 * @test    wait_for_hps_ready
 * @brief   Tests: ofs_cpeng_wait_for_hps_ready
 * @details Tests ofs_cpeng_wait_for_ready by setting the register pointer to
 *          the address of a local variable of that type. First set the ready
 *          bit to 0, call ofs_cpeng_wait_for_ready, and verify the result is
 *          non-zero. Then set the ready bit to 1, call
 *          ofs_cpeng_wait_for_ready and verify the result is 0.
 * */
TEST(ofs_cpeng, wait_for_hps_ready)
{
  ofs_cpeng otest;
  CSR_HPS2HOST_RDY_STATUS ready;
  otest.r_CSR_HPS2HOST_RDY_STATUS = &ready;

  ready.f_HPS_RDY = 0;
  EXPECT_EQ(ofs_cpeng_wait_for_hps_ready(&otest, 1000), 1);

  ready.f_HPS_RDY = 1;
  EXPECT_EQ(ofs_cpeng_wait_for_hps_ready(&otest, 1000), 0);
}

/**
 * @test    copy_chunk
 * @brief   Tests: copy_chunk
 * @details Tests ofs_cpeng_copy_chunk by setting the register pointers in
 * ofs_cpeng structure to local variables. Modify bit field variables in the
 * test before calling ofs_cpeng_copy_chunk then verify that those bit fields
 * are changed as expected by that call.
 * */
TEST(ofs_cpeng, copy_chunk)
{
  ofs_cpeng otest;
  CSR_SRC_ADDR r_src = {0};
  CSR_DST_ADDR r_dst = {0};
  CSR_DATA_SIZE r_size = {0};
  CSR_HOST2CE_MRD_START r_start = {0};
  CSR_CE2HOST_STATUS r_status = {0};

  otest.r_CSR_SRC_ADDR = &r_src;
  otest.r_CSR_DST_ADDR = &r_dst;
  otest.r_CSR_DATA_SIZE = &r_size;
  otest.r_CSR_HOST2CE_MRD_START = &r_start;
  otest.r_CSR_CE2HOST_STATUS = &r_status;

  ASSERT_EQ(r_src.value, 0);
  ASSERT_EQ(r_dst.value, 0);
  ASSERT_EQ(r_size.value, 0);
  ASSERT_EQ(r_start.value, 0);
  ASSERT_EQ(r_status.value, 0);
  EXPECT_EQ(ofs_cpeng_copy_chunk(&otest, 0x1000, 0x2000, 4096), 1);
  EXPECT_EQ(r_src.f_CSR_SRC_ADDR, 0x1000);
  EXPECT_EQ(r_dst.f_CSR_DST_ADDR, 0x2000);
  EXPECT_EQ(r_size.f_CSR_DATA_SIZE, 4096);


  r_status.f_CE_DMA_STS = 0b10;
  EXPECT_EQ(ofs_cpeng_copy_chunk(&otest, 0x3000, 0x4000, 8192), 0);
  EXPECT_EQ(r_src.f_CSR_SRC_ADDR, 0x3000);
  EXPECT_EQ(r_dst.f_CSR_DST_ADDR, 0x4000);
  EXPECT_EQ(r_size.f_CSR_DATA_SIZE, 8192);
}
