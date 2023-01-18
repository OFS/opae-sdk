// Copyright(c) 2018-2023, Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#define NO_OPAE_C
#include "mock/opae_fixtures.h"
KEEP_XFPGA_SYMBOLS

extern "C" {
const char * xfpga_fpgaErrStr(fpga_result);
fpga_result prop_check_and_lock(struct _fpga_properties*);
fpga_result handle_check_and_lock(struct _fpga_handle*);
fpga_result event_handle_check_and_lock(struct _fpga_event_handle*);

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);

#include "props.h"
}

#include "types_int.h"
#include "sysfs_int.h"
#include "intel-fpga.h"
#include "xfpga.h"

using namespace opae::testing;

class common_c_p : public opae_device_p<xfpga_> {
 protected:
  common_c_p() :
    eh_(nullptr)
  {}

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

  virtual void SetUp() override {
    opae_device_p<xfpga_>::SetUp();

    ASSERT_EQ(xfpga_fpgaCreateEventHandle(&eh_), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(xfpga_fpgaDestroyEventHandle(&eh_), FPGA_OK);

    opae_device_p<xfpga_>::TearDown();
  }

  fpga_event_handle eh_;
};

/**
 * @test       common_01
 *
 * @brief      Verifies the string returned by fpgaErrStr() for each
 *             fpga_result enumeration value.
 */
TEST(common, fpgaErrStr) {
  EXPECT_STREQ("success",                 xfpga_fpgaErrStr(FPGA_OK));
  EXPECT_STREQ("invalid parameter",       xfpga_fpgaErrStr(FPGA_INVALID_PARAM));
  EXPECT_STREQ("resource busy",           xfpga_fpgaErrStr(FPGA_BUSY));
  EXPECT_STREQ("exception",               xfpga_fpgaErrStr(FPGA_EXCEPTION));
  EXPECT_STREQ("not found",               xfpga_fpgaErrStr(FPGA_NOT_FOUND));
  EXPECT_STREQ("no memory",               xfpga_fpgaErrStr(FPGA_NO_MEMORY));
  EXPECT_STREQ("not supported",           xfpga_fpgaErrStr(FPGA_NOT_SUPPORTED));
  EXPECT_STREQ("no driver available",     xfpga_fpgaErrStr(FPGA_NO_DRIVER));
  EXPECT_STREQ("no fpga daemon running",  xfpga_fpgaErrStr(FPGA_NO_DAEMON));
  EXPECT_STREQ("insufficient privileges", xfpga_fpgaErrStr(FPGA_NO_ACCESS));
  EXPECT_STREQ("reconfiguration error",   xfpga_fpgaErrStr(FPGA_RECONF_ERROR));
}

/**
 * @test       prop_check_and_lock
 *
 * @brief      When fpga_properties magic is invalid
 *             fpga_result returns FPGA_INVALID_PARAM
 */

TEST(common, prop_check_and_lock) {
  struct _fpga_properties *prop;
  prop = opae_properties_create();

  auto res = prop_check_and_lock(prop);
  EXPECT_EQ(FPGA_OK, res);

  prop->magic = 0x123;
  res = prop_check_and_lock(prop);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  opae_free(prop);
  prop = nullptr;
}

/**
 * @test       handle_check_and_lock
 *
 * @brief      When fpga_handle magic is invalid
 *             fpga_result returns FPGA_INVALID_PARAM
 */
TEST_P(common_c_p, handle_check_and_lock) {
  struct _fpga_handle *h = (struct _fpga_handle*)device_;
  h->hdr.magic = 0x123;
  auto res = handle_check_and_lock((struct _fpga_handle*)device_);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);
  h->hdr.magic = FPGA_HANDLE_MAGIC;
  res = handle_check_and_lock((struct _fpga_handle*)device_);
  EXPECT_EQ(FPGA_OK, res);
}

/**
 * @test       event_handle_check_and_lock
 *
 * @brief      When event_fpga_handle magic is invalid
 *             fpga_result returns FPGA_INVALID_PARAM
 */

TEST_P(common_c_p, event_handle_check_and_lock) {
  auto res = event_handle_check_and_lock((struct _fpga_event_handle*)eh_);
  EXPECT_EQ(FPGA_OK,res);
  struct _fpga_event_handle *eh = (struct _fpga_event_handle*)eh_;

  eh->magic = 0x123;
  res = event_handle_check_and_lock((struct _fpga_event_handle*)eh_);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  eh->magic = FPGA_EVENT_HANDLE_MAGIC;
  res = event_handle_check_and_lock((struct _fpga_event_handle*)eh_);
  EXPECT_EQ(FPGA_OK, res);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(common_c_p);
INSTANTIATE_TEST_SUITE_P(common_c, common_c_p,
                         ::testing::ValuesIn(test_platform::platforms({})));
