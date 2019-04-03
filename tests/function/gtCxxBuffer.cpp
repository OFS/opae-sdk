#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#include <memory>
#include <unistd.h>

#include "gtest/gtest.h"

#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/except.h>


using namespace opae::fpga::types;

class LibopaecppBufCommonALL_f1 : public ::testing::Test {
 protected:
  LibopaecppBufCommonALL_f1() {}

  virtual void SetUp() override {
    tokens_ = token::enumerate({properties::get(FPGA_ACCELERATOR)});
    ASSERT_GT(tokens_.size(), 0);
    accel_ = handle::open(tokens_[0], 0);
    ASSERT_NE(nullptr, accel_.get());
  }

  virtual void TearDown() override {
    accel_.reset();
    ASSERT_NO_THROW(tokens_.clear());
  }

  std::vector<token::ptr_t> tokens_;
  handle::ptr_t accel_;
  shared_buffer::ptr_t buf_;
};

/**
 * @test alloc_01
 * Given an open accelerator handle object<br>
 * When I call shared_buffer::allocate() with a length of 0<br>
 * Then an exception is throw of type opae::fpga::types::except
 */
TEST_F(LibopaecppBufCommonALL_f1, alloc_01) {
  ASSERT_THROW(buf_ = shared_buffer::allocate(accel_, 0), except);
}

/**
 * @test alloc_02
 * Given an open accelerator handle object<br>
 * When I call shared_buffer::allocate() with a length greater than 0<br>
 * Then I get a valid shared_buffer pointer.<br>
 */
TEST_F(LibopaecppBufCommonALL_f1, alloc_02) {
  buf_ = shared_buffer::allocate(accel_, 64);
  ASSERT_NE(nullptr, buf_.get());

  EXPECT_EQ(64, buf_->size());
  EXPECT_NE(0, buf_->io_address());
}

/**
 * @test release_01
 * Given an open accelerator handle object<br>
 * And a valid shared_buffer object created with allocate<br>
 * When I call shared_buffer::release()<br>
 * Then the shared_buffer object is invalidated<br>
 */
TEST_F(LibopaecppBufCommonALL_f1, release_01) {
  buf_ = shared_buffer::allocate(accel_, 64);
  ASSERT_NE(nullptr, buf_.get());

  EXPECT_EQ(64, buf_->size());
  EXPECT_NE(0, buf_->io_address());
  EXPECT_NE(nullptr, buf_->c_type());

  buf_->release();
  EXPECT_EQ(0, buf_->size());
  EXPECT_EQ(0, buf_->io_address());
  EXPECT_EQ(0, buf_->c_type());
}

/**
 * @test alloc_07
 * Given an open accelerator handle object and a pre-allocated buffer<br>
 * When I call shared_buffer::attach() with a length that is a multiple of the page
 * size<br> Then I get a valid shared_buffer pointer.<br>
 */
TEST_F(LibopaecppBufCommonALL_f1, alloc_07) {
  uint64_t pg_size = (uint64_t)sysconf(_SC_PAGE_SIZE);
  uint8_t *buf = (uint8_t *)aligned_alloc(pg_size, pg_size);

  buf_ = shared_buffer::attach(accel_, buf, pg_size);
  ASSERT_NE(nullptr, buf_.get());
  EXPECT_EQ(static_cast<shared_buffer::size_t>(pg_size), buf_->size());
  EXPECT_NE(0, buf_->io_address());
  free(buf);
}

/**
 * @test fill_compare_04
 * Given a valid shared_buffer smart pointer<br>
 * When I call shared_buffer::fill(),<br>
 * Each byte of the buffer is set to the input value.<br>
 * When I call shared_buffer::compare(),<br>
 * Then a byte-wise comparison is performed.<br>
 */
TEST_F(LibopaecppBufCommonALL_f1, fill_compare_04) {
  buf_ = shared_buffer::allocate(accel_, 4);
  ASSERT_NE(nullptr, buf_.get());

  shared_buffer::ptr_t buf2 = shared_buffer::allocate(accel_, 4);
  ASSERT_NE(nullptr, buf2.get());

  buf_->fill(1);
  buf2->fill(1);
  EXPECT_EQ(0, buf_->compare(buf2, buf_->size()));
}

/**
 * @test read_write_05
 * Given a valid shared_buffer smart pointer<br>
 * When I call shared_buffer::write(),<br>
 * Then the requested memory block is updated.<br>
 * When I call shared_buffer::read(),<br>
 * Then the requested memory block is returned.<br>
 */
TEST_F(LibopaecppBufCommonALL_f1, read_write_05) {
  buf_ = shared_buffer::allocate(accel_, 4);
  ASSERT_NE(nullptr, buf_.get());

  buf_->write<uint32_t>(0xdecafbad, 0);
  EXPECT_EQ(0xdecafbad, buf_->read<uint32_t>(0));
}
