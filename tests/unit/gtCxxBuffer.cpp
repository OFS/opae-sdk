#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#include <memory>
#include <unistd.h>

#include "gtest/gtest.h"

#include "opaec++/dma_buffer.h"
#include "opaec++/buffers.h"
#include "opaec++/handle.h"
#include "opaec++/except.h"

using namespace opae::fpga::types;
using namespace opae::fpga::memory;

class CxxBuffer_f1 : public ::testing::Test {
 protected:
  CxxBuffer_f1() {}

  virtual void SetUp() override {
    tokens_ = token::enumerate({FPGA_ACCELERATOR});
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
  dma_buffer::ptr_t buf_;
};

/**
 * @test alloc_01
 * Given an open accelerator handle object<br>
 * When I call dma_buffer::allocate() with a length of 0<br>
 * Then an exception is throw of type opae::fpga::types::except
 */
TEST_F(CxxBuffer_f1, alloc_01) {
  ASSERT_THROW(buf_ = dma_buffer::allocate(accel_, 0), except);
}

/**
 * @test alloc_02
 * Given an open accelerator handle object<br>
 * When I call dma_buffer::allocate() with a length greater than 0<br>
 * Then I get a valid dma_buffer pointer.<br>
 */
TEST_F(CxxBuffer_f1, alloc_02) {
  buf_ = dma_buffer::allocate(accel_, 64);
  ASSERT_NE(nullptr, buf_.get());

  EXPECT_EQ(64, buf_->size());
  EXPECT_NE(0, buf_->iova());
}

/**
 * @test alloc_07
 * Given an open accelerator handle object and a pre-allocated buffer<br>
 * When I call dma_buffer::attach() with a length that is a multiple of the page
 * size<br> Then I get a valid dma_buffer pointer.<br>
 */
TEST_F(CxxBuffer_f1, alloc_07) {
  uint64_t pg_size = (uint64_t)sysconf(_SC_PAGE_SIZE);
  uint8_t *buf = (uint8_t *)malloc(pg_size);

  buf_ = dma_buffer::attach(accel_, buf, pg_size);
  ASSERT_NE(nullptr, buf_.get());
  EXPECT_EQ(static_cast<dma_buffer::size_t>(pg_size), buf_->size());
  EXPECT_NE(0, buf_->iova());
  free(buf);
}

/**
 * @test split_03
 * Given a valid dma_buffer smart pointer<br>
 * When I convert it to a buffer_slice <br>
 * And I call buffer_slice::split()<br>
 * The sub-buffers are created according to the initialier_list.<br>
 */
TEST_F(CxxBuffer_f1, split_03) {
  buf_ = dma_buffer::allocate(accel_, 64);
  ASSERT_NE(nullptr, buf_.get());
  auto chunk = buffer_slice::convert(buf_);
  std::vector<dma_buffer::ptr_t> v = chunk->split({16, 16, 16, 16});

  EXPECT_EQ(chunk->get(), v[0]->get());
  EXPECT_EQ(chunk->iova(), v[0]->iova());
  EXPECT_EQ(16, v[0]->size());

  EXPECT_EQ(chunk->get() + 16, v[1]->get());
  EXPECT_EQ(chunk->iova() + 16, v[1]->iova());
  EXPECT_EQ(16, v[1]->size());

  EXPECT_EQ(chunk->get() + 32, v[2]->get());
  EXPECT_EQ(chunk->iova() + 32, v[2]->iova());
  EXPECT_EQ(16, v[2]->size());

  EXPECT_EQ(chunk->get() + 48, v[3]->get());
  EXPECT_EQ(chunk->iova() + 48, v[3]->iova());
  EXPECT_EQ(16, v[3]->size());
}

/**
 * @test fill_compare_04
 * Given a valid dma_buffer smart pointer<br>
 * When I call dma_buffer::fill(),<br>
 * Each byte of the buffer is set to the input value.<br>
 * When I call dma_buffer::compare(),<br>
 * Then a byte-wise comparison is performed.<br>
 */
TEST_F(CxxBuffer_f1, fill_compare_04) {
  buf_ = dma_buffer::allocate(accel_, 4);
  ASSERT_NE(nullptr, buf_.get());

  dma_buffer::ptr_t buf2 = dma_buffer::allocate(accel_, 4);
  ASSERT_NE(nullptr, buf2.get());

  buf_->fill(1);
  buf2->fill(1);
  EXPECT_EQ(0, buf_->compare(buf2, buf_->size()));
}

/**
 * @test read_write_05
 * Given a valid dma_buffer smart pointer<br>
 * When I call dma_buffer::write(),<br>
 * Then the requested memory block is updated.<br>
 * When I call dma_buffer::read(),<br>
 * Then the requested memory block is returned.<br>
 */
TEST_F(CxxBuffer_f1, read_write_05) {
  buf_ = dma_buffer::allocate(accel_, 4);
  ASSERT_NE(nullptr, buf_.get());

  buf_->write<uint32_t>(0xdecafbad, 0);
  EXPECT_EQ(0xdecafbad, buf_->read<uint32_t>(0));
}

/**
 * @test poll_wait_06
 * Given a valid dma_buffer smart pointer<br>
 * When I call poll(),<br>
 * Then the return value is false when the given mask/value are not observed
 * .<br> When I call poll(),<br> Then the return value is true when the given
 * mask/value are observed .<br> When I call wait(),<br> Then the return value
 * is false when the given mask/value are not observed .<br> When I call
 * wait(),<br> Then the return value is true when the given mask/value are
 * observed .<br>
 */
TEST_F(CxxBuffer_f1, poll_wait_06) {
  buf_ = dma_buffer::allocate(accel_, 4);
  ASSERT_NE(nullptr, buf_.get());

  dma_buffer::size_t offset = 0;
  std::chrono::microseconds micros(1000);
  std::chrono::microseconds each(100);
  uint32_t mask = 0xffffffff;
  uint32_t value = 0xdecafbad;
  uint32_t wrong_value = 0xdeadbeef;

  buf_->write<uint32_t>(value, offset);
  EXPECT_FALSE(poll(buf_, offset, micros, mask, wrong_value));
  EXPECT_TRUE(poll(buf_, offset, micros, mask, value));

  EXPECT_FALSE(wait(buf_, offset, each, micros, mask, wrong_value));
  EXPECT_TRUE(wait(buf_, offset, each, micros, mask, value));
}
