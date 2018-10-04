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

#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/token.h>
#include "gtest/gtest.h"
#include "test_system.h"

#include <linux/ioctl.h>
#include <cstdarg>
#include "intel-fpga.h"

using namespace opae::testing;
using namespace opae::fpga::types;

int mmio_ioctl(mock_object *m, int request, va_list argp) {
  int retval = -1;
  errno = EINVAL;
  UNUSED_PARAM(m);
  UNUSED_PARAM(request);
  struct fpga_port_region_info *rinfo =
      va_arg(argp, struct fpga_port_region_info *);
  if (!rinfo) {
    FPGA_MSG("rinfo is NULL");
    goto out_EINVAL;
  }
  if (rinfo->argsz != sizeof(*rinfo)) {
    FPGA_MSG("wrong structure size");
    goto out_EINVAL;
  }
  if (rinfo->index > 1) {
    FPGA_MSG("unsupported MMIO index");
    goto out_EINVAL;
  }
  if (rinfo->padding != 0) {
    FPGA_MSG("unsupported padding");
    goto out_EINVAL;
  }
  rinfo->flags = FPGA_REGION_READ | FPGA_REGION_WRITE | FPGA_REGION_MMAP;
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

class handle_cxx_core : public ::testing::TestWithParam<std::string> {
 protected:
  handle_cxx_core() : handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaInitialize(nullptr), FPGA_OK);

    tokens_ = token::enumerate({properties::get(FPGA_ACCELERATOR)});
    ASSERT_TRUE(tokens_.size() > 0);

    system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
  }

  virtual void TearDown() override { system_->finalize(); }

  std::vector<token::ptr_t> tokens_;
  handle::ptr_t handle_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test open
 * Given an environment with at least one accelerator<br>
 * When I call token::enumerate with a filter of only FPGA_ACCELERATOR<br>
 * And I call handle::open with a token<br>
 * Then I get a non-null handle<br>
 * And no exceptions are thrown when I release the handle and tokens<br>
 */
TEST_P(handle_cxx_core, open) {
  handle_ = handle::open(tokens_[0], FPGA_OPEN_SHARED);
  ASSERT_NE(nullptr, handle_.get());
  ASSERT_NO_THROW(handle_->reset());
  ASSERT_NO_THROW(tokens_.clear());
}

/**
 * @test open_null
 * Given NULL as the pointer to the token, handle::open should
 * throw an invalid_argument exception.
 */
TEST_P(handle_cxx_core, close_null) {
  int flags = 0;
  token::ptr_t tok = nullptr;

  EXPECT_THROW(handle::open(tok, flags), std::invalid_argument);
}

/**
 * @test reconfigure_null
 * Given an empty bitstream, handle::reconfigure should throw
 * an exception.
 */
TEST_P(handle_cxx_core, reconfigure_null) {
  uint32_t slot = 0;
  const uint8_t *bitstream = {};
  size_t size = 0;
  int flags = 0;

  handle_ = handle::open(tokens_[0], flags);
  ASSERT_NE(nullptr, handle_.get());

  EXPECT_THROW(handle_->reconfigure(slot, bitstream, size, flags),
               invalid_param);
}

/**
 * @test mmio_32
 * write_csr32 should be able to write a value and read_csr32
 * should be able to read it back.
 */
TEST_P(handle_cxx_core, mmio_32) {
  int flags = 0;
  uint64_t offset = 0x100;
  uint32_t csr_space = 0;
  uint32_t value = 10;

  handle_ = handle::open(tokens_[0], flags);
  ASSERT_NE(nullptr, handle_.get());

  ASSERT_NO_THROW(handle_->write_csr32(offset, value, csr_space));
  value = handle_->read_csr32(offset, csr_space);
  EXPECT_EQ(value, 10);
}

/**
 * @test mmio_64
 * write_csr64 should be able to write a value and read_csr64
 * should be able to read it back.
 */
TEST_P(handle_cxx_core, mmio_64) {
  int flags = 0;
  uint64_t offset = 0x100;
  uint32_t csr_space = 0;
  uint64_t value = 10;

  handle_ = handle::open(tokens_[0], flags);
  ASSERT_NE(nullptr, handle_.get());

  ASSERT_NO_THROW(handle_->write_csr64(offset, value, csr_space));
  value = handle_->read_csr64(offset, csr_space);
  EXPECT_EQ(value, 10);
}

/**
 * @test mmio_ptr
 * Verify that handle::mmio_ptr is able to map mmio and retrieve
 * the pointer.
 */
TEST_P(handle_cxx_core, mmio_ptr) {
  int flags = 0;
  uint64_t offset = 0x100;
  uint32_t csr_space = 0;
  uint8_t *h;

  handle_ = handle::open(tokens_[0], flags);
  ASSERT_NE(nullptr, handle_.get());

  ASSERT_NO_THROW(h = handle_->mmio_ptr(offset, csr_space));
  ASSERT_NE(nullptr, h);
}

INSTANTIATE_TEST_CASE_P(handle, handle_cxx_core,
                        ::testing::ValuesIn(test_platform::keys(true)));
