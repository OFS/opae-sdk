// Copyright(c) 2017-2022, Intel Corporation
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

#include <linux/ioctl.h>

extern "C" {
#include "xfpga.h"
#include "types_int.h"
#include "fpga-dfl.h"
#include "error_int.h"

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;

#ifndef BUILD_ASE
/*
 * On hardware, the mmio map is a hash table.
 */
static bool mmio_map_is_empty(struct wsid_tracker *root) {
  if (!root || (root->n_hash_buckets == 0))
    return true;

  for (uint32_t i = 0; i < root->n_hash_buckets; i += 1) {
    if (root->table[i])
      return false;
  }

  return true;
}

#else
/*
 * In ASE, the mmio map is a list.
 */
static bool mmio_map_is_empty(struct wsid_map *root) {
  return !root;
}
#endif

int mmio_ioctl(mock_object * m, int request, va_list argp) {
  int retval = -1;
  errno = EINVAL;
  UNUSED_PARAM(m);
  UNUSED_PARAM(request);
  struct dfl_fpga_port_region_info *rinfo = va_arg(argp, struct dfl_fpga_port_region_info *);
  if (!rinfo) {
    OPAE_MSG("rinfo is NULL");
    goto out_EINVAL;
  }
  if (rinfo->argsz != sizeof(*rinfo)) {
    OPAE_MSG("wrong structure size");
    goto out_EINVAL;
  }
  if (rinfo->index > 1) {
    OPAE_MSG("unsupported MMIO index");
    goto out_EINVAL;
  }
  if (rinfo->padding != 0) {
    OPAE_MSG("unsupported padding");
    goto out_EINVAL;
  }
  rinfo->flags = DFL_PORT_REGION_READ | DFL_PORT_REGION_WRITE | DFL_PORT_REGION_MMAP;
  rinfo->size = 0x40000;
  rinfo->offset = 0;
  retval = 0;
  errno = 0;
out:
  return retval;

out_EINVAL:
  retval = -1;
  errno = EINVAL;
  goto out;
}


class openclose_c_p : public opae_p<xfpga_> {
 protected:

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

  virtual void SetUp() override {
    opae_p<xfpga_>::SetUp();

    EXPECT_EQ(xfpga_fpgaClose(accel_), FPGA_OK);
    accel_ = nullptr;
  }

};

/**
 * @test       open_01
 *
 * @brief      When the fpga_handle * parameter to xfpga_fpgaOpen is NULL, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST_P(openclose_c_p, open_01) {
  fpga_result res;
  res = xfpga_fpgaOpen(NULL, NULL, 0);
  ASSERT_EQ(FPGA_INVALID_PARAM, res);
}

/**
 * @test       open_02
 *
 * @brief      When the fpga_token parameter to xfpga_fpgaOpen is NULL, the
 *             function returns FPGA_INVALID_PARAM.
 */

TEST_P(openclose_c_p, open_02) {
  ASSERT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaOpen(NULL, &accel_, 0));
  EXPECT_EQ(accel_, nullptr);
}

/**
 * @test       open_03
 *
 * @brief      When the flags parameter to xfpga_fpgaOpen is invalid, the
 *             function returns FPGA_INVALID_PARAM.
 *
 */
TEST_P(openclose_c_p, open_03) {
  ASSERT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaOpen(accel_token_, NULL, 42));
  ASSERT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaOpen(accel_token_, &accel_, 42));
  EXPECT_EQ(accel_, nullptr);
}

/**
 * @test       open_04
 *
 * @brief      When the flags parameter to xfpga_fpgaOpen is invalid, the
 *             function returns FPGA_INVALID_PARAM.
 *
 */
TEST_P(openclose_c_p, open_04) {
  auto _token = (struct _fpga_token*)accel_token_;
  auto res = xfpga_fpgaOpen(accel_token_, &accel_, 42);
  ASSERT_EQ(FPGA_INVALID_PARAM, res);
  EXPECT_EQ(accel_, nullptr);

  // Invalid token magic
  _token->hdr.magic = 0x123;
  res = xfpga_fpgaOpen(accel_token_, &accel_, FPGA_OPEN_SHARED);
  ASSERT_EQ(FPGA_INVALID_PARAM, res);
  EXPECT_EQ(accel_, nullptr);

  // Reset token magic
  _token->hdr.magic = FPGA_TOKEN_MAGIC;
}

/**
 * @test       open_05
 *
 * @brief      When the flags parameter to xfpga_fpgaOpen is invalid, the
 *             function returns FPGA_INVALID_PARAM and FPGA_NO_DRIVER.
 *
 */
TEST_P(openclose_c_p, open_05) {
  fpga_result res;
  struct _fpga_token* _token = (struct _fpga_token*)accel_token_;

  // Invalid flag
  res = xfpga_fpgaOpen(accel_token_, &accel_, 42);
  ASSERT_EQ(FPGA_INVALID_PARAM, res);
  EXPECT_EQ(accel_, nullptr);

  // Valid flag
  res = xfpga_fpgaOpen(accel_token_, &accel_, FPGA_OPEN_SHARED);
  ASSERT_EQ(FPGA_OK, res);
  ASSERT_EQ(FPGA_OK, xfpga_fpgaClose(accel_));

  // Invalid token path
  accel_ = nullptr;
  strcpy(_token->devpath,"/dev/intel-fpga-fme.01");
  res = xfpga_fpgaOpen(accel_token_, &accel_, FPGA_OPEN_SHARED);
  ASSERT_EQ(FPGA_NO_DRIVER, res);
  EXPECT_EQ(accel_, nullptr);
}

/**
 * @test       open_06
 *
 * @brief      When the flags parameter to xfpga_fpgaOpen is valid, 
 *             but malloc fails. the function returns FPGA_NO_MEMORY.
 *
 */
TEST_P(openclose_c_p, open_06) {
  system_->invalidate_malloc();
  auto res = xfpga_fpgaOpen(accel_token_, &accel_, 0);
  ASSERT_EQ(FPGA_NO_MEMORY, res);
  EXPECT_EQ(accel_, nullptr);
}

/**
 * @test       close_01 
 *
 * @brief      When the flags parameter to xfpga_fpgaOpen is valid, 
 *             but handle fd is invalid. the function returns 
 *             FPGA_INVALID_PARAM.
 *
 */
TEST_P(openclose_c_p, close_01) {
  int fddev = -1;
  auto res = xfpga_fpgaOpen(accel_token_, &accel_, 0);
  ASSERT_EQ(FPGA_OK, res);

  struct _fpga_handle* _handle = (struct _fpga_handle*)accel_;

  // Invalid handle fd
  fddev = _handle->fddev;
  _handle->fddev = -1;
  res = xfpga_fpgaClose(accel_);
  EXPECT_EQ(res, FPGA_INVALID_PARAM);

  // Valid handle fd
  _handle->fddev = fddev;
  res = xfpga_fpgaClose(accel_);
  accel_ = nullptr;
  EXPECT_EQ(res, FPGA_OK);
}

/**
 * @test       invalid_close
 *
 * @brief      When the fpga_handle parameter to fpgaClose is NULL, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST_P(openclose_c_p, invalid_close) {
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaClose(NULL));
}

/**
 * @test       close_03
 *
 * @brief      When the flags parameter to xfpga_fpgaOpen is valid, it 
 *             returns FPGA_OK.  
 *
 */
TEST_P(openclose_c_p, close_03) {
  uint64_t * mmio_ptr = nullptr;
  auto res = xfpga_fpgaOpen(accel_token_, &accel_, 0);
  ASSERT_EQ(FPGA_OK, res);

  // Register valid ioctl
  system_->register_ioctl_handler(DFL_FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
  EXPECT_TRUE(mmio_map_is_empty(((struct _fpga_handle*)accel_)->mmio_root));

  test_device device = platform_.devices[0];
  if (device.has_afu) {
    ASSERT_EQ(FPGA_OK, xfpga_fpgaMapMMIO(accel_, 0, &mmio_ptr));
    EXPECT_NE(mmio_ptr,nullptr);
  }

  res = xfpga_fpgaClose(accel_);
  EXPECT_EQ(res, FPGA_OK);
  accel_ = nullptr;
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(openclose_c_p);
INSTANTIATE_TEST_SUITE_P(openclose_c, openclose_c_p, 
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1",
                                                                        "dfl-c6100"
                                                                      })));

class openclose_c_skx_dcp_p : public openclose_c_p {};

/**
 * @test       open_share
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             and the flag FPGA_OPEN_SHARED is given, fpgaOpen on an
 *             already opened token returns FPGA_OK.
 */
TEST_P(openclose_c_skx_dcp_p, open_share) {
  fpga_handle h1 = nullptr;
  fpga_handle h2 = nullptr;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaOpen(accel_token_, &h1, FPGA_OPEN_SHARED));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaOpen(accel_token_, &h2, FPGA_OPEN_SHARED));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaClose(h1));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaClose(h2));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(openclose_c_skx_dcp_p);
INSTANTIATE_TEST_SUITE_P(openclose_c_skx_dcp, openclose_c_skx_dcp_p,
                         ::testing::ValuesIn(test_platform::platforms({}, fpga_driver::linux_intel)));

class openclose_c_dfl_p : public openclose_c_p {};

/**
 * @test       open_share
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             and the flag FPGA_OPEN_SHARED is given, fpgaOpen on an
 *             already opened token returns FPGA_BUSY.
 */
TEST_P(openclose_c_dfl_p, open_share) {
  fpga_handle h1 = nullptr;
  fpga_handle h2 = nullptr;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaOpen(accel_token_, &h1, FPGA_OPEN_SHARED));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaOpen(accel_token_, &h2, FPGA_OPEN_SHARED));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaClose(h1));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(openclose_c_dfl_p);
INSTANTIATE_TEST_SUITE_P(openclose_c_dfl, openclose_c_dfl_p,
                         ::testing::ValuesIn(test_platform::hw_platforms({}, fpga_driver::linux_dfl0)));

class openclose_c_mock_p : public openclose_c_p {};

/**
 * @test       invalid_open_close
 *
 * @brief      When the flags parameter to xfpga_fpgaOpen is valid, 
 *             but driver is not loaded. the function returns FPGA_NO_DRIVER.
 *
 */
TEST_P(openclose_c_mock_p, invalid_open_close) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;

  const std::string sysfs_port = "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0";
  const std::string dev_port = "/dev/intel-fpga-port.0";

  // token setup
  strncpy(_tok.sysfspath, sysfs_port.c_str(), sysfs_port.size() + 1);
  strncpy(_tok.devpath, dev_port.c_str(), dev_port.size() + 1);
  _tok.hdr.magic = FPGA_TOKEN_MAGIC;
  _tok.errors = nullptr;
  std::string errpath = sysfs_port + "/errors";
  build_error_list(errpath.c_str(), &_tok.errors);

#ifdef BUILD_ASE
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tok, &h, 0));
  ASSERT_EQ(FPGA_OK, xfpga_fpgaClose(h));
  EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
  EXPECT_EQ(FPGA_OK, xfpga_fpgaDestroyToken(&tok));
#else
  EXPECT_EQ(FPGA_NO_DRIVER, xfpga_fpgaOpen(tok, &h, 0));
#endif
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(openclose_c_mock_p);
INSTANTIATE_TEST_SUITE_P(openclose_c, openclose_c_mock_p, 
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-d5005",
                                                                             "dfl-n3000",
                                                                             "dfl-n6000-sku0",
                                                                             "dfl-n6000-sku1",
                                                                             "dfl-c6100"
                                                                           })));
