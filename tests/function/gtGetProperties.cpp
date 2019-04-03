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

#ifdef __cplusplus
extern "C" {
#endif
#include <opae/access.h>

#ifdef __cplusplus
}
#endif

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

using namespace common_test;

/**
 * @test       vendor_id_0
 *
 * @brief      When querying the vendor ID of an AFU,
 * 	       0x8086 is returned.
 */
TEST(LibopaecGetPropertiesCommonALL, vendor_id_0) {
#ifndef BUILD_ASE
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_properties prop;
  uint16_t x = 0;

  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaGetProperties(tok, &prop));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetVendorID(prop, &x));
  EXPECT_EQ(x, 0x8086);
#endif
}

/**
 * @test       vendor_id_1
 *
 * @brief      When querying the vendor ID of an FME,
 * 	       0x8086 is returned.
 */
TEST(LibopaecGetPropertiesCommonALL, vendor_id_1) {
#ifndef BUILD_ASE
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_properties prop;
  uint16_t x = 0;

  token_for_fme0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaGetProperties(tok, &prop));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetVendorID(prop, &x));
  EXPECT_EQ(x, 0x8086);
#endif
}

/**
 * @test       device_id_0
 *
 * @brief      When querying the device ID of an AFU,
 * 	       0xbcc0 is returned.
 */
TEST(LibopaecGetPropertiesCommonALL, device_id_0) {
#ifndef BUILD_ASE
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_properties prop;
  uint16_t x = 0;

  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaGetProperties(tok, &prop));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetDeviceID(prop, &x));
  EXPECT_EQ(x, 0xbcc0);
#endif
}

/**
 * @test       device_id_1
 *
 * @brief      When querying the device ID of an FME,
 * 	       0xbcc0 is returned.
 */
TEST(LibopaecGetPropertiesCommonALL, device_id_1) {
#ifndef BUILD_ASE
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_properties prop;
  uint16_t x = 0;

  token_for_fme0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaGetProperties(tok, &prop));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetDeviceID(prop, &x));
  EXPECT_EQ(x, 0xbcc0);
#endif
}

/**
 * @test       get_02
 *
 * @brief      When fpgaGetPropertiesFromHandle is called
 * 	       with a NULL handle,
 * 	       Then the function returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecGetPropertiesCommonALL, get_02) {
  fpga_properties prop = NULL;
      
  ASSERT_EQ(FPGA_INVALID_PARAM, fpgaGetPropertiesFromHandle(NULL, &prop));
}

class LibopaecGetPropertiesFCommonMOCKHW : public BaseFixture, public ::testing::Test {
 protected:
  virtual void SetUp() {
    m_AFUHandle = NULL;
    m_FMEHandle = NULL;

    token_for_afu0(&m_AFUToken);
    ASSERT_EQ(FPGA_OK, fpgaOpen(&m_AFUToken, &m_AFUHandle, 0));

    token_for_fme0(&m_FMEToken);
    ASSERT_EQ(FPGA_OK, fpgaOpen(&m_FMEToken, &m_FMEHandle, 0));
  }

  virtual void TearDown() {
    EXPECT_EQ(FPGA_OK, fpgaClose(m_FMEHandle));
    EXPECT_EQ(FPGA_OK, fpgaClose(m_AFUHandle));
  }

  struct _fpga_token m_AFUToken;
  struct _fpga_token m_FMEToken;
  fpga_handle m_AFUHandle;
  fpga_handle m_FMEHandle;
};

/**
 * @test       get_01
 *
 * @brief      When fpgaGetPropertiesFromHandle is called
 *             with a valid handle,
 *             the function returns FPGA_OK, and
 *             the returned properties are valid for that handle.
 */
TEST_F(LibopaecGetPropertiesFCommonMOCKHW, get_01) {
  fpga_properties prop = NULL;
  fpga_objtype objtype = FPGA_DEVICE;

  ASSERT_EQ(FPGA_OK, fpgaGetPropertiesFromHandle(m_AFUHandle, &prop));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetObjectType(prop, &objtype));
  EXPECT_EQ(FPGA_ACCELERATOR, objtype);

  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));

  prop = NULL;
  objtype = FPGA_ACCELERATOR;

  ASSERT_EQ(FPGA_OK, fpgaGetPropertiesFromHandle(m_FMEHandle, &prop));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetObjectType(prop, &objtype));
  EXPECT_EQ(FPGA_DEVICE, objtype);

  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));
}
