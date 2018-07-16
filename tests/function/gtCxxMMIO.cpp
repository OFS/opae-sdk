#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#include <memory>
#include <unistd.h>

#include "gtest/gtest.h"

#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/except.h>


using namespace opae::fpga::types;

class LibopaecppMMIOCommonALL_f1 : public ::testing::Test {
 protected:
  LibopaecppMMIOCommonALL_f1() {}

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
};

/**
 * @test mmio_01
 * Given an open accelerator handle object<br>
 * When I call handle::read_csr32() with an invalid csr_space<br>
 * Then an exception of type opae::fpga::types::no_access is thrown.
 */
TEST_F(LibopaecppMMIOCommonALL_f1, mmio_01) {
  EXPECT_THROW(accel_->read_csr32(0, 10), no_access);
}

/**
 * @test mmio_02
 * Given an open accelerator handle object<br>
 * When I call handle::read_csr64() with an invalid csr_space<br>
 * Then an exception of type opae::fpga::types::no_access is thrown.
 */
TEST_F(LibopaecppMMIOCommonALL_f1, mmio_02) {
  EXPECT_THROW(accel_->read_csr64(0, 10), no_access);
}

/**
 * @test mmio_03
 * Given an open accelerator handle object<br>
 * When I call handle::write_csr32() with an invalid csr_space<br>
 * Then an exception of type opae::fpga::types::no_access is thrown.
 */
TEST_F(LibopaecppMMIOCommonALL_f1, mmio_03) {
  EXPECT_THROW(accel_->write_csr32(0, 0, 10), no_access);
}

/**
 * @test mmio_04
 * Given an open accelerator handle object<br>
 * When I call handle::write_csr64() with an invalid csr_space<br>
 * Then an exception of type opae::fpga::types::no_access is thrown.
 */
TEST_F(LibopaecppMMIOCommonALL_f1, mmio_04) {
  EXPECT_THROW(accel_->write_csr64(0, 0, 10), no_access);
}

static const uint64_t nlb0_dfh      = 0x1000000000001070ULL;
static const uint64_t nlb0_afu_id_l = 0xf89e433683f9040bULL;
static const uint64_t nlb0_afu_id_h = 0xd8424dc4a4a3c413ULL;

/**
 * @test mmio_05
 * Given an open accelerator handle object<br>
 * for the NLB0 accelerator,<br>
 * When I call handle::read_csr64() with csr_space 0<br>
 * Then I can access the NLB0 DFH contents.
 */
TEST_F(LibopaecppMMIOCommonALL_f1, mmio_05) {
  EXPECT_EQ(nlb0_dfh,      accel_->read_csr64(0));
  EXPECT_EQ(nlb0_afu_id_l, accel_->read_csr64(8));
  EXPECT_EQ(nlb0_afu_id_h, accel_->read_csr64(16));
}

/**
 * @test mmio_06
 * Given an open accelerator handle object<br>
 * When I call handle::mmio_ptr() with an invalid csr_space<br>
 * Then an exception of type opae::fpga::types::no_access is thrown.
 */
TEST_F(LibopaecppMMIOCommonALL_f1, mmio_06) {
  EXPECT_THROW(accel_->mmio_ptr(0, 10), no_access);
}

/**
 * @test mmio_07
 * Given an open accelerator handle object<br>
 * for the NLB0 accelerator,<br>
 * When I call handle::mmio_ptr() with csr_space 0<br>
 * Then I can access the NLB0 DFH contents.
 */
TEST_F(LibopaecppMMIOCommonALL_f1, mmio_07) {
  uint64_t *p = reinterpret_cast<uint64_t *>(accel_->mmio_ptr(0));
  EXPECT_EQ(nlb0_dfh, *p);
  ++p;
  EXPECT_EQ(nlb0_afu_id_l, *p);
  ++p;
  EXPECT_EQ(nlb0_afu_id_h, *p);
}
