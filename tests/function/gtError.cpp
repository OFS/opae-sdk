// Copyright(c) 2018, Intel Corporation
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

#include <opae/error.h>

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

using namespace common_test;

/**
 * @test       error_01
 *
 * @brief      When passed a valid AFU token, the combination of fpgaGetProperties()
 *             fpgaPropertiesGetNumErrors(), fpgaPropertiesGetErrorInfo() and
 *             fpgaReadError() is able to print the status of all error registers.
 *
 */
TEST(LibopaecErrorCommonALL, error_01) {
  struct _fpga_token _t;
  fpga_token t = &_t;
  fpga_properties p;
  fpga_error_info info;
  unsigned int n = 0;
  unsigned int i = 0;
  uint64_t val = 0;

  // generate token
  token_for_afu0(&_t);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, fpgaGetProperties(t, &p));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(p, &n));
  printf("Found %d PORT error registers\n", n);

  // for each error register, get info and read the current value  
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, fpgaGetErrorInfo(t, i, &info));
    EXPECT_EQ(FPGA_OK, fpgaReadError(t, i, &val));
    printf("[%u] %s: 0x%016lX\n", i, info.name, val);
  }

}

/**
 * @test       error_02
 *
 * @brief      When passed a valid FME token, the combination of fpgaGetProperties()
 *             fpgaPropertiesGetNumErrors(), fpgaPropertiesGetErrorInfo() and
 *             fpgaReadError() is able to print the status of all error registers.
 *
 */
TEST(LibopaecErrorCommonALL, error_02) {
  struct _fpga_token _t;
  fpga_token t = &_t;
  fpga_properties p;
  fpga_error_info info;
  unsigned int n = 0;
  unsigned int i = 0;
  uint64_t val = 0;

  // generate token
  token_for_fme0(&_t);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, fpgaGetProperties(t, &p));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(p, &n));
  printf("Found %d FME error registers\n", n);

  // for each error register, get info and read the current value  
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, fpgaGetErrorInfo(t, i, &info));
    EXPECT_EQ(FPGA_OK, fpgaReadError(t, i, &val));
    printf("[%u] %s: 0x%016lX\n", i, info.name, val);
  }

}
