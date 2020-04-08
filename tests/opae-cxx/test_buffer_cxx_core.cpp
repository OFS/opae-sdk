// Copyright(c) 2018-2020, Intel Corporation
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

#include "mock/test_system.h"
#include "gtest/gtest.h"
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/token.h>
#include "common_int.h"

using namespace opae::testing;
using namespace opae::fpga::types;

class buffer_cxx_core : public ::testing::TestWithParam<std::string> {
protected:
  buffer_cxx_core() : handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaInitialize(nullptr), FPGA_OK);

    tokens_ = token::enumerate({properties::get(FPGA_ACCELERATOR)});
    ASSERT_TRUE(tokens_.size() > 0);

    handle_ = handle::open(tokens_[0], FPGA_OPEN_SHARED);
    ASSERT_NE(nullptr, handle_.get());
  }

  virtual void TearDown() override {
    tokens_.clear();
    if (handle_.get())
      handle_->close();
    handle_.reset();
    fpgaFinalize();

    system_->finalize();
  }

  std::vector<token::ptr_t> tokens_;
  handle::ptr_t handle_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test shared_buffer::allocate_null
 * Calling shared_buffer::allocate with a null handle should throw
 * invalid_param.
 */
TEST_P(buffer_cxx_core, allocate_null) {
  size_t length = 4096;
  shared_buffer::ptr_t buf;

  EXPECT_THROW(shared_buffer::allocate(NULL, length), std::exception);
}

/**
 * @test shared_buffer::allocate_no_len
 * Calling shared_buffer::allocate with buffer length = 0 should throw
 * invalid_param.
 */
TEST_P(buffer_cxx_core, allocate_no_len) {
  size_t length = 0;
  shared_buffer::ptr_t buf;

  EXPECT_THROW(shared_buffer::allocate(handle_, length), std::exception);
}

/**
 * @test shared_buffer::allocate_valid
 * Calling shared_buffer::allocate with valid params should return a
 * shared buffer and shared_buffer::release will invalidate the
 * object.
 */
TEST_P(buffer_cxx_core, allocate_valid) {
  size_t length = 4096;
  shared_buffer::ptr_t buf;

  ASSERT_NO_THROW(buf = shared_buffer::allocate(handle_, length));
  ASSERT_NE(nullptr, buf);
  EXPECT_EQ(length, buf->size());

  ASSERT_NO_THROW(buf->release());
  ASSERT_NE(nullptr, buf.get());
  EXPECT_EQ(0, buf->size());
  EXPECT_EQ(0, buf->c_type());
}

/**
 * @test shared_buffer::attach_no_len
 * Calling shared_buffer::attach with buffer length = 0 should throw
 * invalid_param.
 */
TEST_P(buffer_cxx_core, attach_no_len) {
  size_t length = 4096;
  uint8_t *buf_addr = nullptr;
  shared_buffer::ptr_t buf;

  ASSERT_NO_THROW(buf = shared_buffer::allocate(handle_, length));
  ASSERT_NE(nullptr, buf.get());

  buf_addr = (uint8_t *)mmap(ADDR, length, PROTECTION, FLAGS_2M, 0, 0);
  ASSERT_NE(nullptr, buf_addr);

  EXPECT_THROW(buf->attach(handle_, buf_addr, 0), invalid_param);

  munmap(buf_addr, length);
}

/**
 * @test attach_valid
 * Given an open accelerator handle object and a pre-allocated buffer,
 * when I call shared_buffer::attach() with a length that is a multiple
 * of the page size, then I get a valid shared_buffer pointer.
 */
TEST_P(buffer_cxx_core, attach_valid) {
  shared_buffer::ptr_t buf;
  uint64_t pg_size = (uint64_t)sysconf(_SC_PAGE_SIZE);
  uint8_t *base_addr = (uint8_t *)aligned_alloc(pg_size, pg_size);

  ASSERT_NE(nullptr, base_addr);

  buf = shared_buffer::attach(handle_, base_addr, pg_size);
  ASSERT_NE(nullptr, buf.get());
  EXPECT_EQ(static_cast<shared_buffer::size_t>(pg_size), buf->size());
  free(base_addr);
}

/**
 * @test shared_buffer::compare_diff
 * Calling shared_buffer::compare on two buffers with different
 * values should return a non-zero value.
 */
TEST_P(buffer_cxx_core, compare_diff) {
  size_t length = 4096;
  shared_buffer::ptr_t buf1;
  shared_buffer::ptr_t buf2;

  ASSERT_NO_THROW(buf1 = shared_buffer::allocate(handle_, length));
  ASSERT_NE(nullptr, buf1.get());
  ASSERT_NO_THROW(buf2 = shared_buffer::allocate(handle_, length));
  ASSERT_NE(nullptr, buf2.get());

  buf1->fill(3);
  buf2->fill(1);

  // Different buffer values
  EXPECT_NE(buf1->compare(buf2, 4096), 0);
}

/**
 * @test shared_buffer::compare
 * Calling shared_buffer::compare on two buffers with identical
 * values should return zero.
 */
TEST_P(buffer_cxx_core, compare_same) {
  size_t length = 4096;
  shared_buffer::ptr_t buf1;
  shared_buffer::ptr_t buf2;

  ASSERT_NO_THROW(buf1 = shared_buffer::allocate(handle_, length));
  ASSERT_NE(nullptr, buf1.get());
  ASSERT_NO_THROW(buf2 = shared_buffer::allocate(handle_, length));
  ASSERT_NE(nullptr, buf2.get());

  buf1->fill(3);
  buf2->fill(3);

  EXPECT_EQ(buf1->compare(buf2, 4096), 0);
}

/**
 * @test shared_buffer::read_write
 * Calling shared_buffer::write updates the memory block and
 * calling shared_buffer::read returns the memory block.
 */
TEST_P(buffer_cxx_core, read_write) {
  shared_buffer::ptr_t buf;

  buf = shared_buffer::allocate(handle_, 4);
  ASSERT_NE(nullptr, buf.get());

  buf->write<uint32_t>(0xdecafbad, 0);
  EXPECT_EQ(0xdecafbad, buf->read<uint32_t>(0));
}

INSTANTIATE_TEST_CASE_P(buffer, buffer_cxx_core,
                        ::testing::ValuesIn(test_platform::keys(true)));
